/* ITUSB1 Manager - Version 3.1 for Debian Linux
   Copyright (c) 2020-2021 Samuel Lourenço

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
#include "metrics.h"

Metrics::Metrics() :
    avg_(),
    min_(1023.75),
    max_(0),
    last_(0),
    nmeas_(0)
{
}

// Returns the average value
float Metrics::average() const
{
    return static_cast<float>(avg_);
}

// Returns the last input value
float Metrics::last() const
{
    return last_;
}

// Returns the maximum value
float Metrics::maximum() const
{
    return max_;
}

// Returns the minimum value
float Metrics::minimum() const
{
    return min_;
}

// Returns the number of measurements
size_t Metrics::numberOfMeasurements() const
{
    return nmeas_;
}

// Clears metrics (corresponds to clearValues() in version 2.0, or cleanValues() in version 1.0, originally located in devicewindow.cpp)
void Metrics::clear()
{
    nmeas_ = 0;  // Clearing nmeas_ (number of measurements) also clears avg_ (the average value)
    min_ = 1023.75;
    max_ = 0;
    last_ = 0;
}

// Updates metrics
void Metrics::update(float value)
{
    ++nmeas_;
    avg_ = (value + avg_ * (nmeas_ - 1)) / nmeas_;  // Calculate the recursive average of all accumulated data points
    if (value < min_) {  // This condition doesn't exclude the next!
        min_ = value;
    }
    if (value > max_) {  // This condition doesn't exclude the previous!
        max_ = value;
    }
    last_ = value;
}
