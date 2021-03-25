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
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QThread>
#include "aboutdialog.h"
#include "informationdialog.h"
#include "devicewindow.h"
#include "ui_devicewindow.h"

DeviceWindow::DeviceWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DeviceWindow)
{
    ui->setupUi(this);
    ui->pushButtonClear->setFocus();
    filepath_ = QDir::homePath();
}

DeviceWindow::~DeviceWindow()
{
    delete ui;
}

void DeviceWindow::openDevice(const QString &serialstr)  // Opens the device and sets it up, while also preparing its window
{
    int err = device_.open(serialstr);
    if (err == 1)  // Failed to initialize libusb
    {
        QMessageBox errorLibUSB;
        errorLibUSB.critical(this, tr("Critical error"), tr("Could not initialize libusb.\n\nThis is a critical error and execution will be aborted."));
        exit(EXIT_FAILURE);  // This error is critical because libusb failed to initialize
    }
    else if (err == 2)  // Failed to find device
    {
        QMessageBox errorDevNotFound;
        errorDevNotFound.critical(this, tr("Error"), tr("Could not find device."));
        this->deleteLater();  // Close window after the subsequent show() call
    }
    else if (err == 3)  // Failed to claim interface
    {
        QMessageBox errorDevUnavailable;
        errorDevUnavailable.critical(this, tr("Error"), tr("Device is currently unavailable.\n\nPlease confirm that the device is not in use."));
        this->deleteLater();  // Close window after the subsequent show() call
    }
    else
    {
        serialstr_ = serialstr;  // Valid serial number (added in version 2.0)
        setupDevice();  // Necessary in order to get correct readings
        this->setWindowTitle("ITUSB1 USB Test Switch (S/N: " + serialstr_ + ")");
        timer_ = new QTimer(this);  // Create a timer
        QObject::connect(timer_, SIGNAL(timeout()), this, SLOT(update()));
        timer_->start(200);
        time_.start();  // Start counting the elapsed time from this point
    }
}

void DeviceWindow::on_actionAbout_triggered()
{
    AboutDialog about;
    about.exec();
}

void DeviceWindow::on_actionDelete_triggered()
{
    deleteData();  // Delete acquired data points
}

void DeviceWindow::on_actionInformation_triggered()
{
    int errcnt = 0;
    QString errstr;
    InformationDialog info;
    info.setManufacturerLabelText(device_.getManufacturer(errcnt, errstr));
    info.setProductLabelText(device_.getProduct(errcnt, errstr));
    info.setSerialLabelText(device_.getSerial(errcnt, errstr));  // It is important to read the serial number from the OTP ROM, instead of just passing the value of "serialstr_"
    info.setRevisionLabelText(device_.getMajorRelease(errcnt, errstr), device_.getMinorRelease(errcnt, errstr));
    info.setMaxPowerLabelText(device_.getMaxPower(errcnt, errstr));
    if (errcnt > 0)
    {
        erracc_ += errcnt;
        errstr.chop(1);  // Remove the last character, which is always a newline
        QMessageBox errorInformation;
        errorInformation.critical(this, tr("Error"), tr("Device information retrieval operation returned the following error%1:\n- %2").arg((errcnt == 1) ? "" : "s", errstr.replace("\n", "\n- ")));
        validateErrors();
    }
    else
        info.exec();
}

void DeviceWindow::on_actionRate100_triggered()
{
    timer_->setInterval(100);
    ui->actionRate100->setChecked(true);
    ui->actionRate200->setChecked(false);
    ui->actionRate300->setChecked(false);
}

void DeviceWindow::on_actionRate200_triggered()
{
    timer_->setInterval(200);
    ui->actionRate200->setChecked(true);
    ui->actionRate100->setChecked(false);
    ui->actionRate300->setChecked(false);
}

