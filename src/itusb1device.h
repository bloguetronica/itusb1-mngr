/* ITUSB1 device class for Qt - Version 3.0.1
   Requires CP2130 class for Qt version 2.0.0 or later
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


#ifndef ITUSB1DEVICE_H
#define ITUSB1DEVICE_H

// Includes
#include <QString>
#include <QStringList>
#include "cp2130.h"

class ITUSB1Device
{
private:
    CP2130 cp2130_;

    quint16 getRawCurrent(int &errcnt, QString &errstr);

public:
    // Class definitions
    static const quint16 VID = 0x10C4;                           // USB vendor ID
    static const quint16 PID = 0x8C96;                           // USB product ID
    static const int SUCCESS = CP2130::SUCCESS;                  // Returned by open() if successful
    static const int ERROR_INIT = CP2130::ERROR_INIT;            // Returned by open() in case of a libusb initialization failure
    static const int ERROR_NOT_FOUND = CP2130::ERROR_NOT_FOUND;  // Returned by open() if the device was not found
    static const int ERROR_BUSY = CP2130::ERROR_BUSY;            // Returned by open() if the device is already in use

    ITUSB1Device();

    bool disconnected() const;
    bool isOpen() const;

    void attach(int &errcnt, QString &errstr);
    void close();
    void detach(int &errcnt, QString &errstr);
    float getCurrent(int &errcnt, QString &errstr);
    QString getManufacturerDesc(int &errcnt, QString &errstr);
    bool getOvercurrentStatus(int &errcnt, QString &errstr);
    QString getProductDesc(int &errcnt, QString &errstr);
    QString getSerialDesc(int &errcnt, QString &errstr);
    CP2130::USBConfig getUSBConfig(int &errcnt, QString &errstr);
    bool getUSBDataStatus(int &errcnt, QString &errstr);
    bool getUSBPowerStatus(int &errcnt, QString &errstr);
    int open(const QString &serial);
    void reset(int &errcnt, QString &errstr);
    void setup(int &errcnt, QString &errstr);
    void switchUSB(bool value, int &errcnt, QString &errstr);
    void switchUSBData(bool value, int &errcnt, QString &errstr);
    void switchUSBPower(bool value, int &errcnt, QString &errstr);

    static QStringList listDevices(int &errcnt, QString &errstr);
};

#endif  // ITUSB1DEVICE_H
