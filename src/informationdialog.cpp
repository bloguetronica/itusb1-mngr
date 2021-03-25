/* ITUSB1 Manager - Version 2.0 for Debian Linux
   Copyright (c) 2020 Samuel Lourenço

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
#include "informationdialog.h"
#include "ui_informationdialog.h"

InformationDialog::InformationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InformationDialog)
{
    ui->setupUi(this);
}

InformationDialog::~InformationDialog()
{
    delete ui;
}

void InformationDialog::setManufacturerLabelText(const QString &manufacturerstr)  // Sets the labelManufacturer text
{
    QString manufacturer = manufacturerstr;
    if (manufacturerstr.size() > 22)
    {
        manufacturer.truncate(20);
        manufacturer.append("...");
    }
    if (manufacturerstr != "Bloguetrónica")
        ui->labelManufacturer->setStyleSheet("color: red;");
    ui->labelManufacturer->setText(manufacturer);
}

void InformationDialog::setMaxPowerLabelText(uint8_t maxpower)  // Sets the labelMaxPower text
{
    QString maxpowerstr = QString::number(2 * maxpower);  // Maximum current value (reported as being the maximum power consumption by the USB-IF)
    maxpowerstr.append(QString(" [0x%1]").arg(maxpower, 2, 16, QChar('0')));  // Append the bMaxPower descriptor value between brackets
    ui->labelMaxPower->setText(maxpowerstr);
}

void InformationDialog::setProductLabelText(const QString &productstr)  // Sets the labelProduct text
{
    QString product = productstr;
    if (productstr.size() > 22)
    {
        product.truncate(20);
        product.append("...");
    }
    if (productstr != "ITUSB1 USB Test Switch")
        ui->labelProduct->setStyleSheet("color: red;");
    ui->labelProduct->setText(product);
}

void InformationDialog::setRevisionLabelText(uint8_t majrelease, uint8_t minrelease)  // Sets the labelRevision text
{
    QString revision;
    if (majrelease > 1 && majrelease <= 27)
        revision.append(QChar(majrelease + 'A' - 2));  // Append major revision letter (a major release number value of 2 corresponds to the letter "A" and so on)
    if (majrelease == 1 || minrelease != 0)
        revision.append(QString::number(minrelease));  // Append minor revision number
    revision.append(QString(" [0x%1]").arg(majrelease << 8 | minrelease, 4, 16, QChar('0')));  // Append the calculated bcdDevice descriptor value between brackets
    if (majrelease == 0 || majrelease > 27)
        ui->labelRevision->setStyleSheet("color: red;");
    ui->labelRevision->setText(revision);
}

void InformationDialog::setSerialLabelText(const QString &serialstr)  // Sets the labelSerial text
{
    QString serial = serialstr;
    if (serialstr.size() > 22)
    {
        serial.truncate(20);
        serial.append("...");
    }
    if (!serialstr.startsWith("IU1-") || serialstr.size() != 12)
        ui->labelSerial->setStyleSheet("color: red;");
    ui->labelSerial->setText(serial);
}
