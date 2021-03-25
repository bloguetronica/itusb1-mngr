/* ITUSB1 Manager - Version 3.0 for Debian Linux
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


#ifndef INFORMATIONDIALOG_H
#define INFORMATIONDIALOG_H

// Includes
#include <QDialog>
#include <QString>

namespace Ui {
class InformationDialog;
}

class InformationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InformationDialog(QWidget *parent = 0);
    ~InformationDialog();
    void setManufacturerLabelText(const QString &manufacturerstr);
    void setMaxPowerLabelText(uint8_t maxpower);
    void setProductLabelText(const QString &productstr);
    void setRevisionLabelText(uint8_t majrelease, uint8_t minrelease);
    void setSerialLabelText(const QString &serialstr);

private:
    Ui::InformationDialog *ui;
};

#endif // INFORMATIONDIALOG_H
