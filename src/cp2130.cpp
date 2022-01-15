/* CP2130 class for Qt - Version 2.2.0
   Copyright (c) 2021-2022 Samuel Lourenço

   This library is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or (at your
   option) any later version.

   This library is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
   License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this library.  If not, see <https://www.gnu.org/licenses/>.


   Please feel free to contact me via e-mail: samuel.fmlourenco@gmail.com */


// Includes
#include <QObject>
#include "cp2130.h"
extern "C" {
#include "libusb-extra.h"
}

// Definitions
const unsigned int TR_TIMEOUT = 500;  // Transfer timeout in milliseconds (increased to 500ms since version 2.0.2)

// Specific to getDescGeneric() and writeDescGeneric() (added in version 2.1.0)
const quint16 DESC_TBLSIZE = 0x0040;           // Descriptor table size, including preamble [64]
const size_t DESC_MAXIDX = DESC_TBLSIZE - 2;   // Maximum usable index [62]
const size_t DESC_IDXINCR = DESC_TBLSIZE - 1;  // Index increment or step between table preambles [63]

// Private generic procedure used to get any descriptor (added as a refactor in version 2.1.0)
QString CP2130::getDescGeneric(quint8 command, int &errcnt, QString &errstr)
{
    unsigned char controlBufferIn[DESC_TBLSIZE];
    controlTransfer(GET, command, 0x0000, 0x0000, controlBufferIn, DESC_TBLSIZE, errcnt, errstr);
    QString descriptor;
    size_t length = controlBufferIn[0];
    size_t end = length > DESC_MAXIDX ? DESC_MAXIDX : length;
    for (size_t i = 2; i < end; i += 2) {  // Process first 30 characters (bytes 2-61 of the array)
        if (controlBufferIn[i] != 0 || controlBufferIn[i + 1] != 0) {  // Filter out null characters
            descriptor += QChar(controlBufferIn[i + 1] << 8 | controlBufferIn[i]);  // UTF-16LE conversion as per the USB 2.0 specification
        }
    }
    if ((command == GET_MANUFACTURING_STRING_1 || command == GET_PRODUCT_STRING_1) && length > DESC_MAXIDX) {
        quint16 midchar = controlBufferIn[DESC_MAXIDX];  // Char in the middle (parted between two tables)
        controlTransfer(GET, command + 2, 0x0000, 0x0000, controlBufferIn, DESC_TBLSIZE, errcnt, errstr);
        midchar = static_cast<quint16>(controlBufferIn[0] << 8 | midchar);  // Reconstruct the char in the middle
        if (midchar != 0x0000) {  // Filter out the reconstructed char if the same is null
            descriptor += QChar(midchar);
        }
        end = length - DESC_IDXINCR;
        for (size_t i = 1; i < end; i += 2) {  // Process remaining characters, up to 31 (bytes 1-62 of the array)
            if (controlBufferIn[i] != 0 || controlBufferIn[i + 1] != 0) {  // Again, filter out null characters
                descriptor += QChar(controlBufferIn[i + 1] << 8 | controlBufferIn[i]);  // UTF-16LE conversion as per the USB 2.0 specification
            }
        }
    }
    return descriptor;
}

// Private generic procedure used to write any descriptor (added as a refactor in version 2.1.0)
void CP2130::writeDescGeneric(const QString &descriptor, quint8 command, int &errcnt, QString &errstr)
{
    size_t length = 2 * descriptor.size() + 2;
    unsigned char controlBufferOut[DESC_TBLSIZE] = {  // It is important to initialize the array in this manner, here, so that the remaining indexes are filled with zeros!
        static_cast<quint8>(length),  // USB string descriptor length
        0x03                          // USB string descriptor constant
    };
    size_t ntables = command == SET_MANUFACTURING_STRING_1 || command == SET_PRODUCT_STRING_1 ? 2 : 1;  // Number of tables to write
    for (size_t i = 0; i < ntables; ++i) {
        size_t start = i == 0 ? 2 : 0;
        size_t offset = DESC_IDXINCR * i;
        for (size_t j = start; j < DESC_IDXINCR; ++j) {
            if (offset + j < length) {  // Rewritten in version 2.1.1 in order to fix a bug
                controlBufferOut[j] = static_cast<quint8>(descriptor[static_cast<int>((offset + j - 2) / 2)].unicode() >> ((i + j) % 2 == 0 ? 0 : 8));
            } else {
                controlBufferOut[j] = 0x00;
            }
        }
        controlTransfer(SET, command + 2 * i, PROM_WRITE_KEY, 0x0000, controlBufferOut, DESC_TBLSIZE, errcnt, errstr);
    }
}

// "Equal to" operator for EventCounter
bool CP2130::EventCounter::operator ==(const CP2130::EventCounter &other) const
{
    return overflow == other.overflow && mode == other.mode && value == other.value;
}

// "Not equal to" operator for EventCounter
bool CP2130::EventCounter::operator !=(const CP2130::EventCounter &other) const
{
    return !(operator ==(other));
}

// "Equal to" operator for PinConfig
bool CP2130::PinConfig::operator ==(const CP2130::PinConfig &other) const
{
    return gpio0 == other.gpio0 && gpio1 == other.gpio1 && gpio2 == other.gpio2 && gpio3 == other.gpio3 && gpio4 == other.gpio4 && gpio5 == other.gpio5 && gpio6 == other.gpio6 && gpio7 == other.gpio7 && gpio8 == other.gpio8 && gpio9 == other.gpio9 && gpio10 == other.gpio10 && sspndlvl == other.sspndlvl && sspndmode == other.sspndmode && wkupmask == other.wkupmask && wkupmatch == other.wkupmatch && divider == other.divider;
}

// "Not equal to" operator for PinConfig
bool CP2130::PinConfig::operator !=(const CP2130::PinConfig &other) const
{
    return !(operator ==(other));
}

// "Equal to" operator for PROMConfig
bool CP2130::PROMConfig::operator ==(const CP2130::PROMConfig &other) const
{
    bool equal = true;
    for (size_t i = 0; i < PROM_BLOCKS; ++i) {
        for (size_t j = 0; j < PROM_BLOCK_SIZE; ++j) {
            if (blocks[i][j] != other.blocks[i][j]) {
                equal = false;
                break;
            }
        }
        if (!equal) {  // Added in version 2.0.1 in order to fix efficiency issue
            break;
        }
    }
    return equal;
}

// "Not equal to" operator for PROMConfig
bool CP2130::PROMConfig::operator !=(const CP2130::PROMConfig &other) const
{
    return !(operator ==(other));
}

// Subscript operator for accessing PROMConfig as a single 512-byte block
quint8 &CP2130::PROMConfig::operator [](size_t index)
{
    return blocks[index / PROM_BLOCK_SIZE][index % PROM_BLOCK_SIZE];
}

// Const version of the previous subscript operator
const quint8 &CP2130::PROMConfig::operator [](size_t index) const  // For correct error reporting and easy debugging, this operator returns a const reference instead of a value
{
    return blocks[index / PROM_BLOCK_SIZE][index % PROM_BLOCK_SIZE];
}

// "Equal to" operator for SiliconVersion
bool CP2130::SiliconVersion::operator ==(const CP2130::SiliconVersion &other) const
{
    return maj == other.maj && min == other.min;
}

