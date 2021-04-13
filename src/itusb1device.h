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


#ifndef ITUSB1DEVICE_H
#define ITUSB1DEVICE_H

// Includes
#include <QStringList>  // Also includes QString
#include "cp2130.h"

class ITUSB1Device
{
private:
    CP2130 cp2130_;

    quint16 getRawCurrent(int &errcnt, QString &errstr) const;

public:
    ITUSB1Device();
    ~ITUSB1Device();

    void attach(int &errcnt, QString &errstr) const;
    void detach(int &errcnt, QString &errstr) const;
    float getCurrent(int &errcnt, QString &errstr) const;
    quint8 getMajorRelease(int &errcnt, QString &errstr) const;
    QString getManufacturer(int &errcnt, QString &errstr) const;
    quint8 getMaxPower(int &errcnt, QString &errstr) const;
    quint8 getMinorRelease(int &errcnt, QString &errstr) const;
    bool getOverCurrentStatus(int &errcnt, QString &errstr) const;
    QString getProduct(int &errcnt, QString &errstr) const;
    QString getSerial(int &errcnt, QString &errstr) const;
    bool getUSBDataStatus(int &errcnt, QString &errstr) const;
    bool getUSBPowerStatus(int &errcnt, QString &errstr) const;
    bool isOpen() const;
    void reset(int &errcnt, QString &errstr) const;
    void setup(int &errcnt, QString &errstr) const;
    void switchUSBData(bool value, int &errcnt, QString &errstr) const;
    void switchUSBPower(bool value, int &errcnt, QString &errstr) const;

    void close();
    int open(const QString &serial);

    static QStringList listDevices(int &errcnt, QString &errstr);
};

#endif // ITUSB1DEVICE_H
