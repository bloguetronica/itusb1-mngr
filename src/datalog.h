/* ITUSB1 Manager - Version 3.4 for Debian Linux
   Copyright (c) 2020-2022 Samuel Lourenço

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   with this program.  If not, see <https://www.gnu.org/licenses/>.


   Please feel free to contact me via e-mail: samuel.fmlourenco@gmail.com */


#ifndef DATALOG_H
#define DATALOG_H

// Includes
#include <QString>
#include <QVector>
#include "datapoint.h"

class DataLog
{
private:
    QVector<DataPoint> dataPoints_;
    bool newData_;

public:
    DataLog();

    bool hasNewData() const;
    bool isEmpty() const;
    size_t size() const;
    QString toCSVString() const;

    void append(const DataPoint &datapt);
    void clear();
    void noNewData();
};

#endif  // DATALOG_H