// "Not equal to" operator for SiliconVersion
bool CP2130::SiliconVersion::operator !=(const CP2130::SiliconVersion &other) const
{
    return !(operator ==(other));
}

// "Equal to" operator for SPIDelays
bool CP2130::SPIDelays::operator ==(const CP2130::SPIDelays &other) const
{
    return cstglen == other.cstglen && prdasten == other.prdasten && pstasten == other.pstasten && itbyten == other.itbyten && prdastdly == other.prdastdly && pstastdly == other.pstastdly && itbytdly == other.itbytdly;
}

// "Not equal to" operator for SPIDelays
bool CP2130::SPIDelays::operator !=(const CP2130::SPIDelays &other) const
{
    return !(operator ==(other));
}

// "Equal to" operator for SPIMode
bool CP2130::SPIMode::operator ==(const CP2130::SPIMode &other) const
{
    return csmode == other.csmode && cfrq == other.cfrq && cpol == other.cpol && cpha == other.cpha;
}

// "Not equal to" operator for SPIMode
bool CP2130::SPIMode::operator !=(const CP2130::SPIMode &other) const
{
    return !(operator ==(other));
}

// "Equal to" operator for USBConfig
bool CP2130::USBConfig::operator ==(const CP2130::USBConfig &other) const
{
    return vid == other.vid && pid == other.pid && majrel == other.majrel && minrel == other.minrel && maxpow == other.maxpow && powmode == other.powmode && trfprio == other.trfprio;
}

// "Not equal to" operator for USBConfig
bool CP2130::USBConfig::operator !=(const CP2130::USBConfig &other) const
{
    return !(operator ==(other));
}

CP2130::CP2130() :
    context_(nullptr),
    handle_(nullptr),
    disconnected_(false),
    kernelWasAttached_(false)
{
}

CP2130::~CP2130()
{
    close();  // The destructor is used to close the device, and this is essential so the device can be freed when the parent object is destroyed
}

// Diagnostic function used to verify if the device has been disconnected
bool CP2130::disconnected() const
{
    return disconnected_;  // Returns true if the device has been disconnected, or false otherwise
}

// Checks if the device is open
bool CP2130::isOpen() const
{
    return handle_ != nullptr;  // Returns true if the device is open, or false otherwise
}

// Safe bulk transfer
void CP2130::bulkTransfer(quint8 endpointAddr, unsigned char *data, int length, int *transferred, int &errcnt, QString &errstr)
{
    if (!isOpen()) {
        errcnt += 1;
        errstr += QObject::tr("In bulkTransfer(): device is not open.\n");  // Program logic error
    } else {
        int result = libusb_bulk_transfer(handle_, endpointAddr, data, length, transferred, TR_TIMEOUT);
        if (result != 0 || (transferred != nullptr && *transferred != length)) {  // Since version 2.0.2, the number of transferred bytes is also verified, as long as a valid (non-null) pointer is passed via "transferred"
            errcnt += 1;
            if (endpointAddr < 0x80) {
                errstr += QObject::tr("Failed bulk OUT transfer to endpoint %1 (address 0x%2).\n").arg(0x0F & endpointAddr).arg(endpointAddr, 2, 16, QChar('0'));
            } else {
                errstr += QObject::tr("Failed bulk IN transfer from endpoint %1 (address 0x%2).\n").arg(0x0F & endpointAddr).arg(endpointAddr, 2, 16, QChar('0'));
            }
            if (result == LIBUSB_ERROR_NO_DEVICE || result == LIBUSB_ERROR_IO) {  // Note that libusb_bulk_transfer() may return "LIBUSB_ERROR_IO" [-1] on device disconnect (version 2.0.2)
                disconnected_ = true;  // This reports that the device has been disconnected
            }
        }
    }
}

// Closes the device safely, if open
void CP2130::close()
{
    if (isOpen()) {  // This condition avoids a segmentation fault if the calling algorithm tries, for some reason, to close the same device twice (e.g., if the device is already closed when the destructor is called)
        libusb_release_interface(handle_, 0);  // Release the interface
        if (kernelWasAttached_) {  // If a kernel driver was attached to the interface before
            libusb_attach_kernel_driver(handle_, 0);  // Reattach the kernel driver
        }
        libusb_close(handle_);  // Close the device
        libusb_exit(context_);  // Deinitialize libusb
        handle_ = nullptr;  // Required to mark the device as closed
    }
}

// Configures the pin mode and value for a given GPIO pin
// Note that this function can override the GPIO pin modes programmed in the OTP ROM configuration
void CP2130::configureGPIO(quint8 pin, quint8 mode, bool value,  int &errcnt, QString &errstr)
{
    if (pin > 10) {
        errcnt += 1;
        errstr += QObject::tr("In configureGPIO(): Pin number must be between 0 and 10.\n");  // Program logic error
    } else {
        unsigned char controlBufferOut[SET_GPIO_MODE_AND_LEVEL_WLEN] = {
            pin,   // Selected GPIO pin
            mode,  // Pin mode (see the values applicable to PinConfig/getPinConfig()/writePinConfig())
            value  // Output value (when applicable)
        };
        controlTransfer(SET, SET_GPIO_MODE_AND_LEVEL, 0x0000, 0x0000, controlBufferOut, SET_GPIO_MODE_AND_LEVEL_WLEN, errcnt, errstr);
    }
}

// Configures delays for a given SPI channel
void CP2130::configureSPIDelays(quint8 channel, const SPIDelays &delays, int &errcnt, QString &errstr)
{
    if (channel > 10) {
        errcnt += 1;
        errstr += QObject::tr("In configureSPIDelays(): SPI channel value must be between 0 and 10.\n");  // Program logic error
    } else {
        unsigned char controlBufferOut[SET_SPI_DELAY_WLEN] = {
            channel,                                                                                                    // Selected channel
            static_cast<quint8>(delays.cstglen << 3 | delays.prdasten << 2 | delays.pstasten << 1 | (delays.itbyten)),  // SPI enable mask (chip select toggle, pre-deassert, post-assert and inter-byte delay enable bits)
            static_cast<quint8>(delays.itbytdly >> 8), static_cast<quint8>(delays.itbytdly),                            // Inter-byte delay
            static_cast<quint8>(delays.pstastdly >> 8), static_cast<quint8>(delays.pstastdly),                          // Post-assert delay
            static_cast<quint8>(delays.prdastdly >> 8), static_cast<quint8>(delays.prdastdly)                           // Pre-deassert delay
        };
        controlTransfer(SET, SET_SPI_DELAY, 0x0000, 0x0000, controlBufferOut, SET_SPI_DELAY_WLEN, errcnt, errstr);
    }
}

// Configures the given SPI channel in respect to its chip select mode, clock frequency, polarity and phase
void CP2130::configureSPIMode(quint8 channel, const SPIMode &mode, int &errcnt, QString &errstr)
{
    if (channel > 10) {
        errcnt += 1;
        errstr += QObject::tr("In configureSPIMode(): SPI channel value must be between 0 and 10.\n");  // Program logic error
    } else {
        unsigned char controlBufferOut[SET_SPI_WORD_WLEN] = {
            channel,                                                                                      // Selected channel
            static_cast<quint8>(mode.cpha << 5 | mode.cpol << 4 | mode.csmode << 3 | (0x07 & mode.cfrq))  // Control word (specified chip select mode, clock frequency, polarity and phase)
        };
        controlTransfer(SET, SET_SPI_WORD, 0x0000, 0x0000, controlBufferOut, SET_SPI_WORD_WLEN, errcnt, errstr);
    }
}

