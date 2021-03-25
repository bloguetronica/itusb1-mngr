/* ITUSB1 device class for Qt - Version 3.0
   Copyright (c) 2020-2021 Samuel Lourenço

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
#include <QObject>  // Added translator support in version 3.0
#include "itusb1device.h"
extern "C" {
#include "libusb-extra.h"
}

// Definitions
const uint16_t VID = 0x10C4;  // USB vendor ID
const uint16_t PID = 0x8C96;  // USB product ID
const unsigned int TR_TIMEOUT = 100;  // Transfer timeout in milliseconds

ITUSB1Device::ITUSB1Device() :
    context_(nullptr),
    handle_(nullptr),
    deviceOpen_(false),
    kernelAttached_(false)
{
}

ITUSB1Device::~ITUSB1Device()
{
    close();  // The destructor is used to close the device, and this is essential so the device can be freed when the parent object is destroyed
}

// Configures the given SPI channel in respect to its chip select mode, clock frequency, polarity and phase
void ITUSB1Device::configureSPIMode(uint8_t channel, const SPIMode &mode, int &errcnt, QString &errstr) const
{
    unsigned char control_buf_out[2] = {
        channel,                                                                                       // Selected channel
        static_cast<uint8_t>(mode.cpha << 5 | mode.cpol << 4 | mode.csmode << 3 | (0x07 & mode.cfrq))  // Control word (specified chip select mode, clock frequency, polarity and phase)
    };
    if (libusb_control_transfer(handle_, 0x40, 0x31, 0x0000, 0x0000, control_buf_out, sizeof(control_buf_out), TR_TIMEOUT) != sizeof(control_buf_out)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x31).\n"));
    }
}

// Disables the chip select corresponding to the target channel
void ITUSB1Device::disableCS(uint8_t channel, int &errcnt, QString &errstr) const
{
    unsigned char control_buf_out[2] = {
        channel,  // Selected channel
        0x00      // Corresponding chip select disabled
    };
    if (libusb_control_transfer(handle_, 0x40, 0x25, 0x0000, 0x0000, control_buf_out, sizeof(control_buf_out), TR_TIMEOUT) != sizeof(control_buf_out)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x25).\n"));
    }
}

// Disables all SPI delays for a given channel
void ITUSB1Device::disableSPIDelays(uint8_t channel, int &errcnt, QString &errstr) const
{
    unsigned char control_buf_out[8] = {
        channel,     // Selected channel
        0x00,        // All SPI delays disabled, no CS toggle
        0x00, 0x00,  // Inter-byte,
        0x00, 0x00,  // post-assert and
        0x00, 0x00   // pre-deassert delays all set to 0us
    };
    if (libusb_control_transfer(handle_, 0x40, 0x33, 0x0000, 0x0000, control_buf_out, sizeof(control_buf_out), TR_TIMEOUT) != sizeof(control_buf_out)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x33).\n"));
    }
}

// Gets the raw value, corresponding to the measured current, from the LTC2312 ADC
uint16_t ITUSB1Device::getCurrent(int &errcnt, QString &errstr) const
{
    unsigned char read_command_buf[8] = {
        0x00, 0x00,             // Reserved
        0x00,                   // Read command
        0x00,                   // Reserved
        0x02, 0x00, 0x00, 0x00  // Two bytes to read
    };
    unsigned char read_input_buf[2];
    int bytes_read, bytes_written;
    if (libusb_bulk_transfer(handle_, 0x01, read_command_buf, sizeof(read_command_buf), &bytes_written, TR_TIMEOUT) != 0) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed bulk OUT transfer to endpoint 1 (address 0x01).\n"));
    } else if (libusb_bulk_transfer(handle_, 0x82, read_input_buf, sizeof(read_input_buf), &bytes_read, TR_TIMEOUT) != 0) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed bulk IN transfer from endpoint 2 (address 0x82).\n"));
    }
    return static_cast<uint16_t>(read_input_buf[0] << 4 | read_input_buf[1] >> 4);
}

// Gets the current value of the GPIO.1 pin on the CP2130
bool ITUSB1Device::getGPIO1(int &errcnt, QString &errstr) const
{
    unsigned char control_buf_in[2];
    if (libusb_control_transfer(handle_, 0xC0, 0x20, 0x0000, 0x0000, control_buf_in, sizeof(control_buf_in), TR_TIMEOUT) != sizeof(control_buf_in)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x20).\n"));
    }
    return ((0x10 & control_buf_in[1]) != 0x00);  // Returns one if bit 4 of byte 1, which corresponds to the GPIO.1 pin, is not set to zero
}

// Gets the current value of the GPIO.2 pin on the CP2130
bool ITUSB1Device::getGPIO2(int &errcnt, QString &errstr) const
{
    unsigned char control_buf_in[2];
    if (libusb_control_transfer(handle_, 0xC0, 0x20, 0x0000, 0x0000, control_buf_in, sizeof(control_buf_in), TR_TIMEOUT) != sizeof(control_buf_in)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x20).\n"));
    }
    return ((0x20 & control_buf_in[1]) != 0x00);  // Returns one if bit 5 of byte 1, which corresponds to the GPIO.2 pin, is not set to zero
}

// Gets the current value of the GPIO.3 pin on the CP2130
bool ITUSB1Device::getGPIO3(int &errcnt, QString &errstr) const
{
    unsigned char control_buf_in[2];
    if (libusb_control_transfer(handle_, 0xC0, 0x20, 0x0000, 0x0000, control_buf_in, sizeof(control_buf_in), TR_TIMEOUT) != sizeof(control_buf_in)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x20).\n"));
    }
    return ((0x40 & control_buf_in[1]) != 0x00);  // Returns one if bit 6 of byte 1, which corresponds to the GPIO.3 pin, is not set to zero
}

// Gets the major release version from the CP2130
uint8_t ITUSB1Device::getMajorRelease(int &errcnt, QString &errstr) const
{
    unsigned char control_buf_in[9];
    if (libusb_control_transfer(handle_, 0xC0, 0x60, 0x0000, 0x0000, control_buf_in, sizeof(control_buf_in), TR_TIMEOUT) != sizeof(control_buf_in)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x60).\n"));
    }
    return control_buf_in[6];
}

// Gets the manufacturer descriptor from the CP2130
QString ITUSB1Device::getManufacturer(int &errcnt, QString &errstr) const
{
    unsigned char control_buf_in[64];
    if (libusb_control_transfer(handle_, 0xC0, 0x62, 0x0000, 0x0000, control_buf_in, sizeof(control_buf_in), TR_TIMEOUT) != sizeof(control_buf_in)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x62).\n"));
    }
    QString manufacturer;
    int end = control_buf_in[0] > 62 ? 62 : control_buf_in[0];
    for (int i = 2; i < end; i += 2) {  // Descriptor length is limited to 30 characters, or 60 bytes
        if (control_buf_in[i] != 0 || control_buf_in[i + 1] != 0) {  // Filter out null characters
            manufacturer.append(QChar(control_buf_in[i + 1] << 8 | control_buf_in[i]));  // UTF-16LE conversion as per the USB 2.0 specification
        }
    }
    return manufacturer;
}

// Gets the maximum power descriptor from the CP2130
uint8_t ITUSB1Device::getMaxPower(int &errcnt, QString &errstr) const
{
    unsigned char control_buf_in[9];
    if (libusb_control_transfer(handle_, 0xC0, 0x60, 0x0000, 0x0000, control_buf_in, sizeof(control_buf_in), TR_TIMEOUT) != sizeof(control_buf_in)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x60).\n"));
    }
    return control_buf_in[4];
}

// Gets the minor release version from the CP2130
uint8_t ITUSB1Device::getMinorRelease(int &errcnt, QString &errstr) const
{
    unsigned char control_buf_in[9];
    if (libusb_control_transfer(handle_, 0xC0, 0x60, 0x0000, 0x0000, control_buf_in, sizeof(control_buf_in), TR_TIMEOUT) != sizeof(control_buf_in)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x60).\n"));
    }
    return control_buf_in[7];
}

// Gets the product descriptor from the CP2130
QString ITUSB1Device::getProduct(int &errcnt, QString &errstr) const
{
    unsigned char control_buf_in[64];
    if (libusb_control_transfer(handle_, 0xC0, 0x66, 0x0000, 0x0000, control_buf_in, sizeof(control_buf_in), TR_TIMEOUT) != sizeof(control_buf_in)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x66).\n"));
    }
    QString product;
    int end = control_buf_in[0] > 62 ? 62 : control_buf_in[0];
    for (int i = 2; i < end; i += 2) {  // Descriptor length is limited to 30 characters, or 60 bytes
        if (control_buf_in[i] != 0 || control_buf_in[i + 1] != 0) {  // Filter out null characters
            product.append(QChar(control_buf_in[i + 1] << 8 | control_buf_in[i]));  // UTF-16LE conversion as per the USB 2.0 specification
        }
    }
    return product;
}

// Gets the serial descriptor from the CP2130
QString ITUSB1Device::getSerial(int &errcnt, QString &errstr) const
{
    unsigned char control_buf_in[64];
    if (libusb_control_transfer(handle_, 0xC0, 0x6A, 0x0000, 0x0000, control_buf_in, sizeof(control_buf_in), TR_TIMEOUT) != sizeof(control_buf_in)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0xC0, 0x6A).\n"));
    }
    QString serial;
    for (int i = 2; i < control_buf_in[0]; i += 2) {
        if (control_buf_in[i] != 0 || control_buf_in[i + 1] != 0) {  // Filter out null characters
            serial.append(QChar(control_buf_in[i + 1] << 8 | control_buf_in[i]));  // UTF-16LE conversion as per the USB 2.0 specification
        }
    }
    return serial;
}

// Checks if the device is open
bool ITUSB1Device::isOpen() const
{
    return deviceOpen_;  // Returns true if the device is open, or false otherwise
}

void ITUSB1Device::reset(int &errcnt, QString &errstr) const  // Issues a reset to the CP2130, which in effect resets the entire device
{
    if (libusb_control_transfer(handle_, 0x40, 0x10, 0x0000, 0x0000, nullptr, 0, TR_TIMEOUT) != 0) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x10).\n"));
    }
}

// Enables the chip select of the target channel, disabling any others
void ITUSB1Device::selectCS(uint8_t channel, int &errcnt, QString &errstr) const
{
    unsigned char control_buf_out[2] = {
        channel,  // Selected channel
        0x02      // Only the corresponding chip select is enabled, all the others are disabled
    };
    if (libusb_control_transfer(handle_, 0x40, 0x25, 0x0000, 0x0000, control_buf_out, sizeof(control_buf_out), TR_TIMEOUT) != sizeof(control_buf_out)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x25).\n"));
    }
}

// Sets the GPIO.1 pin on the CP2130 to a given value
void ITUSB1Device::setGPIO1(bool value, int &errcnt, QString &errstr) const
{
    unsigned char control_buf_out[4] = {
        0x00, static_cast<uint8_t>(value << 4),  // Set the value of GPIO.1 to the intended value
        0x00, 0x10                               // Set the mask so that only GPIO.1 is changed
    };
    if (libusb_control_transfer(handle_, 0x40, 0x21, 0x0000, 0x0000, control_buf_out, sizeof(control_buf_out), TR_TIMEOUT) != sizeof(control_buf_out)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x21).\n"));
    }
}

// Sets the GPIO.2 pin on the CP2130 to a given value
void ITUSB1Device::setGPIO2(bool value, int &errcnt, QString &errstr) const
{
    unsigned char control_buf_out[4] = {
        0x00, static_cast<uint8_t>(value << 5),  // Set the value of GPIO.2 to the intended value
        0x00, 0x20                               // Set the mask so that only GPIO.2 is changed
    };
    if (libusb_control_transfer(handle_, 0x40, 0x21, 0x0000, 0x0000, control_buf_out, sizeof(control_buf_out), TR_TIMEOUT) != sizeof(control_buf_out)) {
        errcnt += 1;
        errstr.append(QObject::tr("Failed control transfer (0x40, 0x21).\n"));
    }
}

// Closes the device safely, if open
void ITUSB1Device::close()
{
    if (deviceOpen_) {  // This condition avoids a segmentation fault if the calling algorithm tries, for some reason, to close the same device twice (e.g., if the device is already closed when the destructor is called)
        libusb_release_interface(handle_, 0);  // Release the interface
        if (kernelAttached_) {  // If a kernel driver was attached to the interface before
            libusb_attach_kernel_driver(handle_, 0);  // Reattach the kernel driver
            // No need to flag the kernel driver as detached here (since version 3.0)
        }
        libusb_close(handle_);  // Close the device
        libusb_exit(context_);  // Deinitialize libusb
        deviceOpen_ = false;  // Flag the device as closed
    }
}

// Opens the device having the given serial number, and assigns its handle
int ITUSB1Device::open(const QString &serial)
{
    int retval = 0;
    if (!deviceOpen_) {  // Just in case the calling algorithm tries to open a device that was already sucessfully open, or tries to open different devices concurrently, all while using (or referencing to) the same object
        if (libusb_init(&context_) != 0) {  // Initialize libusb. In case of failure
            retval = 1;
        } else {  // If libusb is initialized
            handle_ = libusb_open_device_with_vid_pid_serial(context_, VID, PID, reinterpret_cast<unsigned char *>(serial.toLocal8Bit().data()));
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
                        // No need to flag the kernel driver as detached here (since version 3.0)
                    }
                    libusb_close(handle_);  // Close the device
                    libusb_exit(context_);  // Deinitialize libusb
                    retval = 3;
                } else {
                    deviceOpen_ = true;  // Flag the device as open
                }
            }
        }
    }
    return retval;
}

// Helper function to list devices (static member since version 3.0)
QStringList ITUSB1Device::listDevices(int &errcnt, QString &errstr)
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
                if (libusb_get_device_descriptor(devs[i], &desc) == 0 && desc.idVendor == VID && desc.idProduct == PID) {  // If the device descriptor is retrieved, and both VID and PID correspond to the ITUSB1 Power Supply
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