void DeviceWindow::on_actionRate300_triggered()
{
    timer_->setInterval(300);
    ui->actionRate300->setChecked(true);
    ui->actionRate100->setChecked(false);
    ui->actionRate200->setChecked(false);
}

void DeviceWindow::on_actionSave_triggered()
{
    QString document = "Time (s),Current (mA),USB power,USB data,OC flag\n";
    size_t len = datapts_.size();  // This translates to the number of elements of "datapts_"
    for (size_t i = 0; i < len; ++i)
    {
        document.append(QString("%1,").arg(datapts_.value(i).time, 0, 'f', 3));
        document.append(QString("%1,").arg(datapts_.value(i).curr, 0, 'f', 1));
        document.append(QString("%1,").arg(datapts_.value(i).up));
        document.append(QString("%1,").arg(datapts_.value(i).ud));
        document.append(QString("%1\n").arg(datapts_.value(i).oc));
    }
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Logged Data to File"), filepath_, tr("CSV files (*.csv);;All files (*)"));
    if (!filename.isEmpty())  // Note that the previous dialog will return an empty string if the user cancels it
    {
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox errorWrite;
            errorWrite.critical(this, tr("Error"), tr("Could not write to %1.\n\nPlease verify that you have write access to this file.").arg(QDir::toNativeSeparators(filename)));
        }
        else
        {
            QTextStream out(&file);
            out.setCodec("UTF-8");
            out << document;
            file.close();
            filepath_ = filename;
        }
    }
}

void DeviceWindow::on_checkBoxData_clicked()
{
    int errcnt = 0;
    QString errstr;
    if (ui->checkBoxData->isChecked())
        device_.setGPIO2(false, errcnt, errstr);
    else
        device_.setGPIO2(true, errcnt, errstr);
    if (errcnt > 0)
    {
        erracc_ += errcnt;
        errstr.chop(1);  // Remove the last character, which is always a newline
        QMessageBox errorDataSw;
        errorDataSw.critical(this, tr("Error"), tr("Data switch operation returned the following error%1:\n- %2").arg((errcnt == 1) ? "" : "s", errstr.replace("\n", "\n- ")));
        validateErrors();
    }
}

void DeviceWindow::on_checkBoxPower_clicked()
{
    int errcnt = 0;
    QString errstr;
    if (ui->checkBoxPower->isChecked())
        device_.setGPIO1(false, errcnt, errstr);
    else
        device_.setGPIO1(true, errcnt, errstr);
    if (errcnt > 0)
    {
        erracc_ += errcnt;
        errstr.chop(1);  // Remove the last character, which is always a newline
        QMessageBox errorPowerSw;
        errorPowerSw.critical(this, tr("Error"), tr("Power switch operation returned the following error%1:\n- %2").arg((errcnt == 1) ? "" : "s", errstr.replace("\n", "\n- ")));
        validateErrors();
    }
}

void DeviceWindow::on_pushButtonAttach_clicked()
{
    int errcnt = 0;
    QString errstr;
    if (device_.getGPIO1(errcnt, errstr) != device_.getGPIO2(errcnt, errstr))  // If GPIO.1 and GPIO.2 pins do not match in value, indicating an unusual state
    {
        device_.setGPIO1(true, errcnt, errstr);  // Set GPIO.1 to a logical high to switch off VBUS first
        device_.setGPIO2(true, errcnt, errstr);  // Set GPIO.2 to a logical high to disconnect the data lines
        QThread::msleep(100);  // Wait 100ms to allow for device shutdown
    }
    if (device_.getGPIO1(errcnt, errstr) && device_.getGPIO2(errcnt, errstr))  // If GPIO.1 and GPIO.2 are both set to a logical high
    {
        device_.setGPIO1(false, errcnt, errstr);  // Set GPIO.1 to a logical low to switch VBUS on
        QThread::msleep(100);  // Wait 100ms in order to emulate a manual attachment of the device
        device_.setGPIO2(false, errcnt, errstr);  // Set GPIO.2 to a logical low to connect the data lines
        QThread::msleep(100);  // Wait 100ms so that device enumeration process can, at least, start (this is not enough to guarantee enumeration, though)
    }
    if (errcnt > 0)
    {
        erracc_ += errcnt;
        errstr.chop(1);  // Remove the last character, which is always a newline
        QMessageBox errorAttach;
        errorAttach.critical(this, tr("Error"), tr("Attach operation returned the following error%1:\n- %2").arg((errcnt == 1) ? "" : "s", errstr.replace("\n", "\n- ")));
        validateErrors();
    }
}