// Safe control transfer
void CP2130::controlTransfer(quint8 bmRequestType, quint8 bRequest, quint16 wValue, quint16 wIndex, unsigned char *data, quint16 wLength, int &errcnt, QString &errstr)
{
    if (!isOpen()) {
        errcnt += 1;
        errstr += QObject::tr("In controlTransfer(): device is not open.\n");  // Program logic error
    } else {
        int result = libusb_control_transfer(handle_, bmRequestType, bRequest, wValue, wIndex, data, wLength, TR_TIMEOUT);
        if (result != wLength) {
            errcnt += 1;
            errstr += QObject::tr("Failed control transfer (0x%1, 0x%2).\n").arg(bmRequestType, 2, 16, QChar('0')).arg(bRequest, 2, 16, QChar('0'));
            if (result == LIBUSB_ERROR_NO_DEVICE || result == LIBUSB_ERROR_IO || result == LIBUSB_ERROR_PIPE) {  // Note that libusb_control_transfer() may return "LIBUSB_ERROR_IO" [-1] or "LIBUSB_ERROR_PIPE" [-9] on device disconnect (version 2.0.2)
                disconnected_ = true;  // This reports that the device has been disconnected
            }
        }
    }
}

// Disables the chip select of the target channel
void CP2130::disableCS(quint8 channel, int &errcnt, QString &errstr)
{
    if (channel > 10) {
        errcnt += 1;
        errstr += QObject::tr("In disableCS(): SPI channel value must be between 0 and 10.\n");  // Program logic error
    } else {
        unsigned char controlBufferOut[SET_GPIO_CHIP_SELECT_WLEN] = {
            channel,  // Selected channel
            0x00      // Corresponding chip select disabled
        };
        controlTransfer(SET, SET_GPIO_CHIP_SELECT, 0x0000, 0x0000, controlBufferOut, SET_GPIO_CHIP_SELECT_WLEN, errcnt, errstr);
    }
}

// Disables all SPI delays for a given channel
void CP2130::disableSPIDelays(quint8 channel, int &errcnt, QString &errstr)
{
    if (channel > 10) {
        errcnt += 1;
        errstr += QObject::tr("In disableSPIDelays(): SPI channel value must be between 0 and 10.\n");  // Program logic error
    } else {
        unsigned char controlBufferOut[SET_SPI_DELAY_WLEN] = {
            channel,     // Selected channel
            0x00,        // All SPI delays disabled, no CS toggle
            0x00, 0x00,  // Inter-byte,
            0x00, 0x00,  // post-assert and
            0x00, 0x00   // pre-deassert delays all set to 0us
        };
        controlTransfer(SET, SET_SPI_DELAY, 0x0000, 0x0000, controlBufferOut, SET_SPI_DELAY_WLEN, errcnt, errstr);
    }
}

// Enables the chip select of the target channel
void CP2130::enableCS(quint8 channel, int &errcnt, QString &errstr)
{
    if (channel > 10) {
        errcnt += 1;
        errstr += QObject::tr("In enableCS(): SPI channel value must be between 0 and 10.\n");  // Program logic error
    } else {
        unsigned char controlBufferOut[SET_GPIO_CHIP_SELECT_WLEN] = {
            channel,  // Selected channel
            0x01      // Corresponding chip select enabled
        };
        controlTransfer(SET, SET_GPIO_CHIP_SELECT, 0x0000, 0x0000, controlBufferOut, SET_GPIO_CHIP_SELECT_WLEN, errcnt, errstr);
    }
}

// Returns the current clock divider value
quint8 CP2130::getClockDivider(int &errcnt, QString &errstr)
{
    unsigned char controlBufferIn[GET_CLOCK_DIVIDER_WLEN];
    controlTransfer(GET, GET_CLOCK_DIVIDER, 0x0000, 0x0000, controlBufferIn, GET_CLOCK_DIVIDER_WLEN, errcnt, errstr);
    return controlBufferIn[0];
}

// Returns the chip select status for a given channel
bool CP2130::getCS(quint8 channel, int &errcnt, QString &errstr)
{
    bool cs;
    if (channel > 10) {
        errcnt += 1;
        errstr += QObject::tr("In getCS(): SPI channel value must be between 0 and 10.\n");  // Program logic error
        cs = false;
    } else {
        unsigned char controlBufferIn[GET_GPIO_CHIP_SELECT_WLEN];
        controlTransfer(GET, GET_GPIO_CHIP_SELECT, 0x0000, 0x0000, controlBufferIn, GET_GPIO_CHIP_SELECT_WLEN, errcnt, errstr);
        cs = (0x01 << channel & (controlBufferIn[0] << 8 | controlBufferIn[1])) != 0x00;
    }
    return cs;
}

// Returns the address of the endpoint assuming the IN direction
quint8 CP2130::getEndpointInAddr(int &errcnt, QString &errstr)
{
    return getTransferPriority(errcnt, errstr) == PRIOWRITE ? 0x82 : 0x81;
}

// Returns the address of the endpoint assuming the OUT direction
quint8 CP2130::getEndpointOutAddr(int &errcnt, QString &errstr)
{
    return getTransferPriority(errcnt, errstr) == PRIOWRITE ? 0x01 : 0x02;
}

// Gets the event counter, including mode and value
CP2130::EventCounter CP2130::getEventCounter(int &errcnt, QString &errstr)
{
    unsigned char controlBufferIn[GET_EVENT_COUNTER_WLEN];
    controlTransfer(GET, GET_EVENT_COUNTER, 0x0000, 0x0000, controlBufferIn, GET_EVENT_COUNTER_WLEN, errcnt, errstr);
    CP2130::EventCounter evtcntr;
    evtcntr.overflow = (0x80 & controlBufferIn[0]) != 0x00;                              // Event counter overflow bit corresponds to bit 7 of byte 0
    evtcntr.mode = 0x07 & controlBufferIn[0];                                            // GPIO.4/EVTCNTR pin mode corresponds to bits 3:0 of byte 0
    evtcntr.value = static_cast<quint16>(controlBufferIn[1] << 8 | controlBufferIn[2]);  // Event count value corresponds to bytes 1 and 2 (big-endian conversion)
    return evtcntr;
}

// Gets the full FIFO threshold
quint8 CP2130::getFIFOThreshold(int &errcnt, QString &errstr)
{
    unsigned char controlBufferIn[GET_FULL_THRESHOLD_WLEN];
    controlTransfer(GET, GET_FULL_THRESHOLD, 0x0000, 0x0000, controlBufferIn, GET_FULL_THRESHOLD_WLEN, errcnt, errstr);
    return controlBufferIn[0];
}

// Returns the current value of the GPIO.0 pin on the CP2130
bool CP2130::getGPIO0(int &errcnt, QString &errstr)
{
    return (BMGPIO0 & getGPIOs(errcnt, errstr)) != 0x0000;
}

// Returns the current value of the GPIO.1 pin on the CP2130
bool CP2130::getGPIO1(int &errcnt, QString &errstr)
{
    return (BMGPIO1 & getGPIOs(errcnt, errstr)) != 0x0000;
}

