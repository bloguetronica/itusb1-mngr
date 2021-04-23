/* CP2130 class for Qt - Version 0.4.1
   Copyright (c) 2021 Samuel Lourenço

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
const quint16 MEM_KEY = 0xA5F1;
const unsigned int TR_TIMEOUT = 100;  // Transfer timeout in milliseconds

CP2130::CP2130() :
    context_(nullptr),
    handle_(nullptr),
    kernelAttached_(false)
{
}

CP2130::~CP2130()
{
    close();  // The destructor is used to close the device, and this is essential so the device can be freed when the parent object is destroyed
}

// Safe bulk transfer
void CP2130::bulkTransfer(quint8 endpoint, unsigned char *data, int length, int *transferred, int &errcnt, QString &errstr) const
{
    if (!isOpen()) {
        errcnt += 1;
        errstr.append(QObject::tr("In bulkTransfer(): device is not open.\n"));  // Programmer error
    } else if (libusb_bulk_transfer(handle_, endpoint, data, length, transferred, TR_TIMEOUT) != 0) {
        errcnt += 1;
        if (endpoint < 0x80) {
            errstr.append(QObject::tr("Failed bulk OUT transfer to endpoint %1 (address 0x%2).\n").arg(0x0F & endpoint).arg(endpoint, 2, 16, QChar('0')));
        } else {
            errstr.append(QObject::tr("Failed bulk IN transfer from endpoint %1 (address 0x%2).\n").arg(0x0F & endpoint).arg(endpoint, 2, 16, QChar('0')));
        }
    }
}

// Configures delays for a given SPI channel
void CP2130::configureSPIDelays(quint8 channel, const SPIDelays &delays, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[8] = {
        channel,                                                                                                    // Selected channel
        static_cast<quint8>(delays.cstglen << 3 | delays.prdasten << 2 | delays.pstasten << 1 | (delays.itbyten)),  // SPI enable mask (chip select toggle, pre-deassert, post-assert and inter-byte delay enable bits)
        static_cast<quint8>(delays.itbytdly >> 8), static_cast<quint8>(delays.itbytdly),                            // Inter-byte delay
        static_cast<quint8>(delays.pstastdly >> 8), static_cast<quint8>(delays.pstastdly),                          // Post-assert delay
        static_cast<quint8>(delays.prdastdly >> 8), static_cast<quint8>(delays.prdastdly)                           // Pre-deassert delay
    };
    controlTransfer(0x40, 0x33, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Configures the given SPI channel in respect to its chip select mode, clock frequency, polarity and phase
void CP2130::configureSPIMode(quint8 channel, const SPIMode &mode, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[2] = {
        channel,                                                                                      // Selected channel
        static_cast<quint8>(mode.cpha << 5 | mode.cpol << 4 | mode.csmode << 3 | (0x07 & mode.cfrq))  // Control word (specified chip select mode, clock frequency, polarity and phase)
    };
    controlTransfer(0x40, 0x31, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Safe control transfer
void CP2130::controlTransfer(quint8 bmRequestType, quint8 bRequest, quint16 wValue, quint16 wIndex, unsigned char *data, quint16 wLength, int &errcnt, QString &errstr) const
{
    if (!isOpen()) {
        errcnt += 1;
        errstr.append(QObject::tr("In controlTransfer(): device is not open.\n"));  // Programmer error
    } else if (libusb_control_transfer(handle_, bmRequestType, bRequest, wValue, wIndex, data, wLength, TR_TIMEOUT) != wLength) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x%1, 0x%2).\n").arg(bmRequestType, 2, 16, QChar('0')).arg(bRequest, 2, 16, QChar('0')));
    }
}

// Disables the chip select of the target channel
void CP2130::disableCS(quint8 channel, int &errcnt, QString &errstr) const
{
    if (channel > 10) {
        errcnt += 1;
        errstr.append(QObject::tr("In disableCS(): SPI channel value must be between 0 and 10.\n"));  // Programmer error
    } else {
        unsigned char controlBufferOut[2] = {
            channel,  // Selected channel
            0x00      // Corresponding chip select disabled
        };
        controlTransfer(0x40, 0x25, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
    }
}

// Disables all SPI delays for a given channel
void CP2130::disableSPIDelays(quint8 channel, int &errcnt, QString &errstr) const
{
    if (channel > 10) {
        errcnt += 1;
        errstr.append(QObject::tr("In disableSPIDelays(): SPI channel value must be between 0 and 10.\n"));  // Programmer error
    } else {
        unsigned char controlBufferOut[8] = {
            channel,     // Selected channel
            0x00,        // All SPI delays disabled, no CS toggle
            0x00, 0x00,  // Inter-byte,
            0x00, 0x00,  // post-assert and
            0x00, 0x00   // pre-deassert delays all set to 0us
        };
        controlTransfer(0x40, 0x33, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
    }
}

// Enables the chip select of the target channel
void CP2130::enableCS(quint8 channel, int &errcnt, QString &errstr) const
{
    if (channel > 10) {
        errcnt += 1;
        errstr.append(QObject::tr("In enableCS(): SPI channel value must be between 0 and 10.\n"));  // Programmer error
    } else {
        unsigned char controlBufferOut[2] = {
            channel,  // Selected channel
            0x01      // Corresponding chip select enabled
        };
        controlTransfer(0x40, 0x25, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
    }
}

// Returns the chip select status for a given channel
bool CP2130::getCS(quint8 channel, int &errcnt, QString &errstr) const
{
    bool retval;
    if (channel > 10) {
        errcnt += 1;
        errstr.append(QObject::tr("In getCS(): SPI channel value must be between 0 and 10.\n"));  // Programmer error
        retval = false;
    } else {
        unsigned char controlBufferIn[4];
        controlTransfer(0xC0, 0x24, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
        retval = ((0x01 << channel & (controlBufferIn[0] << 8 | controlBufferIn[1])) != 0x00);
    }
    return retval;
}

// Returns the current value of the GPIO.1 pin on the CP2130
bool CP2130::getGPIO1(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
    return ((0x10 & controlBufferIn[1]) != 0x00);  // Returns one if bit 4 of byte 1, which corresponds to the GPIO.1 pin, is not set to zero
}

// Returns the current value of the GPIO.2 pin on the CP2130
bool CP2130::getGPIO2(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
    return ((0x20 & controlBufferIn[1]) != 0x00);  // Returns one if bit 5 of byte 1, which corresponds to the GPIO.2 pin, is not set to zero
}

// Returns the current value of the GPIO.3 pin on the CP2130
bool CP2130::getGPIO3(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
    return ((0x40 & controlBufferIn[1]) != 0x00);  // Returns one if bit 6 of byte 1, which corresponds to the GPIO.3 pin, is not set to zero
}

// Returns the current value of the GPIO.4 pin on the CP2130
bool CP2130::getGPIO4(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
    return ((0x80 & controlBufferIn[1]) != 0x00);  // Returns one if bit 7 of byte 1, which corresponds to the GPIO.4 pin, is not set to zero
}

// Returns the current value of the GPIO.5 pin on the CP2130
bool CP2130::getGPIO5(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
    return ((0x01 & controlBufferIn[0]) != 0x00);  // Returns one if bit 0 of byte 0, which corresponds to the GPIO.5 pin, is not set to zero
}

// Returns the current value of the GPIO.6 pin on the CP2130
bool CP2130::getGPIO6(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
    return ((0x04 & controlBufferIn[0]) != 0x00);  // Returns one if bit 2 of byte 0, which corresponds to the GPIO.5 pin, is not set to zero
}

// Returns the current value of the GPIO.7 pin on the CP2130
bool CP2130::getGPIO7(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
    return ((0x08 & controlBufferIn[0]) != 0x00);  // Returns one if bit 3 of byte 0, which corresponds to the GPIO.5 pin, is not set to zero
}

// Returns the current value of the GPIO.8 pin on the CP2130
bool CP2130::getGPIO8(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
    return ((0x10 & controlBufferIn[0]) != 0x00);  // Returns one if bit 4 of byte 0, which corresponds to the GPIO.5 pin, is not set to zero
}

// Returns the current value of the GPIO.9 pin on the CP2130
bool CP2130::getGPIO9(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
    return ((0x20 & controlBufferIn[0]) != 0x00);  // Returns one if bit 5 of byte 0, which corresponds to the GPIO.5 pin, is not set to zero
}

// Returns the current value of the GPIO.10 pin on the CP2130
bool CP2130::getGPIO10(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
    return ((0x40 & controlBufferIn[0]) != 0x00);  // Returns one if bit 6 of byte 0, which corresponds to the GPIO.5 pin, is not set to zero
}

// Returns the lock word from the CP2130 OTP ROM
quint16 CP2130::getLockWord(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    controlTransfer(0xC0, 0x6E, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
    return static_cast<quint16>(controlBufferIn[1] << 8 | controlBufferIn[0]);  // Returns both lock bytes as a word (little-endian conversion)
}

// Gets the manufacturer descriptor from the CP2130 OTP ROM
QString CP2130::getManufacturerDesc(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[64];
    quint16 bufsize = static_cast<quint16>(sizeof(controlBufferIn));
    controlTransfer(0xC0, 0x62, 0x0000, 0x0000, controlBufferIn, bufsize, errcnt, errstr);
    QString manufacturer;
    int length = controlBufferIn[0];
    int end = length > 62 ? 62 : length;
    for (int i = 2; i < end; i += 2) {  // Process first 30 characters (bytes 2-61 of the array)
        if (controlBufferIn[i] != 0 || controlBufferIn[i + 1] != 0) {  // Filter out null characters
            manufacturer.append(QChar(controlBufferIn[i + 1] << 8 | controlBufferIn[i]));  // UTF-16LE conversion as per the USB 2.0 specification
        }
    }
    if (length > 62) {
        quint16 midchar = controlBufferIn[62];  // Char in the middle (parted between two tables)
        controlTransfer(0xC0, 0x64, 0x0000, 0x0000, controlBufferIn, bufsize, errcnt, errstr);
        midchar = static_cast<quint16>(controlBufferIn[0] << 8 | midchar);  // Reconstruct the char in the middle
        if (midchar != 0x0000) {  // Filter out the reconstructed char if the same is null
            manufacturer.append(QChar(midchar));
        }
        end = length - 63;
        for (int i = 1; i < end; i += 2) {  // Process remaining characters, up to 31 (bytes 1-62 of the array)
            if (controlBufferIn[i] != 0 || controlBufferIn[i + 1] != 0) {  // Again, filter out null characters
                manufacturer.append(QChar(controlBufferIn[i + 1] << 8 | controlBufferIn[i]));  // UTF-16LE conversion as per the USB 2.0 specification
            }
        }
    }
    return manufacturer;
}

// Gets the pin configuration from the CP2130 OTP ROM
CP2130::PinConfig CP2130::getPinConfig(int &errcnt, QString &errstr) const
{
    PinConfig config;
    unsigned char controlBufferIn[20];
    controlTransfer(0xC0, 0x6C, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
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
QString CP2130::getProductDesc(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[64];
    quint16 bufsize = static_cast<quint16>(sizeof(controlBufferIn));
    controlTransfer(0xC0, 0x66, 0x0000, 0x0000, controlBufferIn, bufsize, errcnt, errstr);
    QString product;
    int length = controlBufferIn[0];
    int end = length > 62 ? 62 : length;
    for (int i = 2; i < end; i += 2) {  // Process first 30 characters (bytes 2-61 of the array)
        if (controlBufferIn[i] != 0 || controlBufferIn[i + 1] != 0) {  // Filter out null characters
            product.append(QChar(controlBufferIn[i + 1] << 8 | controlBufferIn[i]));  // UTF-16LE conversion as per the USB 2.0 specification
        }
    }
    if (length > 62) {
        quint16 midchar = controlBufferIn[62];  // Char in the middle (parted between two tables)
        controlTransfer(0xC0, 0x68, 0x0000, 0x0000, controlBufferIn, bufsize, errcnt, errstr);
        midchar = static_cast<quint16>(controlBufferIn[0] << 8 | midchar);  // Reconstruct the char in the middle
        if (midchar != 0x0000) {  // Filter out the reconstructed char if the same is null
            product.append(QChar(midchar));
        }
        end = length - 63;
        for (int i = 1; i < end; i += 2) {  // Process remaining characters, up to 31 (bytes 1-62 of the array)
            if (controlBufferIn[i] != 0 || controlBufferIn[i + 1] != 0) {  // Again, filter out null characters
                product.append(QChar(controlBufferIn[i + 1] << 8 | controlBufferIn[i]));  // UTF-16LE conversion as per the USB 2.0 specification
            }
        }
    }
    return product;
}

// Gets the serial descriptor from the CP2130 OTP ROM
QString CP2130::getSerialDesc(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[64];
    controlTransfer(0xC0, 0x6A, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
    QString serial;
    for (int i = 2; i < controlBufferIn[0]; i += 2) {
        if (controlBufferIn[i] != 0 || controlBufferIn[i + 1] != 0) {  // Filter out null characters
            serial.append(QChar(controlBufferIn[i + 1] << 8 | controlBufferIn[i]));  // UTF-16LE conversion as per the USB 2.0 specification
        }
    }
    return serial;
}

// Returns the SPI delays for a given channel
CP2130::SPIDelays CP2130::getSPIDelays(quint8 channel, int &errcnt, QString &errstr) const
{
    SPIDelays delays;
    if (channel > 10) {
        errcnt += 1;
        errstr.append(QObject::tr("In getSPIDelays(): SPI channel value must be between 0 and 10.\n"));  // Programmer error
        delays = {false, false, false, false, 0x0000, 0x0000, 0x0000};
    } else {
        unsigned char controlBufferIn[8];
        controlTransfer(0xC0, 0x32, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
        delays.cstglen = ((0x08 & controlBufferIn[1]) != 0x00);                                 // CS toggle enable corresponds to bit 3 of byte 1
        delays.prdasten = ((0x04 & controlBufferIn[1]) != 0x00);                                // Pre-deassert delay enable corresponds to bit 2 of byte 1
        delays.pstasten = ((0x02 & controlBufferIn[1]) != 0x00);                                // Post-assert delay enable to bit 1 of byte 1
        delays.itbyten = ((0x01 &controlBufferIn[1]) != 0x00);                                  // Inter-byte delay enable corresponds to bit 0 of byte 1
        delays.itbytdly = static_cast<quint16>(controlBufferIn[2] << 8 | controlBufferIn[3]);   // Inter-byte delay corresponds to bytes 2 and 3 (big-endian conversion)
        delays.pstastdly = static_cast<quint16>(controlBufferIn[4] << 8 | controlBufferIn[5]);  // Post-assert delay corresponds to bytes 4 and 5 (big-endian conversion)
        delays.prdastdly = static_cast<quint16>(controlBufferIn[6] << 8 | controlBufferIn[7]);  // Pre-deassert delay corresponds to bytes 6 and 7 (big-endian conversion)

    }
    return delays;
}

// Returns the SPI mode for a given channel
CP2130::SPIMode CP2130::getSPIMode(quint8 channel, int &errcnt, QString &errstr) const
{
    SPIMode mode;
    if (channel > 10) {
        errcnt += 1;
        errstr.append(QObject::tr("In getSPIMode(): SPI channel value must be between 0 and 10.\n"));  // Programmer error
        mode = {false, 0x00, false, false};
    } else {
        unsigned char controlBufferIn[11];
        controlTransfer(0xC0, 0x30, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
        mode.csmode = ((0x08 & controlBufferIn[channel]) != 0x00);  // Chip select mode corresponds to bit 3
        mode.cfrq = 0x07 & controlBufferIn[channel];                // Clock frequency is set in the bits 2:0
        mode.cpha = ((0x20 & controlBufferIn[channel]) != 0x00);    // Clock phase corresponds to bit 5
        mode.cpol = ((0x10 &controlBufferIn[channel]) != 0x00);     // Clock polarity corresponds to bit 4
    }
    return mode;
}

// Gets the USB configuration, including VID, PID, major and minor release versions, from the CP2130 OTP ROM
CP2130::USBConfig CP2130::getUSBConfig(int &errcnt, QString &errstr) const
{
    USBConfig config;
    unsigned char controlBufferIn[9];
    controlTransfer(0xC0, 0x60, 0x0000, 0x0000, controlBufferIn, static_cast<quint16>(sizeof(controlBufferIn)), errcnt, errstr);
    config.vid = static_cast<quint16>(controlBufferIn[1] << 8 | controlBufferIn[0]);  // VID corresponds to bytes 0 and 1 (little-endian conversion)
    config.pid = static_cast<quint16>(controlBufferIn[3] << 8 | controlBufferIn[2]);  // PID corresponds to bytes 2 and 3 (little-endian conversion)
    config.majrel = controlBufferIn[6];                                               // Major release version corresponds to byte 6
    config.minrel = controlBufferIn[7];                                               // Minor release version corresponds to byte 7
    config.maxpow = controlBufferIn[4];                                               // Maximum power consumption corresponds to byte 4
    config.powmode = controlBufferIn[5];                                              // Power mode corresponds to byte 5
    config.trfprio = controlBufferIn[8];                                              // Transfer priority corresponds to byte 8
    return config;
}

// Checks if the device is open
bool CP2130::isOpen() const
{
    return handle_ != nullptr;  // Returns true if the device is open, or false otherwise
}

// Returns true is the OTP ROM of the CP2130 was never written
bool CP2130::isOTPBlank(int &errcnt, QString &errstr) const
{
    return getLockWord(errcnt, errstr) == 0xFFFF;
}

// Returns true is the OTP ROM of the CP2130 is locked
bool CP2130::isOTPLocked(int &errcnt, QString &errstr) const
{
    return ((LWALL & getLockWord(errcnt, errstr)) == 0x0000);  // Note that the reserved bits are ignored
}

// Locks the OTP ROM of the CP2130, preventing further changes
void CP2130::lockOTP(int &errcnt, QString &errstr) const
{
    writeLockWord(0x0000, errcnt, errstr);  // Both lock bytes are set to zero
}

// Issues a reset to the CP2130
void CP2130::reset(int &errcnt, QString &errstr) const
{
    controlTransfer(0x40, 0x10, 0x0000, 0x0000, nullptr, 0, errcnt, errstr);
}

// Enables the chip select of the target channel, disabling any others
void CP2130::selectCS(quint8 channel, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[2] = {
        channel,  // Selected channel
        0x02      // Only the corresponding chip select is enabled, all the others are disabled
    };
    controlTransfer(0x40, 0x25, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Sets the GPIO.1 pin on the CP2130 to a given value
void CP2130::setGPIO1(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        0x00, static_cast<quint8>(value << 4),  // Set the value of GPIO.1 to the intended value
        0x00, 0x10                              // Set the mask so that only GPIO.1 is changed
    };
    controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Sets the GPIO.2 pin on the CP2130 to a given value
void CP2130::setGPIO2(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        0x00, static_cast<quint8>(value << 5),  // Set the value of GPIO.2 to the intended value
        0x00, 0x20                              // Set the mask so that only GPIO.2 is changed
    };
    controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Sets the GPIO.3 pin on the CP2130 to a given value
void CP2130::setGPIO3(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        0x00, static_cast<quint8>(value << 6),  // Set the value of GPIO.3 to the intended value
        0x00, 0x40                              // Set the mask so that only GPIO.3 is changed
    };
    controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Sets the GPIO.4 pin on the CP2130 to a given value
void CP2130::setGPIO4(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        0x00, static_cast<quint8>(value << 7),  // Set the value of GPIO.4 to the intended value
        0x00, 0x80                              // Set the mask so that only GPIO.4 is changed
    };
    controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Sets the GPIO.5 pin on the CP2130 to a given value
void CP2130::setGPIO5(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        static_cast<quint8>(value), 0x00,  // Set the value of GPIO.5 to the intended value
        0x01, 0x00                         // Set the mask so that only GPIO.5 is changed
    };
    controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Sets the GPIO.6 pin on the CP2130 to a given value
void CP2130::setGPIO6(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        static_cast<quint8>(value << 2), 0x00,  // Set the value of GPIO.6 to the intended value
        0x04, 0x00                              // Set the mask so that only GPIO.6 is changed
    };
    controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Sets the GPIO.7 pin on the CP2130 to a given value
void CP2130::setGPIO7(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        static_cast<quint8>(value << 3), 0x00,  // Set the value of GPIO.7 to the intended value
        0x08, 0x00                              // Set the mask so that only GPIO.7 is changed
    };
    controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Sets the GPIO.8 pin on the CP2130 to a given value
void CP2130::setGPIO8(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        static_cast<quint8>(value << 4), 0x00,  // Set the value of GPIO.8 to the intended value
        0x10, 0x00                              // Set the mask so that only GPIO.8 is changed
    };
    controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Sets the GPIO.9 pin on the CP2130 to a given value
void CP2130::setGPIO9(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        static_cast<quint8>(value << 5), 0x00,  // Set the value of GPIO.9 to the intended value
        0x20, 0x00                              // Set the mask so that only GPIO.9 is changed
    };
    controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Sets the GPIO.10 pin on the CP2130 to a given value
void CP2130::setGPIO10(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        static_cast<quint8>(value << 6), 0x00,  // Set the value of GPIO.10 to the intended value
        0x40, 0x00                              // Set the mask so that only GPIO.10 is changed
    };
    controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// This procedure is used to lock fields in the CP2130 OTP ROM - Use with care!
void CP2130::writeLockWord(quint16 word, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[2] = {
        static_cast<quint8>(word), static_cast<quint8>(word >> 8)  // Sets both lock bytes to the intended value
    };
    controlTransfer(0x40, 0x6F, MEM_KEY, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Writes the manufacturer descriptor to the CP2130 OTP ROM
void CP2130::writeManufacturerDesc(QString manufacturer, int &errcnt, QString &errstr) const
{
    int strsize = manufacturer.size();
    if (strsize > 62) {
        errcnt += 1;
        errstr.append(QObject::tr("In writeManufacturerDesc(): manufacturer descriptor string cannot be longer than 62 characters.\n"));  // Programmer error
    } else {
        int length = 2 * strsize + 2;
        unsigned char controlBufferOut[64];
        controlBufferOut[0] = length;  // USB string descriptor length
        controlBufferOut[1] = 0x03;  // USB string descriptor constant
        quint16 bufsize = static_cast<quint16>(sizeof(controlBufferOut));
        for (int i = 2; i < bufsize - 1; ++i)
        {
            if (i < length) {
                controlBufferOut[i] = static_cast<quint8>(manufacturer[(i - 2) / 2].unicode() >> (i % 2 == 0 ? 0 : 8));  // If index is even, value will correspond to the LSB of the UTF-16 character, otherwise it will correspond to the MSB of the same
            } else {
                controlBufferOut[i] = 0x00;
            }
        }
        controlBufferOut[bufsize - 1] = 0x00;  // The last byte of the first table is reserved, so it should be set to zero
        controlTransfer(0x40, 0x63, MEM_KEY, 0x0000, controlBufferOut, bufsize, errcnt, errstr);
        for (int i = 0; i < bufsize; ++i)
        {
            if (i < length - 63) {
                controlBufferOut[i] = static_cast<quint8>(manufacturer[(i + 61) / 2].unicode() >> (i % 2 == 0 ? 8 : 0));  // If index is even, value will correspond to the MSB of the UTF-16 character, otherwise it will correspond to the LSB of the same
            } else {
                controlBufferOut[i] = 0x00;  // Note that, inherently, the last byte of the second table will always be set to zero
            }
        }
        controlTransfer(0x40, 0x65, MEM_KEY, 0x0000, controlBufferOut, bufsize, errcnt, errstr);
    }
}

// Writes the pin configuration to the CP2130 OTP ROM
void CP2130::writePinConfig(PinConfig config, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[20] = {
        config.gpio0,                                                                       // GPIO.0 pin config
        config.gpio1,                                                                       // GPIO.1 pin config
        config.gpio2,                                                                       // GPIO.2 pin config
        config.gpio3,                                                                       // GPIO.3 pin config
        config.gpio4,                                                                       // GPIO.4 pin config
        config.gpio5,                                                                       // GPIO.5 pin config
        config.gpio6,                                                                       // GPIO.6 pin config
        config.gpio7,                                                                       // GPIO.7 pin config
        config.gpio8,                                                                       // GPIO.8 pin config
        config.gpio9,                                                                       // GPIO.9 pin config
        config.gpio10,                                                                      // GPIO.10 pin config
        static_cast<quint8>(config.sspndlvl >> 8), static_cast<quint8>(config.sspndlvl),    // Suspend pin level bitmap
        static_cast<quint8>(config.sspndmode >> 8), static_cast<quint8>(config.sspndmode),  // Suspend pin mode bitmap
        static_cast<quint8>(config.wkupmask >> 8), static_cast<quint8>(config.wkupmask),    // Wakeup pin mask bitmap
        static_cast<quint8>(config.wkupmatch >> 8), static_cast<quint8>(config.wkupmatch),  // Wakeup pin match bitmap
        config.divider                                                                      // Clock divider
    };
    controlTransfer(0x40, 0x6D, MEM_KEY, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Writes the product descriptor to the CP2130 OTP ROM
void CP2130::writeProductDesc(QString product, int &errcnt, QString &errstr) const
{
    int strsize = product.size();
    if (strsize > 62) {
        errcnt += 1;
        errstr.append(QObject::tr("In writeProductDesc(): product descriptor string cannot be longer than 62 characters.\n"));  // Programmer error
    } else {
        int length = 2 * strsize + 2;
        unsigned char controlBufferOut[64];
        controlBufferOut[0] = length;  // USB string descriptor length
        controlBufferOut[1] = 0x03;  // USB string descriptor constant
        quint16 bufsize = static_cast<quint16>(sizeof(controlBufferOut));
        for (int i = 2; i < bufsize - 1; ++i)
        {
            if (i < length) {
                controlBufferOut[i] = static_cast<quint8>(product[(i - 2) / 2].unicode() >> (i % 2 == 0 ? 0 : 8));  // If index is even, value will correspond to the LSB of the UTF-16 character, otherwise it will correspond to the MSB of the same
            } else {
                controlBufferOut[i] = 0x00;
            }
        }
        controlBufferOut[bufsize - 1] = 0x00;  // The last byte of the first table is reserved, so it should be set to zero
        controlTransfer(0x40, 0x67, MEM_KEY, 0x0000, controlBufferOut, bufsize, errcnt, errstr);
        for (int i = 0; i < bufsize; ++i)
        {
            if (i < length - 63) {
                controlBufferOut[i] = static_cast<quint8>(product[(i + 61) / 2].unicode() >> (i % 2 == 0 ? 8 : 0));  // If index is even, value will correspond to the MSB of the UTF-16 character, otherwise it will correspond to the LSB of the same
            } else {
                controlBufferOut[i] = 0x00;  // Note that, inherently, the last byte of the second table will always be set to zero
            }
        }
        controlTransfer(0x40, 0x69, MEM_KEY, 0x0000, controlBufferOut, bufsize, errcnt, errstr);
    }
}

// Writes the serial descriptor to the CP2130 OTP ROM
void CP2130::writeSerialDesc(QString serial, int &errcnt, QString &errstr) const
{
    int strsize = serial.size();
    if (strsize > 30) {
        errcnt += 1;
        errstr.append(QObject::tr("In writeSerialDesc(): serial descriptor string cannot be longer than 30 characters.\n"));  // Programmer error
    } else {
        unsigned char controlBufferOut[64];
        controlBufferOut[0] = 2 * strsize + 2;  // USB string descriptor length
        controlBufferOut[1] = 0x03;  // USB string descriptor constant
        quint16 bufsize = static_cast<quint16>(sizeof(controlBufferOut));
        for (int i = 2; i < bufsize; ++i)
        {
            if (i < controlBufferOut[0]) {  // If index is lesser than the USB descriptor length
                controlBufferOut[i] = static_cast<quint8>(serial[(i - 2) / 2].unicode() >> (i % 2 == 0 ? 0 : 8));  // If index is even, value will correspond to the LSB of the UTF-16 character, otherwise it will correspond to the MSB of the same
            } else {
                controlBufferOut[i] = 0x00;  // Note that, inherently, the last two bytes will always be set to zero
            }
        }
        controlTransfer(0x40, 0x6B, MEM_KEY, 0x0000, controlBufferOut, bufsize, errcnt, errstr);
    }
}

// Writes the USB configuration to the CP2130 OTP ROM
void CP2130::writeUSBConfig(USBConfig config, quint8 mask, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[10] = {
        static_cast<quint8>(config.vid), static_cast<quint8>(config.vid >> 8),  // VID
        static_cast<quint8>(config.pid), static_cast<quint8>(config.pid >> 8),  // PID
        config.maxpow,                                                          // Maximum consumption current
        config.powmode,                                                         // Power mode
        config.majrel, config.minrel,                                           // Major and minor release versions
        config.trfprio,                                                         // Transfer priority
        mask                                                                    // Write mask (can be obtained using the return value of getLockWord(), after being bitwise ANDed with "LWUSBCFG" [0x009F] and the resulting value cast to quint8)
    };
    controlTransfer(0x40, 0x61, MEM_KEY, 0x0000, controlBufferOut, static_cast<quint16>(sizeof(controlBufferOut)), errcnt, errstr);
}

// Closes the device safely, if open
void CP2130::close()
{
    if (isOpen()) {  // This condition avoids a segmentation fault if the calling algorithm tries, for some reason, to close the same device twice (e.g., if the device is already closed when the destructor is called)
        libusb_release_interface(handle_, 0);  // Release the interface
        if (kernelAttached_) {  // If a kernel driver was attached to the interface before
            libusb_attach_kernel_driver(handle_, 0);  // Reattach the kernel driver
        }
        libusb_close(handle_);  // Close the device
        libusb_exit(context_);  // Deinitialize libusb
        handle_ = nullptr;  // Required to mark the device as closed
    }
}

// Opens the device having the given serial number, and assigns its handle
int CP2130::open(quint16 vid, quint16 pid, const QString &serial)
{
    int retval = 0;
    if (!isOpen()) {  // Just in case the calling algorithm tries to open a device that was already sucessfully open, or tries to open different devices concurrently, all while using (or referencing to) the same object
        if (libusb_init(&context_) != 0) {  // Initialize libusb. In case of failure
            retval = 1;
        } else {  // If libusb is initialized
            handle_ = libusb_open_device_with_vid_pid_serial(context_, vid, pid, reinterpret_cast<unsigned char *>(serial.toLocal8Bit().data()));
            if (handle_ == nullptr) {  // If the previous operation fails to get a device handle
                libusb_exit(context_);  // Deinitialize libusb
                retval = 2;
            } else {  // If the device is successfully opened and a handle obtained
                if (libusb_kernel_driver_active(handle_, 0) != 0) {  // If a kernel driver is active on the interface
                    libusb_detach_kernel_driver(handle_, 0);  // Detach the kernel driver
                    kernelAttached_ = true;  // Flag that the kernel driver was attached
                } else {
                    kernelAttached_ = false;  // The kernel driver was not attached
                }
                if (libusb_claim_interface(handle_, 0) != 0) {  // Claim the interface. In case of failure
                    if (kernelAttached_) {  // If a kernel driver was attached to the interface before
                        libusb_attach_kernel_driver(handle_, 0);  // Reattach the kernel driver
                    }
                    libusb_close(handle_);  // Close the device
                    libusb_exit(context_);  // Deinitialize libusb
                    handle_ = nullptr;  // Required to mark the device as closed
                    retval = 3;
                }
            }
        }
    }
    return retval;
}

// Helper function to list devices
QStringList CP2130::listDevices(quint16 vid, quint16 pid, int &errcnt, QString &errstr)
{
    QStringList devices;
    libusb_context *context;
    if (libusb_init(&context) != 0) {  // Initialize libusb. In case of failure
        errcnt += 1;
        errstr.append(QObject::tr("Could not initialize libusb.\n"));
    } else {  // If libusb is initialized
        libusb_device **devs;
        ssize_t devlist = libusb_get_device_list(context, &devs);  // Get a device list
        if (devlist < 0) {  // If the previous operation fails to get a device list
            errcnt += 1;
            errstr.append(QObject::tr("Failed to retrieve a list of devices.\n"));
        } else {
            for (ssize_t i = 0; i < devlist; ++i) {  // Run through all listed devices
                struct libusb_device_descriptor desc;
                if (libusb_get_device_descriptor(devs[i], &desc) == 0 && desc.idVendor == vid && desc.idProduct == pid) {  // If the device descriptor is retrieved, and both VID and PID correspond to the ITUSB2 USB Test Switch
                    libusb_device_handle *handle;
                    if (libusb_open(devs[i], &handle) == 0) {  // Open the listed device. If successfull
                        unsigned char str_desc[256];
                        libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, str_desc, static_cast<int>(sizeof(str_desc)));  // Get the serial number string in ASCII format
                        QString serial;
                        devices.append(serial.fromLocal8Bit(reinterpret_cast<char *>(str_desc)));  // Append the serial number string to the list
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