void DeviceWindow::on_pushButtonClear_clicked()
{
    clearValues();  // Clear metrics
    ui->labelOCFault->clear();  // Clear "OC fault!" warning, if applicable
}

void DeviceWindow::on_pushButtonDetach_clicked()
{
    int errcnt = 0;
    QString errstr;
    if (!device_.getGPIO1(errcnt, errstr) || !device_.getGPIO2(errcnt, errstr))  // If GPIO.1 or GPIO.2, or both, are set to a logical low
    {
        device_.setGPIO2(true, errcnt, errstr);  // Set GPIO.2 to a logical high so that the data lines are disconnected
        QThread::msleep(100);  // Wait 100ms in order to emulate a manual detachment of the device
        device_.setGPIO1(true, errcnt, errstr);  // Set GPIO.1 to a logical high to switch VBUS off
        QThread::msleep(100);  // Wait 100ms to allow for device shutdown
    }
    if (errcnt > 0)
    {
        erracc_ += errcnt;
        errstr.chop(1);  // Remove the last character, which is always a newline
        QMessageBox errorDetach;
        errorDetach.critical(this, tr("Error"), tr("Detach operation returned the following error%1:\n- %2").arg((errcnt == 1) ? "" : "s", errstr.replace("\n", "\n- ")));
        validateErrors();
    }
}

void DeviceWindow::on_pushButtonReset_clicked()
{
    timer_->stop();  // Stop the update timer momentarily, in order to avoid a segmentation fault if the device gets disconnected during a reset, or other unexpected behavior (bug fix added in version 2.0)
    int errcnt = 0;
    QString errstr;
    device_.reset(errcnt, errstr);
    if (errcnt > 0)
    {
        erracc_ += errcnt;
        errstr.chop(1);  // Remove the last character, which is always a newline
        QMessageBox errorReset;
        errorReset.critical(this, tr("Error"), tr("Reset operation returned the following error:\n- %1").arg(errstr));
        validateErrors();
    }
    device_.close();  // Important! - This should be done always, even if the previous reset operation shows an error, because an error doesn't mean that a device reset was not effected
    if (windowEnabled_)  // If "validateErrors()" passes, keeping the device window enabled (condition added in version 2.0)
    {
        int err;
        for (int i = 0; i < 10; ++i)  // Verify enumeration 10 times
        {
            QThread::msleep(500);  // Wait 500ms each time
            err = device_.open(serialstr_);
            if (err != 2)  // Retry only if the device was not found yet (as it may take some time to enumerate)
                break;
        }
        if (err == 1)  // Failed to initialize libusb
        {
            QMessageBox errorLibUSB;
            errorLibUSB.critical(this, tr("Critical error"), tr("Could not reinitialize libusb.\n\nThis is a critical error and execution will be aborted."));
            exit(EXIT_FAILURE);  // This error is critical because libusb failed to initialize
        }
        else if (err == 2)  // Failed to find device
        {
            QMessageBox errorDevDisconnected;
            errorDevDisconnected.critical(this, tr("Error"), tr("Device disconnected."));
            this->close();  // Close window
        }
        else if (err == 3)  // Failed to claim interface
        {
            QMessageBox errorDevUnavailable;
            errorDevUnavailable.critical(this, tr("Error"), tr("Device ceased to be available.\n\nPlease verify that the device is not in use by another application."));
            this->close();  // Close window
        }
        else
        {
            setupDevice();  // Necessary in order to get correct readings after a device reset
            clearValues();  // Clear metrics
            deleteData();  // Delete acquired data points
            erracc_ = 0;  // Zero the error count accumulator, since a new session gets started once the reset is done
            ui->labelOCFault->clear();  // Clear "OC fault!" warning, if applicable
            timer_->start();  // Restart the timer using previous settings
            time_.start();  // Restart counting the elapsed time from zero
        }
    }
}