// Returns the current value of the GPIO.2 pin on the CP2130
bool CP2130::getGPIO2(int &errcnt, QString &errstr)
{
    return (BMGPIO2 & getGPIOs(errcnt, errstr)) != 0x0000;
}

// Returns the current value of the GPIO.3 pin on the CP2130
bool CP2130::getGPIO3(int &errcnt, QString &errstr)
{
    return (BMGPIO3 & getGPIOs(errcnt, errstr)) != 0x0000;
}

// Returns the current value of the GPIO.4 pin on the CP2130
bool CP2130::getGPIO4(int &errcnt, QString &errstr)
{
    return (BMGPIO4 & getGPIOs(errcnt, errstr)) != 0x0000;
}

// Returns the current value of the GPIO.5 pin on the CP2130
bool CP2130::getGPIO5(int &errcnt, QString &errstr)
{
    return (BMGPIO5 & getGPIOs(errcnt, errstr)) != 0x0000;
}

// Returns the current value of the GPIO.6 pin on the CP2130
bool CP2130::getGPIO6(int &errcnt, QString &errstr)
{
    return (BMGPIO6 & getGPIOs(errcnt, errstr)) != 0x0000;
}

// Returns the current value of the GPIO.7 pin on the CP2130
bool CP2130::getGPIO7(int &errcnt, QString &errstr)
{
    return (BMGPIO7 & getGPIOs(errcnt, errstr)) != 0x0000;
}

// Returns the current value of the GPIO.8 pin on the CP2130
bool CP2130::getGPIO8(int &errcnt, QString &errstr)
{
    return (BMGPIO8 & getGPIOs(errcnt, errstr)) != 0x0000;
}

// Returns the current value of the GPIO.9 pin on the CP2130
bool CP2130::getGPIO9(int &errcnt, QString &errstr)
{
    return (BMGPIO9 & getGPIOs(errcnt, errstr)) != 0x0000;
}

// Returns the current value of the GPIO.10 pin on the CP2130
bool CP2130::getGPIO10(int &errcnt, QString &errstr)
{
    return (BMGPIO10 & getGPIOs(errcnt, errstr)) != 0x0000;
}

// Returns the value of all GPIO pins on the CP2130, in bitmap format
quint16 CP2130::getGPIOs(int &errcnt, QString &errstr)
{
    unsigned char controlBufferIn[GET_GPIO_VALUES_WLEN];
    controlTransfer(GET, GET_GPIO_VALUES, 0x0000, 0x0000, controlBufferIn, GET_GPIO_VALUES_WLEN, errcnt, errstr);
    return static_cast<quint16>(BMGPIOS & (controlBufferIn[0] << 8 | controlBufferIn[1]));  // Returns the value of every GPIO pin in bitmap format (big-endian conversion)
}

// Returns the lock word from the CP2130 OTP ROM
quint16 CP2130::getLockWord(int &errcnt, QString &errstr)
{
    unsigned char controlBufferIn[GET_LOCK_BYTE_WLEN];
    controlTransfer(GET, GET_LOCK_BYTE, 0x0000, 0x0000, controlBufferIn, GET_LOCK_BYTE_WLEN, errcnt, errstr);
    return static_cast<quint16>(controlBufferIn[1] << 8 | controlBufferIn[0]);  // Returns both lock bytes as a word (little-endian conversion)
}

// Gets the manufacturer descriptor from the CP2130 OTP ROM
QString CP2130::getManufacturerDesc(int &errcnt, QString &errstr)
{
    return getDescGeneric(GET_MANUFACTURING_STRING_1, errcnt, errstr);
}

// Gets the pin configuration from the CP2130 OTP ROM
CP2130::PinConfig CP2130::getPinConfig(int &errcnt, QString &errstr)
{
    unsigned char controlBufferIn[GET_PIN_CONFIG_WLEN];
    controlTransfer(GET, GET_PIN_CONFIG, 0x0000, 0x0000, controlBufferIn, GET_PIN_CONFIG_WLEN, errcnt, errstr);
    PinConfig config;
    config.gpio0 = controlBufferIn[0];                                                        // GPIO.0 pin config corresponds to byte 0
    config.gpio1 = controlBufferIn[1];                                                        // GPIO.1 pin config corresponds to byte 1
    config.gpio2 = controlBufferIn[2];                                                        // GPIO.2 pin config corresponds to byte 2
    config.gpio3 = controlBufferIn[3];                                                        // GPIO.3 pin config corresponds to byte 3
    config.gpio4 = controlBufferIn[4];                                                        // GPIO.4 pin config corresponds to byte 4
    config.gpio5 = controlBufferIn[5];                                                        // GPIO.5 pin config corresponds to byte 5
    config.gpio6 = controlBufferIn[6];                                                        // GPIO.6 pin config corresponds to byte 6
    config.gpio7 = controlBufferIn[7];                                                        // GPIO.7 pin config corresponds to byte 7
    config.gpio8 = controlBufferIn[8];                                                        // GPIO.8 pin config corresponds to byte 8
    config.gpio9 = controlBufferIn[9];                                                        // GPIO.9 pin config corresponds to byte 9
    config.gpio10 = controlBufferIn[10];                                                      // GPIO.10 pin config corresponds to byte 10
    config.sspndlvl = static_cast<quint16>(controlBufferIn[11] << 8 | controlBufferIn[12]);   // Suspend pin level bitmap corresponds to bytes 11 and 12 (big-endian conversion)
    config.sspndmode = static_cast<quint16>(controlBufferIn[13] << 8 | controlBufferIn[14]);  // Suspend pin mode bitmap corresponds to bytes 13 and 14 (big-endian conversion)
    config.wkupmask = static_cast<quint16>(controlBufferIn[15] << 8 | controlBufferIn[16]);   // Wakeup pin mask bitmap corresponds to bytes 15 and 16 (big-endian conversion)
    config.wkupmatch = static_cast<quint16>(controlBufferIn[17] << 8 | controlBufferIn[18]);  // Wakeup pin match bitmap corresponds to bytes 17 and 18 (big-endian conversion)
    config.divider = controlBufferIn[19];                                                     // Clock divider corresponds to byte 19
    return config;
}

// Gets the product descriptor from the CP2130 OTP ROM
QString CP2130::getProductDesc(int &errcnt, QString &errstr)
{
    return getDescGeneric(GET_PRODUCT_STRING_1, errcnt, errstr);
}

// Gets the entire CP2130 OTP ROM content as a structure of eight 64-byte blocks
CP2130::PROMConfig CP2130::getPROMConfig(int &errcnt, QString &errstr)
{
    PROMConfig config;
    for (size_t i = 0; i < PROM_BLOCKS; ++i) {
        unsigned char controlBufferIn[GET_PROM_CONFIG_WLEN];
        controlTransfer(GET, GET_PROM_CONFIG, 0x0000, static_cast<quint16>(i), controlBufferIn, GET_PROM_CONFIG_WLEN, errcnt, errstr);
        for (size_t j = 0; j < PROM_BLOCK_SIZE; ++j) {
            config.blocks[i][j] = controlBufferIn[j];
        }
    }
    return config;
}

