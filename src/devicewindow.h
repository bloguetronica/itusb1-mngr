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


#ifndef DEVICEWINDOW_H
#define DEVICEWINDOW_H

// Includes
#include <QMainWindow>
#include <QTime>
#include <QTimer>
#include "itusb1device.h"

namespace Ui {
class DeviceWindow;
}

class DeviceWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DeviceWindow(QWidget *parent = 0);
    ~DeviceWindow();

    void openDevice(const QString &serialstr);

private slots:
    void on_actionAbout_triggered();
    void on_actionDelete_triggered();
    void on_actionInformation_triggered();
    void on_actionRate100_triggered();
    void on_actionRate200_triggered();
    void on_actionRate300_triggered();
    void on_actionSave_triggered();
    void on_checkBoxData_clicked();
    void on_checkBoxPower_clicked();
    void on_pushButtonAttach_clicked();
    void on_pushButtonClear_clicked();
    void on_pushButtonDetach_clicked();
    void on_pushButtonReset_clicked();
    void update();

private:
    Ui::DeviceWindow *ui;
    bool windowEnabled_ = true;
    double avg_;
    float min_ = 1023.75, max_ = 0;
    size_t nmeas_ = 0, erracc_ = 0;
    struct datapoint
    {
        double time;
        float curr;
        bool up;
        bool ud;
        bool oc;
    };
    ITUSB1Device device_;
    QString filepath_, serialstr_;
    QTime time_;
    QTimer *timer_;
    QVector<datapoint> datapts_;
    void clearValues();
    void deleteData();
    void setupDevice();
    void validateErrors();
};

#endif // DEVICEWINDOW_H
