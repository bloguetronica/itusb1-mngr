/* ITUSB1 device class for Qt - Version 2.0
   Copyright (c) 2020 Samuel Lourenço

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


#ifndef ITUSB1DEVICE_H
#define ITUSB1DEVICE_H

// Includes
#include <QStringList>  // Also includes QString
#include <libusb-1.0/libusb.h>

// Definitions
const int CFRQ1500K = 0x03;  // Value corresponding to a clock frequency of 1.5MHz, applicable to configureSPIMode()
const bool CPOL0 = false;    // Boolean corresponding to CPOL = 0, applicable to configureSPIMode()
const bool CPHA0 = false;    // Boolean corresponding to CPHA = 0, applicable to configureSPIMode()
const bool CSMODEPP = true;  // Boolean corresponding to chip select push-pull mode, applicable to configureSPIMode()

class ITUSB1Device
{
private:
    libusb_context *context_;
    libusb_device_handle *handle_;
    bool deviceOpen_, kernelAttached_;

public:
    ITUSB1Device();
    ~ITUSB1Device();

    void configureSPIMode(uint8_t channel, bool csmode, uint8_t cfrq, bool cpol, bool cpha, int &errcnt, QString &errstr) const;
    void disableCS(uint8_t channel, int &errcnt, QString &errstr) const;
    void disableSPIDelays(uint8_t channel, int &errcnt, QString &errstr) const;
    uint16_t getCurrent(int &errcnt, QString &errstr) const;
    bool getGPIO1(int &errcnt, QString &errstr) const;
    bool getGPIO2(int &errcnt, QString &errstr) const;
    bool getGPIO3(int &errcnt, QString &errstr) const;
    uint8_t getMajorRelease(int &errcnt, QString &errstr) const;
    QString getManufacturer(int &errcnt, QString &errstr) const;
    uint8_t getMaxPower(int &errcnt, QString &errstr) const;
    uint8_t getMinorRelease(int &errcnt, QString &errstr) const;
    QString getProduct(int &errcnt, QString &errstr) const;
    QString getSerial(int &errcnt, QString &errstr) const;
    bool isOpen() const;
    void reset(int &errcnt, QString &errstr) const;
    void selectCS(uint8_t channel, int &errcnt, QString &errstr) const;
    void setGPIO1(bool value, int &errcnt, QString &errstr) const;
    void setGPIO2(bool value, int &errcnt, QString &errstr) const;

    void close();
    int open(const QString &serial);
};

QStringList listDevices(int &errcnt, QString &errstr);

#endif // ITUSB1DEVICE_H