// Gets the serial descriptor from the CP2130 OTP ROM
QString CP2130::getSerialDesc(int &errcnt, QString &errstr)
{
    return getDescGeneric(GET_SERIAL_STRING, errcnt, errstr);
}

// Returns the CP2130 silicon, read-only version
CP2130::SiliconVersion CP2130::getSiliconVersion(int &errcnt, QString &errstr)
{
    unsigned char controlBufferIn[GET_READONLY_VERSION_WLEN];
    controlTransfer(GET, GET_READONLY_VERSION, 0x0000, 0x0000, controlBufferIn, GET_READONLY_VERSION_WLEN, errcnt, errstr);
    SiliconVersion version;
    version.maj = controlBufferIn[0];  // Major read-only version corresponds to byte 0
    version.min = controlBufferIn[1];  // Minor read-only version corresponds to byte 1
    return version;
}

// Returns the SPI delays for a given channel
CP2130::SPIDelays CP2130::getSPIDelays(quint8 channel, int &errcnt, QString &errstr)
{
    SPIDelays delays;
    if (channel > 10) {
        errcnt += 1;
        errstr += QObject::tr("In getSPIDelays(): SPI channel value must be between 0 and 10.\n");  // Program logic error
        delays = {false, false, false, false, 0x0000, 0x0000, 0x0000};
    } else {
        unsigned char controlBufferIn[GET_SPI_DELAY_WLEN];
        controlTransfer(GET, GET_SPI_DELAY, 0x0000, 0x0000, controlBufferIn, GET_SPI_DELAY_WLEN, errcnt, errstr);
        delays.cstglen = (0x08 & controlBufferIn[1]) != 0x00;                                   // CS toggle enable corresponds to bit 3 of byte 1
        delays.prdasten = (0x04 & controlBufferIn[1]) != 0x00;                                  // Pre-deassert delay enable corresponds to bit 2 of byte 1
        delays.pstasten = (0x02 & controlBufferIn[1]) != 0x00;                                  // Post-assert delay enable to bit 1 of byte 1
        delays.itbyten = (0x01 &controlBufferIn[1]) != 0x00;                                    // Inter-byte delay enable corresponds to bit 0 of byte 1
        delays.itbytdly = static_cast<quint16>(controlBufferIn[2] << 8 | controlBufferIn[3]);   // Inter-byte delay corresponds to bytes 2 and 3 (big-endian conversion)
        delays.pstastdly = static_cast<quint16>(controlBufferIn[4] << 8 | controlBufferIn[5]);  // Post-assert delay corresponds to bytes 4 and 5 (big-endian conversion)
        delays.prdastdly = static_cast<quint16>(controlBufferIn[6] << 8 | controlBufferIn[7]);  // Pre-deassert delay corresponds to bytes 6 and 7 (big-endian conversion)
    }
    return delays;
}

// Returns the SPI mode for a given channel
CP2130::SPIMode CP2130::getSPIMode(quint8 channel, int &errcnt, QString &errstr)
{
    SPIMode mode;
    if (channel > 10) {
        errcnt += 1;
        errstr += QObject::tr("In getSPIMode(): SPI channel value must be between 0 and 10.\n");  // Program logic error
        mode = {false, 0x00, false, false};
    } else {
        unsigned char controlBufferIn[GET_SPI_WORD_WLEN];
        controlTransfer(GET, GET_SPI_WORD, 0x0000, 0x0000, controlBufferIn, GET_SPI_WORD_WLEN, errcnt, errstr);
        mode.csmode = (0x08 & controlBufferIn[channel]) != 0x00;  // Chip select mode corresponds to bit 3
        mode.cfrq = 0x07 & controlBufferIn[channel];              // Clock frequency is set in the bits 2:0
        mode.cpha = (0x20 & controlBufferIn[channel]) != 0x00;    // Clock phase corresponds to bit 5
        mode.cpol = (0x10 &controlBufferIn[channel]) != 0x00;     // Clock polarity corresponds to bit 4
    }
    return mode;
}

// Returns the transfer priority from the CP2130 OTP ROM
quint8 CP2130::getTransferPriority(int &errcnt, QString &errstr)
{
    return getUSBConfig(errcnt, errstr).trfprio;  // Refactored in version 2.1.0, because the overhead presented by this solution was found to be very slim
}

// Gets the USB configuration, including VID, PID, major and minor release versions, from the CP2130 OTP ROM
CP2130::USBConfig CP2130::getUSBConfig(int &errcnt, QString &errstr)
{
    unsigned char controlBufferIn[GET_USB_CONFIG_WLEN];
    controlTransfer(GET, GET_USB_CONFIG, 0x0000, 0x0000, controlBufferIn, GET_USB_CONFIG_WLEN, errcnt, errstr);
    USBConfig config;
    config.vid = static_cast<quint16>(controlBufferIn[1] << 8 | controlBufferIn[0]);  // VID corresponds to bytes 0 and 1 (little-endian conversion)
    config.pid = static_cast<quint16>(controlBufferIn[3] << 8 | controlBufferIn[2]);  // PID corresponds to bytes 2 and 3 (little-endian conversion)
    config.majrel = controlBufferIn[6];                                               // Major release version corresponds to byte 6
    config.minrel = controlBufferIn[7];                                               // Minor release version corresponds to byte 7
    config.maxpow = controlBufferIn[4];                                               // Maximum power consumption corresponds to byte 4
    config.powmode = controlBufferIn[5];                                              // Power mode corresponds to byte 5
    config.trfprio = controlBufferIn[8];                                              // Transfer priority corresponds to byte 8
    return config;
}

// Returns true is the OTP ROM of the CP2130 was never written
bool CP2130::isOTPBlank(int &errcnt, QString &errstr)
{
    return getLockWord(errcnt, errstr) == 0xFFFF;
}

// Returns true is the OTP ROM of the CP2130 is locked
bool CP2130::isOTPLocked(int &errcnt, QString &errstr)
{
    return (LWALL & getLockWord(errcnt, errstr)) == 0x0000;  // Note that the reserved bits are ignored
}

// Returns true if a ReadWithRTR command is currently active
bool CP2130::isRTRActive(int &errcnt, QString &errstr)
{
    unsigned char controlBufferIn[GET_RTR_STATE_WLEN];
    controlTransfer(GET, GET_RTR_STATE, 0x0000, 0x0000, controlBufferIn, GET_RTR_STATE_WLEN, errcnt, errstr);
    return controlBufferIn[0] == 0x01;
}

// Locks the OTP ROM of the CP2130, preventing further changes
void CP2130::lockOTP(int &errcnt, QString &errstr)
{
    writeLockWord(0x0000, errcnt, errstr);  // Both lock bytes are set to zero
}

