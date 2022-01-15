/* ITUSB1 Manager - Version 3.3 for Debian Linux
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


// Includes
#include "datalog.h"

DataLog::DataLog() :
    dataPoints_(),
    newData_(false)
{
}

// Returns true if log has new data to be processed
bool DataLog::hasNewData() const
{
    return newData_;
}

// Returns true if log is empty
bool DataLog::isEmpty() const
{
    return dataPoints_.isEmpty();
}

// Returns the number of elements
size_t DataLog::size() const
{
    return static_cast<size_t>(dataPoints_.size());  // QVector::size() returns an int, but should return a size_t - Future Qt versions may cover this!
}

// Returns a CSV formatted string
QString DataLog::toCSVString() const
{
    QString csvstr = "Time (s),Current (mA),USB power,USB data,OC flag\n";
    for (DataPoint datapt : dataPoints_) {
        csvstr += QString("%1,").arg(datapt.time, 0, 'f', 3);
        csvstr += QString("%1,").arg(datapt.curr, 0, 'f', 1);
        csvstr += QString("%1,").arg(datapt.up);
        csvstr += QString("%1,").arg(datapt.ud);
        csvstr += QString("%1\n").arg(datapt.oc);
    }
    return csvstr;
}

// Append new data point
void DataLog::append(const DataPoint &datapt)
{
    dataPoints_ += datapt;
    newData_ = true;
}

// Empties the log
void DataLog::clear()
{
    dataPoints_.clear();
    dataPoints_.squeeze();  // This action releases memory that is no longer required
    newData_ = false;
}

// Used to flag the log as processed
void DataLog::noNewData()
{
    newData_ = false;
}