void DeviceWindow::update()  // Updates the view
{
    // No verification is done here since version 2.0
    int errcnt = 0;
    QString errstr;
    bool nup = device_.getGPIO1(errcnt, errstr);  // Get the current value of the GPIO.1 pin, which corresponds to the !UPEN signal
    bool nud = device_.getGPIO2(errcnt, errstr);  // Get the current value of the GPIO.2 pin, which corresponds to the !UDEN signal
    bool noc = device_.getGPIO3(errcnt, errstr);  // Get the current value of the GPIO.3 pin, which corresponds to the !UPOC signal
    device_.selectCS(0, errcnt, errstr);  // Enable the chip select corresponding to channel 0, and disable any others
    device_.getCurrent(errcnt, errstr);  // Discard this reading, as it will reflect a past measurement
    uint16_t curr_code_sum = 0;
    for (int i = 0; i < 5; ++i)
    {
        curr_code_sum += device_.getCurrent(errcnt, errstr);  // Read the raw value (from the LTC2312 on channel 0) and add it to the sum
    }
    QThread::usleep(100);  // Wait 100us, in order to prevent possible errors while disabling the chip select (workaround)
    device_.disableCS(0, errcnt, errstr);  // Disable the previously enabled chip select
    if (errcnt > 0)
    {
        erracc_ += errcnt;
        errstr.chop(1);  // Removes the last character, which is always a newline
        QMessageBox errorUpdate;
        errorUpdate.critical(this, tr("Error"), tr("Update operation returned the following error%1:\n- %2").arg((errcnt == 1) ? "" : "s", errstr.replace("\n", "\n- ")));
        validateErrors();
    }
    else  // Update values if no errors occur (implemented since version 2.0)
    {
        float current = curr_code_sum / 20.0, value;  // Calculate the average current out of five readings for each point (current = curr_code / 4.0 for a single reading)
        if (ui->actionLog->isChecked())
        {
            datapoint datapt;
            datapt.time = time_.elapsed() / 1000.0;  // Log time
            datapt.curr = current;  // Current
            datapt.up = !nup;  // USB power status
            datapt.ud = !nud;  // USB data status
            datapt.oc = !noc;  // OC status
            datapts_.append(datapt);  // Add data point to log
        }
        ++nmeas_;
        avg_ = (current + avg_ * (nmeas_ - 1)) / nmeas_;  // Calculate the recursive average of all accumulated data points
        if (current < min_)  // This condition doesn't exclude the next!
            min_ = current;  // Minimum value
        if (current > max_)  // This condition doesn't exclude the previous!
            max_ = current;  // Maximum value
        if (ui->radioButtonDisplayAvg->isChecked())
            value = static_cast<float>(avg_);  // Pass the average to be displayed
        else if (ui->radioButtonDisplayMin->isChecked())
            value = min_;  // Pass the minimum value to be displayed
        else if (ui->radioButtonDisplayMax->isChecked())
            value = max_;  // Pass the maximum value to be displayed
        else
            value = current;
        if (nup || nud)
            ui->labelStatus->setText("Connection disabled");
        else
            ui->labelStatus->setText("Connection enabled");
        if (value > 500)
            ui->lcdNumberCurrent->setStyleSheet("color: darkred;");  // A dark red color indicates that the measured current exceeds the 500mA limit established by the USB 2.0 specification
        else
            ui->lcdNumberCurrent->setStyleSheet("");
        if (value < 1000)
            ui->lcdNumberCurrent->display(QString::number(value, 'f', 1));  // Display the value passed previously
        else
            ui->lcdNumberCurrent->display(QString("OL"));  // Display "OL"
        if (noc)
        {
            ui->labelOCFault->setStyleSheet("");  // Empty (default) style sheet
            if (!ui->actionPersistent->isChecked())
                ui->labelOCFault->clear();
        }
        else
        {
            ui->labelOCFault->setStyleSheet("color: red;");
            ui->labelOCFault->setText("OC fault!");
        }
        if (nup)
            ui->checkBoxPower->setChecked(false);
        else
            ui->checkBoxPower->setChecked(true);
        if (nud)
            ui->checkBoxData->setChecked(false);
        else
            ui->checkBoxData->setChecked(true);
    }
}