// Opens the device having the given VID, PID and, optionally, the given serial number, and assigns its handle
// Since version 2.1.0, it is not required to specify a serial number
int CP2130::open(quint16 vid, quint16 pid, const QString &serial)
{
    int retval = SUCCESS;
    if (!isOpen()) {  // Just in case the calling algorithm tries to open a device that was already sucessfully open, or tries to open different devices concurrently, all while using (or referencing to) the same object
        if (libusb_init(&context_) != 0) {  // Initialize libusb. In case of failure
            retval = ERROR_INIT;
        } else {  // If libusb is initialized
            if (serial.isNull()) {  // Note that serial, by omission, is a null QString
                handle_ = libusb_open_device_with_vid_pid(context_, vid, pid);  // If no serial number is specified, this will open the first device found with matching VID and PID
            } else {
                handle_ = libusb_open_device_with_vid_pid_serial(context_, vid, pid, reinterpret_cast<unsigned char *>(serial.toLatin1().data()));
            }
            if (handle_ == nullptr) {  // If the previous operation fails to get a device handle
                libusb_exit(context_);  // Deinitialize libusb
                retval = ERROR_NOT_FOUND;
            } else {  // If the device is successfully opened and a handle obtained
                if (libusb_kernel_driver_active(handle_, 0) == 1) {  // If a kernel driver is active on the interface
                    libusb_detach_kernel_driver(handle_, 0);  // Detach the kernel driver
                    kernelWasAttached_ = true;  // Flag that the kernel driver was attached
                } else {
                    kernelWasAttached_ = false;  // The kernel driver was not attached
                }
                if (libusb_claim_interface(handle_, 0) != 0) {  // Claim the interface. In case of failure
                    if (kernelWasAttached_) {  // If a kernel driver was attached to the interface before
                        libusb_attach_kernel_driver(handle_, 0);  // Reattach the kernel driver
                    }
                    libusb_close(handle_);  // Close the device
                    libusb_exit(context_);  // Deinitialize libusb
                    handle_ = nullptr;  // Required to mark the device as closed
                    retval = ERROR_BUSY;
                } else {
                    disconnected_ = false;  // Note that this flag is never assumed to be true for a device that was never opened - See constructor for details!
                }
            }
        }
    }
    return retval;
}

// Issues a reset to the CP2130
void CP2130::reset(int &errcnt, QString &errstr)
{
    controlTransfer(SET, RESET_DEVICE, 0x0000, 0x0000, nullptr, RESET_DEVICE_WLEN, errcnt, errstr);
}

// Enables the chip select of the target channel, disabling any others
void CP2130::selectCS(quint8 channel, int &errcnt, QString &errstr)
{
    if (channel > 10) {
        errcnt += 1;
        errstr += QObject::tr("In selectCS(): SPI channel value must be between 0 and 10.\n");  // Program logic error
    } else {
        unsigned char controlBufferOut[SET_GPIO_CHIP_SELECT_WLEN] = {
            channel,  // Selected channel
            0x02      // Only the corresponding chip select is enabled, all the others are disabled
        };
        controlTransfer(SET, SET_GPIO_CHIP_SELECT, 0x0000, 0x0000, controlBufferOut, SET_GPIO_CHIP_SELECT_WLEN, errcnt, errstr);
    }
}

// Sets the clock divider value
void CP2130::setClockDivider(quint8 value, int &errcnt, QString &errstr)
{
    unsigned char controlBufferOut[SET_CLOCK_DIVIDER_WLEN] = {
        value  // Intended clock divider value (GPIO.5 clock frequency = 24 MHz / divider)
    };
    controlTransfer(SET, SET_CLOCK_DIVIDER, 0x0000, 0x0000, controlBufferOut, SET_CLOCK_DIVIDER_WLEN, errcnt, errstr);
}

// Sets the event counter
void CP2130::setEventCounter(const EventCounter &evcntr, int &errcnt, QString &errstr)
{
    unsigned char controlBufferOut[SET_EVENT_COUNTER_WLEN] = {
        static_cast<quint8>(0x07 & evcntr.mode),                                   // Set GPIO.4/EVTCNTR pin mode
        static_cast<quint8>(evcntr.value >> 8), static_cast<quint8>(evcntr.value)  // Set the event count value
    };
    controlTransfer(SET, SET_EVENT_COUNTER, 0x0000, 0x0000, controlBufferOut, SET_EVENT_COUNTER_WLEN, errcnt, errstr);
}

// Sets the full FIFO threshold
void CP2130::setFIFOThreshold(quint8 threshold, int &errcnt, QString &errstr)
{
    unsigned char controlBufferOut[SET_FULL_THRESHOLD_WLEN] = {
        threshold  // Intended FIFO threshold
    };
    controlTransfer(SET, SET_FULL_THRESHOLD, 0x0000, 0x0000, controlBufferOut, SET_FULL_THRESHOLD_WLEN, errcnt, errstr);
}

// Sets the GPIO.0 pin on the CP2130 to a given value
void CP2130::setGPIO0(bool value, int &errcnt, QString &errstr)
{
    setGPIOs(BMGPIOS * value, BMGPIO0, errcnt, errstr);
}

// Sets the GPIO.1 pin on the CP2130 to a given value
void CP2130::setGPIO1(bool value, int &errcnt, QString &errstr)
{
    setGPIOs(BMGPIOS * value, BMGPIO1, errcnt, errstr);
}

// Sets the GPIO.2 pin on the CP2130 to a given value
void CP2130::setGPIO2(bool value, int &errcnt, QString &errstr)
{
    setGPIOs(BMGPIOS * value, BMGPIO2, errcnt, errstr);
}

// Sets the GPIO.3 pin on the CP2130 to a given value
void CP2130::setGPIO3(bool value, int &errcnt, QString &errstr)
{
    setGPIOs(BMGPIOS * value, BMGPIO3, errcnt, errstr);
}

// Sets the GPIO.4 pin on the CP2130 to a given value
void CP2130::setGPIO4(bool value, int &errcnt, QString &errstr)
{
    setGPIOs(BMGPIOS * value, BMGPIO4, errcnt, errstr);
}

// Sets the GPIO.5 pin on the CP2130 to a given value
void CP2130::setGPIO5(bool value, int &errcnt, QString &errstr)
{
    setGPIOs(BMGPIOS * value, BMGPIO5, errcnt, errstr);
}

// Sets the GPIO.6 pin on the CP2130 to a given value
void CP2130::setGPIO6(bool value, int &errcnt, QString &errstr)
{
    setGPIOs(BMGPIOS * value, BMGPIO6, errcnt, errstr);
}

// Sets the GPIO.7 pin on the CP2130 to a given value
void CP2130::setGPIO7(bool value, int &errcnt, QString &errstr)
{
    setGPIOs(BMGPIOS * value, BMGPIO7, errcnt, errstr);
}

// Sets the GPIO.8 pin on the CP2130 to a given value
void CP2130::setGPIO8(bool value, int &errcnt, QString &errstr)
{
    setGPIOs(BMGPIOS * value, BMGPIO8, errcnt, errstr);
}

// Sets the GPIO.9 pin on the CP2130 to a given value
void CP2130::setGPIO9(bool value, int &errcnt, QString &errstr)
{
    setGPIOs(BMGPIOS * value, BMGPIO9, errcnt, errstr);
}

// Sets the GPIO.10 pin on the CP2130 to a given value
void CP2130::setGPIO10(bool value, int &errcnt, QString &errstr)
{
    setGPIOs(BMGPIOS * value, BMGPIO10, errcnt, errstr);
}

