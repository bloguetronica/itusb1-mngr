/* CP2130 class for Qt - Version 0.3.1 for Debian Linux
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
int CP2130::bulkTransfer(quint8 endpoint, unsigned char *data, int length, int *transferred, int &errcnt, QString &errstr) const
{
    int retval;
    if (!isOpen()) {
        errcnt += 1;
        errstr.append(QObject::tr("In bulkTransfer(): device is not open.\n"));  // Programmer error
        retval = LIBUSB_ERROR_NO_DEVICE;  // This is the most appropriate error, although the return value should be discarded
    } else {
        retval = libusb_bulk_transfer(handle_, endpoint, data, length, transferred, TR_TIMEOUT);
    }
    return retval;
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
    quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
    if (controlTransfer(0x40, 0x33, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x33).\n"));
    }
}

// Configures the given SPI channel in respect to its chip select mode, clock frequency, polarity and phase
void CP2130::configureSPIMode(quint8 channel, const SPIMode &mode, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[2] = {
        channel,                                                                                      // Selected channel
        static_cast<quint8>(mode.cpha << 5 | mode.cpol << 4 | mode.csmode << 3 | (0x07 & mode.cfrq))  // Control word (specified chip select mode, clock frequency, polarity and phase)
    };
    quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
    if (controlTransfer(0x40, 0x31, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x31).\n"));
    }
}

// Safe control transfer
int CP2130::controlTransfer(quint8 bmRequestType, quint8 bRequest, quint16 wValue, quint16 wIndex, unsigned char *data, quint16 wLength, int &errcnt, QString &errstr) const
{
    int retval;
    if (!isOpen()) {
        errcnt += 1;
        errstr.append(QObject::tr("In controlTransfer(): device is not open.\n"));  // Programmer error
        retval = LIBUSB_ERROR_NO_DEVICE;  // This is the most appropriate error, although the return value should be discarded
    } else {
        retval = libusb_control_transfer(handle_, bmRequestType, bRequest, wValue, wIndex, data, wLength, TR_TIMEOUT);
    }
    return retval;
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
        quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
        if (controlTransfer(0x40, 0x25, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
            errcnt += 1;
            errstr.append(QObject::tr("Failed control transfer (0x40, 0x25).\n"));
        }
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
        quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
        if (controlTransfer(0x40, 0x33, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
            errcnt += 1;
            errstr.append(QObject::tr("Failed control transfer (0x40, 0x33).\n"));
        }
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
        quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
        if (controlTransfer(0x40, 0x25, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
            errcnt += 1;
            errstr.append(QObject::tr("Failed control transfer (0x40, 0x25).\n"));
        }
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
        quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
        if (controlTransfer(0xC0, 0x24, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
            errcnt += 1;
            errstr.append(QObject::tr("Failed control transfer (0xC0, 0x24).\n"));
        }
        retval = ((0x01 << channel & (controlBufferIn[0] << 8 | controlBufferIn[1])) != 0x00);
    }
    return retval;
}

// Returns the current value of the GPIO.1 pin on the CP2130
bool CP2130::getGPIO1(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x20).\n"));
    }
    return ((0x10 & controlBufferIn[1]) != 0x00);  // Returns one if bit 4 of byte 1, which corresponds to the GPIO.1 pin, is not set to zero
}

// Returns the current value of the GPIO.2 pin on the CP2130
bool CP2130::getGPIO2(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x20).\n"));
    }
    return ((0x20 & controlBufferIn[1]) != 0x00);  // Returns one if bit 5 of byte 1, which corresponds to the GPIO.2 pin, is not set to zero
}

// Returns the current value of the GPIO.3 pin on the CP2130
bool CP2130::getGPIO3(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x20).\n"));
    }
    return ((0x40 & controlBufferIn[1]) != 0x00);  // Returns one if bit 6 of byte 1, which corresponds to the GPIO.3 pin, is not set to zero
}

// Returns the current value of the GPIO.4 pin on the CP2130
bool CP2130::getGPIO4(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x20).\n"));
    }
    return ((0x80 & controlBufferIn[1]) != 0x00);  // Returns one if bit 7 of byte 1, which corresponds to the GPIO.4 pin, is not set to zero
}

// Returns the current value of the GPIO.5 pin on the CP2130
bool CP2130::getGPIO5(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x20).\n"));
    }
    return ((0x01 & controlBufferIn[0]) != 0x00);  // Returns one if bit 0 of byte 0, which corresponds to the GPIO.5 pin, is not set to zero
}

// Returns the current value of the GPIO.6 pin on the CP2130
bool CP2130::getGPIO6(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x20).\n"));
    }
    return ((0x04 & controlBufferIn[0]) != 0x00);  // Returns one if bit 2 of byte 0, which corresponds to the GPIO.5 pin, is not set to zero
}

// Returns the current value of the GPIO.7 pin on the CP2130
bool CP2130::getGPIO7(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x20).\n"));
    }
    return ((0x08 & controlBufferIn[0]) != 0x00);  // Returns one if bit 3 of byte 0, which corresponds to the GPIO.5 pin, is not set to zero
}

// Returns the current value of the GPIO.8 pin on the CP2130
bool CP2130::getGPIO8(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x20).\n"));
    }
    return ((0x10 & controlBufferIn[0]) != 0x00);  // Returns one if bit 4 of byte 0, which corresponds to the GPIO.5 pin, is not set to zero
}

// Returns the current value of the GPIO.9 pin on the CP2130
bool CP2130::getGPIO9(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x20).\n"));
    }
    return ((0x20 & controlBufferIn[0]) != 0x00);  // Returns one if bit 5 of byte 0, which corresponds to the GPIO.5 pin, is not set to zero
}

// Returns the current value of the GPIO.10 pin on the CP2130
bool CP2130::getGPIO10(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[2];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x20, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x20).\n"));
    }
    return ((0x40 & controlBufferIn[0]) != 0x00);  // Returns one if bit 6 of byte 0, which corresponds to the GPIO.5 pin, is not set to zero
}

// Gets the major release version from the CP2130 OTP ROM
quint8 CP2130::getMajorRelease(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[9];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x60, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x60).\n"));
    }
    return controlBufferIn[6];
}

// Gets the manufacturer descriptor from the CP2130 OTP ROM
QString CP2130::getManufacturer(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[64];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x62, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {  // First table
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x62).\n"));
    }
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
        if (controlTransfer(0xC0, 0x64, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {  // Second table
            errcnt += 1;
            errstr.append(QObject::tr("Failed control transfer (0xC0, 0x64).\n"));
        }
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

// Gets the maximum power descriptor from the CP2130 OTP ROM
quint8 CP2130::getMaxPower(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[9];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x60, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x60).\n"));
    }
    return controlBufferIn[4];
}

// Gets the minor release version from the CP2130 OTP ROM
quint8 CP2130::getMinorRelease(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[9];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x60, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x60).\n"));
    }
    return controlBufferIn[7];
}

// Gets the product descriptor from the CP2130 OTP ROM
QString CP2130::getProduct(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[64];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x66, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {  // First table
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x66).\n"));
    }
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
        if (controlTransfer(0xC0, 0x68, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {  // Second table
            errcnt += 1;
            errstr.append(QObject::tr("Failed control transfer (0xC0, 0x68).\n"));
        }
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
QString CP2130::getSerial(int &errcnt, QString &errstr) const
{
    unsigned char controlBufferIn[64];
    quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
    if (controlTransfer(0xC0, 0x6A, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x6A).\n"));
    }
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
        quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
        if (controlTransfer(0xC0, 0x32, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
            errcnt += 1;
            errstr.append(QObject::tr("Failed control transfer (0xC0, 0x32).\n"));
        }
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
        quint16 size = static_cast<quint16>(sizeof(controlBufferIn));
        if (controlTransfer(0xC0, 0x30, 0x0000, 0x0000, controlBufferIn, size, errcnt, errstr) != size) {
            errcnt += 1;
            errstr.append(QObject::tr("Failed control transfer (0xC0, 0x30).\n"));
        }
        mode.csmode = ((0x08 & controlBufferIn[channel]) != 0x00);  // Chip select mode corresponds to bit 3
        mode.cfrq = 0x07 & controlBufferIn[channel];                // Clock frequency is set in the bits 2:0
        mode.cpha = ((0x20 & controlBufferIn[channel]) != 0x00);    // Clock phase corresponds to bit 5
        mode.cpol = ((0x10 &controlBufferIn[channel]) != 0x00);     // Clock polarity corresponds to bit 4
    }
    return mode;
}

// Issues a reset to the CP2130
void CP2130::reset(int &errcnt, QString &errstr) const
{
    if (controlTransfer(0x40, 0x10, 0x0000, 0x0000, nullptr, 0, errcnt, errstr) != 0) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x10).\n"));
    }
}

// Enables the chip select of the target channel, disabling any others
void CP2130::selectCS(quint8 channel, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[2] = {
        channel,  // Selected channel
        0x02      // Only the corresponding chip select is enabled, all the others are disabled
    };
    quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
    if (controlTransfer(0x40, 0x25, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x25).\n"));
    }
}

// Sets the GPIO.1 pin on the CP2130 to a given value
void CP2130::setGPIO1(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        0x00, static_cast<quint8>(value << 4),  // Set the value of GPIO.1 to the intended value
        0x00, 0x10                              // Set the mask so that only GPIO.1 is changed
    };
    quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
    if (controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x21).\n"));
    }
}

// Sets the GPIO.2 pin on the CP2130 to a given value
void CP2130::setGPIO2(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        0x00, static_cast<quint8>(value << 5),  // Set the value of GPIO.2 to the intended value
        0x00, 0x20                              // Set the mask so that only GPIO.2 is changed
    };
    quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
    if (controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x21).\n"));
    }
}

// Sets the GPIO.3 pin on the CP2130 to a given value
void CP2130::setGPIO3(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        0x00, static_cast<quint8>(value << 6),  // Set the value of GPIO.3 to the intended value
        0x00, 0x40                              // Set the mask so that only GPIO.3 is changed
    };
    quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
    if (controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x21).\n"));
    }
}

// Sets the GPIO.4 pin on the CP2130 to a given value
void CP2130::setGPIO4(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        0x00, static_cast<quint8>(value << 7),  // Set the value of GPIO.4 to the intended value
        0x00, 0x80                              // Set the mask so that only GPIO.4 is changed
    };
    quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
    if (controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x21).\n"));
    }
}

// Sets the GPIO.5 pin on the CP2130 to a given value
void CP2130::setGPIO5(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        static_cast<quint8>(value), 0x00,  // Set the value of GPIO.5 to the intended value
        0x01, 0x00                         // Set the mask so that only GPIO.5 is changed
    };
    quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
    if (controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x21).\n"));
    }
}

// Sets the GPIO.6 pin on the CP2130 to a given value
void CP2130::setGPIO6(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        static_cast<quint8>(value << 2), 0x00,  // Set the value of GPIO.6 to the intended value
        0x04, 0x00                              // Set the mask so that only GPIO.6 is changed
    };
    quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
    if (controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x21).\n"));
    }
}

// Sets the GPIO.7 pin on the CP2130 to a given value
void CP2130::setGPIO7(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        static_cast<quint8>(value << 3), 0x00,  // Set the value of GPIO.7 to the intended value
        0x08, 0x00                              // Set the mask so that only GPIO.7 is changed
    };
    quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
    if (controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x21).\n"));
    }
}

// Sets the GPIO.8 pin on the CP2130 to a given value
void CP2130::setGPIO8(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        static_cast<quint8>(value << 4), 0x00,  // Set the value of GPIO.8 to the intended value
        0x10, 0x00                              // Set the mask so that only GPIO.8 is changed
    };
    quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
    if (controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x21).\n"));
    }
}

// Sets the GPIO.9 pin on the CP2130 to a given value
void CP2130::setGPIO9(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        static_cast<quint8>(value << 5), 0x00,  // Set the value of GPIO.9 to the intended value
        0x20, 0x00                              // Set the mask so that only GPIO.9 is changed
    };
    quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
    if (controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x21).\n"));
    }
}

// Sets the GPIO.10 pin on the CP2130 to a given value
void CP2130::setGPIO10(bool value, int &errcnt, QString &errstr) const
{
    unsigned char controlBufferOut[4] = {
        static_cast<quint8>(value << 6), 0x00,  // Set the value of GPIO.10 to the intended value
        0x40, 0x00                              // Set the mask so that only GPIO.10 is changed
    };
    quint16 size = static_cast<quint16>(sizeof(controlBufferOut));
    if (controlTransfer(0x40, 0x21, 0x0000, 0x0000, controlBufferOut, size, errcnt, errstr) != size) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x21).\n"));
    }
}

// Checks if the device is open
bool CP2130::isOpen() const
{
    return handle_ != nullptr;  // Returns true if the device is open, or false otherwise
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
                        libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, str_desc, sizeof(str_desc));  // Get the serial number string in ASCII format
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