void DeviceWindow::clearValues()  // Clears metrics
{
    nmeas_ = 0;
    min_ = 1023.75;
    max_ = 0;
}

void DeviceWindow::deleteData()  // Deletes any acquired data points
{
    datapts_.clear();
    datapts_.squeeze();  // This action releases memory that is no longer required
}

void DeviceWindow::setupDevice()  // Prepares the device, performing basic configurations
{
    int errcnt = 0;
    QString errstr;
    device_.configureSPIMode(0, CSMODEPP, CFRQ1500K, CPOL0, CPHA0, errcnt, errstr);  // Chip select pin mode regarding channel 0 is push-pull, the clock frequency is set to 1.5MHz, clock polarity is active high (CPOL = 0) and data is valid on each rising edge (CPHA = 0)
    device_.disableSPIDelays(0, errcnt, errstr);  // Disable all SPI delays for channel 0
    device_.selectCS(0, errcnt, errstr);  // Enable the chip select corresponding to channel 0, and disable any others
    device_.getCurrent(errcnt, errstr);  // Discard this first reading - This also wakes up the LTC2312, if in nap or sleep mode!
    QThread::usleep(1100);  // Wait 1.1ms to ensure that the LTC2312 is awake, and also to prevent possible errors while disabling the chip select (workaround)
    device_.disableCS(0, errcnt, errstr);  // Disable the previously enabled chip select
    if (errcnt > 0)
    {
        errstr.chop(1);  // Remove the last character, which is always a newline
        QMessageBox errorSetup;
        errorSetup.critical(this, tr("Error"), tr("Setup operation returned the following error%1:\n- %2\n\nPlease try accessing the device again.").arg((errcnt == 1) ? "" : "s", errstr.replace("\n", "\n- ")));
        device_.reset(errcnt, errstr);  // Try to reset the device for sanity purposes, but don't check if it was successful
        this->deleteLater();  // In a context where the window is already visible, it has the same effect as "this->close()"
    }
}

void DeviceWindow::validateErrors()  // Checks if more than 10 errors occurred during the session
{
    if (erracc_ > 10)  // If the session accumulated more than 10 errors
    {
        timer_->stop();  // This prevents further errors
        QMessageBox errorTooMany;
        errorTooMany.critical(this, tr("Error"), tr("Detected too many errors. Device may not be properly connected.\n\nThe device window will be disabled."));
        windowEnabled_ = false;  // Added in version 2.0
        ui->actionInformation->setEnabled(false);
        ui->menuOptions->setEnabled(false);
        ui->centralwidget->setEnabled(false);
        ui->lcdNumberCurrent->setStyleSheet("");  // Just to ensure that any previously applied styles are removed
        ui->labelOCFault->setStyleSheet("");
        int errcnt;
        QString errstr;
        device_.reset(errcnt, errstr);  // Try to reset the device for sanity purposes, but don't check if it was successful
        device_.close();  // Ensure that the device is freed, even if the previous device reset is not effective ("device_.reset()" also frees the device interface, as an effect of re-enumeration)
    }
}