// Sets one or more GPIO pins on the CP2130 to the intended values, according to the values and mask bitmaps
void CP2130::setGPIOs(quint16 bmValues, quint16 bmMask, int &errcnt, QString &errstr)
{
    unsigned char controlBufferOut[SET_GPIO_VALUES_WLEN] = {
        static_cast<quint8>((BMGPIOS & bmValues) >> 8), static_cast<quint8>(BMGPIOS & bmValues),  // GPIO values bitmap
        static_cast<quint8>((BMGPIOS & bmMask) >> 8), static_cast<quint8>(BMGPIOS & bmMask)       // Mask bitmap
    };
    controlTransfer(SET, SET_GPIO_VALUES, 0x0000, 0x0000, controlBufferOut, SET_GPIO_VALUES_WLEN, errcnt, errstr);
}

// Requests and reads the given number of bytes from the SPI bus, and then returns a vector
// This is the prefered method of reading from the bus, if both endpoint addresses are known
QVector<quint8> CP2130::spiRead(quint32 bytesToRead, quint8 endpointInAddr, quint8 endpointOutAddr, int &errcnt, QString &errstr)
{
    unsigned char readCommandBuffer[8] = {
        0x00, 0x00,    // Reserved
        CP2130::READ,  // Read command
        0x00,          // Reserved
        static_cast<quint8>(bytesToRead),
        static_cast<quint8>(bytesToRead >> 8),
        static_cast<quint8>(bytesToRead >> 16),
        static_cast<quint8>(bytesToRead >> 24)
    };
#if LIBUSB_API_VERSION >= 0x01000105
    bulkTransfer(endpointOutAddr, readCommandBuffer, static_cast<int>(sizeof(readCommandBuffer)), nullptr, errcnt, errstr);
#else
    int bytesWritten;
    bulkTransfer(endpointOutAddr, readCommandBuffer, static_cast<int>(sizeof(readCommandBuffer)), &bytesWritten, errcnt, errstr);
#endif
    unsigned char *readInputBuffer = new unsigned char[bytesToRead];  // Allocated dynamically since version 2.1.0
    int bytesRead = 0;  // Important!
    bulkTransfer(endpointInAddr, readInputBuffer, static_cast<int>(bytesToRead), &bytesRead, errcnt, errstr);
    QVector<quint8> retdata(bytesRead);
    for (int i = 0; i < bytesRead; ++i) {
        retdata[i] = readInputBuffer[i];
    }
    delete[] readInputBuffer;
    return retdata;
}

// This function is a shorthand version of the previous one (both endpoint addresses are automatically deduced, at the cost of decreased speed)
QVector<quint8> CP2130::spiRead(quint32 bytesToRead, int &errcnt, QString &errstr)
{
    return spiRead(bytesToRead, getEndpointInAddr(errcnt, errstr), getEndpointOutAddr(errcnt, errstr), errcnt, errstr);
}

// Writes to the SPI bus, using the given vector
// This is the prefered method of writing to the bus, if the endpoint OUT address is known
void CP2130::spiWrite(const QVector<quint8> &data, quint8 endpointOutAddr, int &errcnt, QString &errstr)
{
    quint32 bytesToWrite = static_cast<quint32>(data.size());  // Conversion done for sanity purposes
    int bufsize = bytesToWrite + 8;
    unsigned char *writeCommandBuffer = new unsigned char[bufsize] {  // Allocated dynamically since version 2.1.0
        0x00, 0x00,     // Reserved
        CP2130::WRITE,  // Write command
        0x00,           // Reserved
        static_cast<quint8>(bytesToWrite),
        static_cast<quint8>(bytesToWrite >> 8),
        static_cast<quint8>(bytesToWrite >> 16),
        static_cast<quint8>(bytesToWrite >> 24)
    };
    for (size_t i = 0; i < bytesToWrite; ++i) {
        writeCommandBuffer[i + 8] = data[i];
    }
#if LIBUSB_API_VERSION >= 0x01000105
    bulkTransfer(endpointOutAddr, writeCommandBuffer, bufsize, nullptr, errcnt, errstr);
#else
    int bytesWritten;
    bulkTransfer(endpointOutAddr, writeCommandBuffer, bufsize, &bytesWritten, errcnt, errstr);
#endif
    delete[] writeCommandBuffer;
}

// This function is a shorthand version of the previous one (the endpoint OUT address is automatically deduced at the cost of decreased speed)
void CP2130::spiWrite(const QVector<quint8> &data, int &errcnt, QString &errstr)
{
    spiWrite(data, getEndpointOutAddr(errcnt, errstr), errcnt, errstr);
}

// Writes to the SPI bus while reading back, returning a vector of the same size as the one given
// This is the prefered method of writing and reading, if both endpoint addresses are known
QVector<quint8> CP2130::spiWriteRead(const QVector<quint8> &data, quint8 endpointInAddr, quint8 endpointOutAddr, int &errcnt, QString &errstr)
{
    size_t bytesToWriteRead = static_cast<size_t>(data.size());
    size_t bytesLeft = bytesToWriteRead;
    QVector<quint8> retdata;
    while (bytesLeft > 0) {
        int payload = bytesLeft > 56 ? 56 : bytesLeft;
        int bufsize = payload + 8;
        unsigned char *writeReadCommandBuffer = new unsigned char[bufsize] {
            0x00, 0x00,         // Reserved
            CP2130::WRITEREAD,  // WriteRead command
            0x00,               // Reserved
            static_cast<quint8>(payload),
            static_cast<quint8>(payload >> 8),
            static_cast<quint8>(payload >> 16),
            static_cast<quint8>(payload >> 24)
        };
        for (int i = 0; i < payload; ++i) {
            writeReadCommandBuffer[i + 8] = data[bytesToWriteRead - bytesLeft + i];
        }
#if LIBUSB_API_VERSION >= 0x01000105
        bulkTransfer(endpointOutAddr, writeReadCommandBuffer, bufsize, nullptr, errcnt, errstr);
#else
        int bytesWritten;
        bulkTransfer(endpointOutAddr, writeReadCommandBuffer, bufsize, &bytesWritten, errcnt, errstr);
#endif
        delete[] writeReadCommandBuffer;
        unsigned char *writeReadInputBuffer = new unsigned char[payload];
        int bytesRead = 0;  // Important!
        bulkTransfer(endpointInAddr, writeReadInputBuffer, payload, &bytesRead, errcnt, errstr);
        for (int i = 0; i < bytesRead; ++i) {
            retdata += writeReadInputBuffer[i];
        }
        delete[] writeReadInputBuffer;
        bytesLeft -= payload;
    }
    return retdata;
}

// This function is a shorthand version of the previous one (both endpoint addresses are automatically deduced, at the cost of decreased speed)
QVector<quint8> CP2130::spiWriteRead(const QVector<quint8> &data, int &errcnt, QString &errstr)
{
    return spiWriteRead(data, getEndpointInAddr(errcnt, errstr), getEndpointOutAddr(errcnt, errstr), errcnt, errstr);
}

// Aborts the current ReadWithRTR command
void CP2130::stopRTR(int &errcnt, QString &errstr)
{
    unsigned char controlBufferOut[SET_RTR_STOP_WLEN] = {
        0x01  // Abort current ReadWithRTR command
    };
    controlTransfer(SET, SET_RTR_STOP, 0x0000, 0x0000, controlBufferOut, SET_RTR_STOP_WLEN, errcnt, errstr);
}

