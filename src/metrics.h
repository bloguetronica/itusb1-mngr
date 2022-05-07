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


#ifndef METRICS_H
#define METRICS_H

// Includes
#include <cstddef>

class Metrics
{
private:
    double avg_;
    float min_, max_, last_;
    size_t nmeas_;

public:
    Metrics();

    float average() const;
    float last() const;
    float maximum() const;
    float minimum() const;
    size_t numberOfMeasurements() const;

    void clear();
    void update(float value);
};

#endif  // METRICS_H
