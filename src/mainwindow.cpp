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
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include "aboutdialog.h"
#include "devicewindow.h"
#include "itusb1device.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->comboBoxDevices->setFocus();
    refresh();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Instead of forcing any existing child windows to be closed, this will allow them to execute any pending tasks (implemented in version 3.0)
void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    QList<QMainWindow *> childList = findChildren<QMainWindow *>();
    for (QMainWindow *child : childList) {
        child->close();  // This will "force" window closure, even if the user tries to cancel from the child's popup dialog
    }
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog about;
    about.exec();
}

void MainWindow::on_comboBoxDevices_currentIndexChanged(int index)
{
    if (index == 0) {
        ui->pushButtonOpen->setEnabled(false);
    } else {
        ui->pushButtonOpen->setEnabled(true);
    }
}

void MainWindow::on_pushButtonOpen_clicked()
{
    QString serialstr = ui->comboBoxDevices->currentText();  // Extract the serial number from the chosen item in the combo box
    DeviceWindow *deview = new DeviceWindow(this);  // Create a new window that will close when its parent window closes
    deview->setAttribute(Qt::WA_DeleteOnClose);  // This will not only free the allocated memory once the window is closed, but will also automatically call the destructor of the respective device, which in turn closes it
    deview->openDevice(serialstr);  // Access the selected device and prepare its view
    deview->show();  // Then open the corresponding window
}

void MainWindow::on_pushButtonRefresh_clicked()
{
    refresh();
}

// Refreshes the combo box list
void MainWindow::refresh()
{
    int errcnt = 0;
    QString errstr;
    QStringList comboBoxList = {tr("Select device...")};
    comboBoxList += ITUSB1Device::listDevices(errcnt, errstr);
    if (errcnt > 0) {
        QMessageBox::critical(this, tr("Critical Error"), tr("%1\nThis is a critical error and execution will be aborted.").arg(errstr));
        exit(EXIT_FAILURE);  // This error is critical because either libusb failed to initialize, or could not retrieve a list of devices
    } else {
        ui->comboBoxDevices->clear();
        ui->comboBoxDevices->addItems(comboBoxList);
    }
}