// This procedure is used to lock fields in the CP2130 OTP ROM - Use with care!
void CP2130::writeLockWord(quint16 word, int &errcnt, QString &errstr)
{
    unsigned char controlBufferOut[SET_LOCK_BYTE_WLEN] = {
        static_cast<quint8>(word), static_cast<quint8>(word >> 8)  // Sets both lock bytes to the intended value
    };
    controlTransfer(SET, SET_LOCK_BYTE, PROM_WRITE_KEY, 0x0000, controlBufferOut, SET_LOCK_BYTE_WLEN, errcnt, errstr);
}

// Writes the manufacturer descriptor to the CP2130 OTP ROM
void CP2130::writeManufacturerDesc(const QString &manufacturer, int &errcnt, QString &errstr)
{
    size_t strsize = static_cast<size_t>(manufacturer.size());
    if (strsize > DESCMXL_MANUFACTURER) {
        errcnt += 1;
        errstr += QObject::tr("In writeManufacturerDesc(): manufacturer descriptor string cannot be longer than 62 characters.\n");  // Program logic error
    } else {
        writeDescGeneric(manufacturer, SET_MANUFACTURING_STRING_1, errcnt, errstr);  // Refactored in version 2.1.0
    }
}

// Writes the pin configuration to the CP2130 OTP ROM
void CP2130::writePinConfig(const PinConfig &config, int &errcnt, QString &errstr)
{
    unsigned char controlBufferOut[SET_PIN_CONFIG_WLEN] = {
        config.gpio0,                                                                              // GPIO.0 pin config
        config.gpio1,                                                                              // GPIO.1 pin config
        config.gpio2,                                                                              // GPIO.2 pin config
        config.gpio3,                                                                              // GPIO.3 pin config
        config.gpio4,                                                                              // GPIO.4 pin config
        config.gpio5,                                                                              // GPIO.5 pin config
        config.gpio6,                                                                              // GPIO.6 pin config
        config.gpio7,                                                                              // GPIO.7 pin config
        config.gpio8,                                                                              // GPIO.8 pin config
        config.gpio9,                                                                              // GPIO.9 pin config
        config.gpio10,                                                                             // GPIO.10 pin config
        static_cast<quint8>(0x7F & config.sspndlvl >> 8), static_cast<quint8>(config.sspndlvl),    // Suspend pin level bitmap
        static_cast<quint8>(config.sspndmode >> 8), static_cast<quint8>(config.sspndmode),         // Suspend pin mode bitmap
        static_cast<quint8>(0x7F & config.wkupmask >> 8), static_cast<quint8>(config.wkupmask),    // Wakeup pin mask bitmap
        static_cast<quint8>(0x7F & config.wkupmatch >> 8), static_cast<quint8>(config.wkupmatch),  // Wakeup pin match bitmap
        config.divider                                                                             // Clock divider
    };
    controlTransfer(SET, SET_PIN_CONFIG, PROM_WRITE_KEY, 0x0000, controlBufferOut, SET_PIN_CONFIG_WLEN, errcnt, errstr);
}

// Writes the product descriptor to the CP2130 OTP ROM
void CP2130::writeProductDesc(const QString &product, int &errcnt, QString &errstr)
{
    size_t strsize = static_cast<size_t>(product.size());
    if (strsize > DESCMXL_PRODUCT) {
        errcnt += 1;
        errstr += QObject::tr("In writeProductDesc(): product descriptor string cannot be longer than 62 characters.\n");  // Program logic error
    } else {
        writeDescGeneric(product, SET_PRODUCT_STRING_1, errcnt, errstr);  // Refactored in version 2.1.0
    }
}

// Writes over the entire CP2130 OTP ROM
void CP2130::writePROMConfig(const PROMConfig &config, int &errcnt, QString &errstr)
{
    for (size_t i = 0; i < PROM_BLOCKS; ++i) {
        unsigned char controlBufferOut[SET_PROM_CONFIG_WLEN];
        for (size_t j = 0; j < PROM_BLOCK_SIZE; ++j) {
            controlBufferOut[j] = config.blocks[i][j];
        }
        controlTransfer(SET, SET_PROM_CONFIG, PROM_WRITE_KEY, static_cast<quint16>(i), controlBufferOut, SET_PROM_CONFIG_WLEN, errcnt, errstr);
    }
}

// Writes the serial descriptor to the CP2130 OTP ROM
void CP2130::writeSerialDesc(const QString &serial, int &errcnt, QString &errstr)
{
    size_t strsize = static_cast<size_t>(serial.size());
    if (strsize > DESCMXL_SERIAL) {
        errcnt += 1;
        errstr += QObject::tr("In writeSerialDesc(): serial descriptor string cannot be longer than 30 characters.\n");  // Program logic error
    } else {
        writeDescGeneric(serial, SET_SERIAL_STRING, errcnt, errstr);  // Refactored in version 2.1.0
    }
}

// Writes the USB configuration to the CP2130 OTP ROM
void CP2130::writeUSBConfig(const USBConfig &config, quint8 mask, int &errcnt, QString &errstr)
{
    unsigned char controlBufferOut[SET_USB_CONFIG_WLEN] = {
        static_cast<quint8>(config.vid), static_cast<quint8>(config.vid >> 8),  // VID
        static_cast<quint8>(config.pid), static_cast<quint8>(config.pid >> 8),  // PID
        config.maxpow,                                                          // Maximum consumption current
        config.powmode,                                                         // Power mode
        config.majrel, config.minrel,                                           // Major and minor release versions
        config.trfprio,                                                         // Transfer priority
        mask                                                                    // Write mask (can be obtained using the return value of getLockWord(), after being bitwise ANDed with "LWUSBCFG" [0x009F] and the resulting value cast to quint8)
    };
    controlTransfer(SET, SET_USB_CONFIG, PROM_WRITE_KEY, 0x0000, controlBufferOut, SET_USB_CONFIG_WLEN, errcnt, errstr);
}

// Helper function to list devices
QStringList CP2130::listDevices(quint16 vid, quint16 pid, int &errcnt, QString &errstr)
{
    QStringList devices;
    libusb_context *context;
    if (libusb_init(&context) != 0) {  // Initialize libusb. In case of failure
        errcnt += 1;
        errstr += QObject::tr("Could not initialize libusb.\n");
    } else {  // If libusb is initialized
        libusb_device **devs;
        ssize_t devlist = libusb_get_device_list(context, &devs);  // Get a device list
        if (devlist < 0) {  // If the previous operation fails to get a device list
            errcnt += 1;
            errstr += QObject::tr("Failed to retrieve a list of devices.\n");
        } else {
            for (ssize_t i = 0; i < devlist; ++i) {  // Run through all listed devices
                struct libusb_device_descriptor desc;
                if (libusb_get_device_descriptor(devs[i], &desc) == 0 && desc.idVendor == vid && desc.idProduct == pid) {  // If the device descriptor is retrieved, and both VID and PID correspond to the respective given values
                    libusb_device_handle *handle;
                    if (libusb_open(devs[i], &handle) == 0) {  // Open the listed device. If successfull
                        unsigned char str_desc[256];
                        libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, str_desc, static_cast<int>(sizeof(str_desc)));  // Get the serial number string in ASCII format
                        devices += reinterpret_cast<char *>(str_desc);  // Append the serial number string to the list
                        libusb_close(handle);  // Close the device
                    }
                }
            }
            libusb_free_device_list(devs, 1);  // Free device list
        }
        libusb_exit(context);  // Deinitialize libusb
    }
    return devices;
}
