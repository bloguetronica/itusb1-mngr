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


// Includes
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QThread>
#include "aboutdialog.h"
#include "cp2130.h"
#include "datapoint.h"
#include "informationdialog.h"
#include "devicewindow.h"
#include "ui_devicewindow.h"

// Definitions
const int ENUM_RETRIES = 10;  // Number of enumeration retries
const int ERR_LIMIT = 10;     // Error limit

DeviceWindow::DeviceWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DeviceWindow)
{
    ui->setupUi(this);
    labelTime_ = new QLabel(tr("Time: 0s"));
    this->statusBar()->addPermanentWidget(labelTime_);
    labelLog_ = new QLabel(tr("Log: 0"));
    this->statusBar()->addPermanentWidget(labelLog_);
    labelMeas_ = new QLabel(tr("Meas.: 0"));
    this->statusBar()->addPermanentWidget(labelMeas_);
    ui->pushButtonClear->setFocus();
    filepath_ = QDir::homePath();
}

DeviceWindow::~DeviceWindow()
{
    delete ui;
}

// Opens the device and sets it up, while also preparing its window
void DeviceWindow::openDevice(const QString &serialstr)
{
    int err = device_.open(serialstr);
    if (err == ITUSB1Device::SUCCESS) {  // Device was successfully opened
        serialstr_ = serialstr;  // Valid serial number (added in version 2.0)
        setupDevice();  // Necessary in order to get correct readings
        this->setWindowTitle(tr("ITUSB1 USB Test Switch (S/N: %1)").arg(serialstr_));  // Change implemented in version 3.0
        timer_ = new QTimer(this);  // Create a timer
        QObject::connect(&device_, SIGNAL(switchedUSBData()), this, SLOT(update()));
        QObject::connect(&device_, SIGNAL(switchedUSBPower()), this, SLOT(update()));
        QObject::connect(timer_, SIGNAL(timeout()), this, SLOT(update()));
        timer_->start(200);
        time_.start();  // Start counting the elapsed time from this point
    } else if (err == ITUSB1Device::ERROR_INIT) {  // Failed to initialize libusb
        QMessageBox::critical(this, tr("Critical Error"), tr("Could not initialize libusb.\n\nThis is a critical error and execution will be aborted."));
        exit(EXIT_FAILURE);  // This error is critical because libusb failed to initialize
    } else {
        if (err == ITUSB1Device::ERROR_NOT_FOUND) {  // Failed to find device
            QMessageBox::critical(this, tr("Error"), tr("Could not find device."));
        } else if (err == ITUSB1Device::ERROR_BUSY) {  // Failed to claim interface
            QMessageBox::critical(this, tr("Error"), tr("Device is currently unavailable.\n\nPlease confirm that the device is not in use."));
        }
        this->deleteLater();  // Close window after the subsequent show() call
    }
}

// Allows user to save pending data points (implemented in version 3.0)
void DeviceWindow::closeEvent(QCloseEvent *event)
{
    if (log_.hasNewData()) {  // If there are pending data points
        int qmret = QMessageBox::warning(this, tr("Unsaved Data Points"), tr("There are data points in memory that were not saved yet.\n\nDo you wish to save them?"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        if (qmret == QMessageBox::Save) {  // If user clicked "Save"
            int result;
            do {
                result = saveDataPrompt();  // Prompt user where to save, and save if successful (i.e., returns zero)
            } while (result == 2);  // While errors occur (e.g., the user selected, or reselected a location that is write-protected)
            if (result != 0) {
                event->ignore();  // The user canceled the save, so, cancel window closing event
            }
        } else if (qmret == QMessageBox::Cancel) {  // If user clicked "Cancel"
            event->ignore();  // Cancel window closing event
        }  // If the user clicks "Discard", it is implied that the event will proceed and the window will be closed!
    }
}

void DeviceWindow::on_actionAbout_triggered()
{
    AboutDialog about;
    about.exec();
}

// Corresponds to on_actionDelete_triggered(), as found in version 2.0
void DeviceWindow::on_actionDeleteData_triggered()
{
    int qmret = QMessageBox::question(this, tr("Delete Logged Data?"), tr("This action will delete any data points acquired until now.\n\nDo you wish to proceed?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);  // Confirmation added in version 3.0
    if (qmret == QMessageBox::Yes) {  // If user clicked "Yes"
        deleteData();  // Delete acquired data points
    }
}

void DeviceWindow::on_actionInformation_triggered()
{
    int errcnt = 0;
    QString errstr;
    InformationDialog info;
    info.setManufacturerLabelText(device_.getManufacturerDesc(errcnt, errstr));
    info.setProductLabelText(device_.getProductDesc(errcnt, errstr));
    info.setSerialLabelText(device_.getSerialDesc(errcnt, errstr));  // It is important to read the serial number from the OTP ROM, instead of just passing the value of serialstr_
    CP2130::USBConfig config = device_.getUSBConfig(errcnt, errstr);
    info.setRevisionLabelText(config.majrel, config.minrel);
    info.setMaxPowerLabelText(config.maxpow);
    if (opCheck(tr("device-information-retrieval-op"), errcnt, errstr)) {  // If error check passes (the string "device-information-retrieval-op" should be translated to "Device information retrieval")
        info.exec();
    }
}

void DeviceWindow::on_actionRate50_triggered()
{
    timer_->setInterval(50);
    ui->actionRate50->setChecked(true);
    ui->actionRate100->setChecked(false);
    ui->actionRate200->setChecked(false);
    ui->actionRate300->setChecked(false);
    ui->actionRate500->setChecked(false);
}

void DeviceWindow::on_actionRate100_triggered()
{
    timer_->setInterval(100);
    ui->actionRate100->setChecked(true);
    ui->actionRate50->setChecked(false);
    ui->actionRate200->setChecked(false);
    ui->actionRate300->setChecked(false);
    ui->actionRate500->setChecked(false);
}

void DeviceWindow::on_actionRate200_triggered()
{
    timer_->setInterval(200);
    ui->actionRate200->setChecked(true);
    ui->actionRate50->setChecked(false);
    ui->actionRate100->setChecked(false);
    ui->actionRate300->setChecked(false);
    ui->actionRate500->setChecked(false);
}

void DeviceWindow::on_actionRate300_triggered()
{
    timer_->setInterval(300);
    ui->actionRate300->setChecked(true);
    ui->actionRate50->setChecked(false);
    ui->actionRate100->setChecked(false);
    ui->actionRate200->setChecked(false);
    ui->actionRate500->setChecked(false);
}

void DeviceWindow::on_actionRate500_triggered()
{
    timer_->setInterval(500);
    ui->actionRate500->setChecked(true);
    ui->actionRate50->setChecked(false);
    ui->actionRate100->setChecked(false);
    ui->actionRate200->setChecked(false);
    ui->actionRate300->setChecked(false);
}

// Added in version 3.0
void DeviceWindow::on_actionResetTime_triggered()
{
    if (log_.isEmpty()) {
        resetTimeCount();  // Reset time count
    } else {
        int qmret = QMessageBox::question(this, tr("Reset Time Count?"), tr("This action, besides resetting the elapsed time count, will also delete any previously acquired data points.\n\nDo you wish to proceed?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (qmret == QMessageBox::Yes) {  // If user clicked "Yes"
            deleteData();  // Delete acquired data points
            if (device_.isOpen()) {  // This condition is essential so the timer won't be restarted if, in the meantime, the device gets disconnected - In effect this prevents further errors!
                resetTimeCount();  // Reset time count - This will restart the timer too!
            }
        }
    }
}

// Corresponds to on_actionSave_triggered(), as found in version 2.0
void DeviceWindow::on_actionSaveData_triggered()
{
    saveDataPrompt();  // Unlike in closeEvent(), it is not required to obtain the result, since there will be no retries in any case (user can retry later)
}

void DeviceWindow::on_checkBoxData_clicked()
{
    int errcnt = 0;
    QString errstr;
    device_.switchUSBData(ui->checkBoxData->isChecked(), errcnt, errstr);
    opCheck(tr("data-switch-op"), errcnt, errstr);  // The string "data-switch-op" should be translated to "Data switch"
}

void DeviceWindow::on_checkBoxPower_clicked()
{
    int errcnt = 0;
    QString errstr;
    device_.switchUSBPower(ui->checkBoxPower->isChecked(), errcnt, errstr);
    opCheck(tr("power-switch-op"), errcnt, errstr);  // The string "power-switch-op" should be translated to "Power switch"
}

void DeviceWindow::on_pushButtonAttach_clicked()
{
    int errcnt = 0;
    QString errstr;
    device_.attach(errcnt, errstr);
    opCheck(tr("attach-op"), errcnt, errstr);
}

void DeviceWindow::on_pushButtonClear_clicked()
{
    clearMetrics();
    ui->labelOCFault->clear();  // Clear "OC fault!" warning, if applicable
}

void DeviceWindow::on_pushButtonDetach_clicked()
{
    int errcnt = 0;
    QString errstr;
    device_.detach(errcnt, errstr);
    opCheck(tr("detach-op"), errcnt, errstr);
}

void DeviceWindow::on_pushButtonReset_clicked()
{
    if (log_.isEmpty()) {
        resetDevice();
    } else {
        int qmret = QMessageBox::question(this, tr("Reset Device?"), tr("This action, besides resetting the device, will also delete any previously acquired data points.\n\nDo you wish to proceed?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);  // Confirmation added in version 3.0
        if (qmret == QMessageBox::Yes) {  // If user clicked "Yes"
            if (device_.isOpen()) {  // This condition is required to prevent multiple errors if, in the meantime, the device gets disconnected
                resetDevice();
            } else {
                deleteData();  // Delete all data points, as an alternative to resetDevice()
            }
        }
    }
}

// This is the main update routine
void DeviceWindow::update()
{
    // No verification is done here since version 2.0
    int errcnt = 0;
    QString errstr;
    float current = device_.getCurrent(errcnt, errstr);
    bool up = device_.getUSBPowerStatus(errcnt, errstr);
    bool ud = device_.getUSBDataStatus(errcnt, errstr);
    bool oc = device_.getOvercurrentStatus(errcnt, errstr);
    if (opCheck(tr("update-op"), errcnt, errstr)) {  // Update values if no errors occur (implemented since version 2.0, and refactored in version 3.0)
        if (ui->actionLogData->isChecked()) {
            logDataPoint(current, up, ud, oc);
        }
        metrics_.update(current);  // Update actual, average, minimum and maximum values
        updateView(up, ud, oc);
    }
}

// Wrapper function that also resets the "Meas" counter in the status bar (added in version 3.0)
void DeviceWindow::clearMetrics()  // Wrapper function that also resets the "Meas" counter in the status bar (added in version 3.0)
{
    metrics_.clear();
    labelMeas_->setText(tr("Meas.: 0"));
}

// Deletes any acquired data points
void DeviceWindow::deleteData()
{
    log_.clear();
    setLogActionsEnabled(false);  // And also disable logging related actions (added in version 3.0)
    labelLog_->setText(tr("Log: 0"));
}

// Partially disables device window (added in version 3.0, as a refactor)
void::DeviceWindow::disableView()
{
    ui->actionInformation->setEnabled(false);
    ui->menuOptions->setEnabled(false);
    ui->centralwidget->setEnabled(false);
    ui->lcdNumberCurrent->setStyleSheet("");  // Just to ensure that any previously applied styles are removed
    ui->labelOCFault->setStyleSheet("");
    ui->statusBar->setEnabled(false);
}

// Logs acquired data point (added in version 3.0, as a refactor)
void DeviceWindow::logDataPoint(float current, bool up, bool ud, bool oc)
{
    DataPoint datapt;
    datapt.time = time_.elapsed() / 1000.0;  // Log time
    datapt.curr = current;  // Current
    datapt.up = up;  // USB power status
    datapt.ud = ud;  // USB data status
    datapt.oc = oc;  // OC status
    log_.append(datapt);  // Add data point
    setLogActionsEnabled(true);  // And also enable logging related actions
}

// Checks for errors and validates (or ultimately halts) device operations (added in version 3.0, as a refactor)
bool DeviceWindow::opCheck(const QString &op, int errcnt, QString errstr)
{
    bool retval;
    if (errcnt > 0) {
        if (device_.disconnected()) {  // Added in version 3.2
            timer_->stop();  // This prevents further errors
            QMessageBox::critical(this, tr("Error"), tr("Device disconnected.\n\nThe corresponding window will be disabled."));
            disableView();  // Disable device window
            device_.close();
        } else {
            errstr.chop(1);  // Remove the last character, which is always a newline
            QMessageBox::critical(this, tr("Error"), tr("%1 operation returned the following error(s):\n– %2", "", errcnt).arg(op, errstr.replace("\n", "\n– ")));
            erracc_ += errcnt;
            if (erracc_ > ERR_LIMIT) {  // If the session accumulated more errors than the limit set by "ERR_LIMIT" [10] (this is validateErrors(), in essence)
                timer_->stop();  // Again, this prevents further errors
                QMessageBox::critical(this, tr("Error"), tr("Detected too many errors.\n\nThe device window will be disabled."));
                disableView();  // Disable device window
                device_.reset(errcnt, errstr);  // Try to reset the device for sanity purposes, but don't check if it was successful
                device_.close();  // Ensure that the device is freed, even if the previous device reset is not effective (device_.reset() also frees the device interface, as an effect of re-enumeration)
                // Since version 3.0, it is essential that device_.close() is called, since some important checks rely on device_.isOpen() to retrieve a proper status
            }
        }
        retval = false;  // Failed check
    } else {
        retval = true;  // Passed check
    }
    return retval;
}

// Resets the device (added in version 3.0, as a refactor)
void DeviceWindow::resetDevice()
{
    timer_->stop();  // Stop the update timer momentarily, in order to avoid recurrent errors if the device gets disconnected during a reset, or other unexpected behavior (bug fix added in version 2.0)
    clearMetrics();
    deleteData();  // As of version 3.0, all data points are deleted before a reset is issued to the device, so that the window may close if there is a problem - Effectively, this will bypass the inner workings of closeEvent()!
    ui->labelOCFault->clear();  // Clear "OC fault!" warning, if applicable
    int errcnt = 0;
    QString errstr;
    device_.reset(errcnt, errstr);
    opCheck(tr("reset-op"), errcnt, errstr);  // The string "reset-op" should be translated to "Reset"
    if (device_.isOpen()) {  // If opCheck() passes, thus, not closing the device (condition refactored in version 3.0, because windowEnabled_ was found to be redundant and, therefore, removed)
        device_.close();  // Important! - This should be done always, even if the previous reset operation shows an error, because an error doesn't mean that a device reset was not effected
        int err;
        for (int i = 0; i < ENUM_RETRIES; ++i) {  // Verify enumeration according to the number of times set by "ENUM_RETRIES" [10]
            QThread::msleep(500);  // Wait 500ms each time
            err = device_.open(serialstr_);
            if (err != ITUSB1Device::ERROR_NOT_FOUND) {  // Retry only if the device was not found yet (as it may take some time to enumerate)
                break;
            }
        }
        if (err == ITUSB1Device::ERROR_INIT) {  // Failed to initialize libusb
            QMessageBox::critical(this, tr("Critical Error"), tr("Could not reinitialize libusb.\n\nThis is a critical error and execution will be aborted."));
            exit(EXIT_FAILURE);  // This error is critical because libusb failed to initialize
        } else if (err == ITUSB1Device::ERROR_NOT_FOUND) {  // Failed to find device
            QMessageBox::critical(this, tr("Error"), tr("Device disconnected."));
            this->close();  // Close window
        } else if (err == ITUSB1Device::ERROR_BUSY) {  // Failed to claim interface
            QMessageBox::critical(this, tr("Error"), tr("Device ceased to be available.\n\nPlease verify that the device is not in use by another application."));
            this->close();  // Close window
        } else {
            setupDevice();  // Necessary in order to get correct readings after a device reset
            erracc_ = 0;  // Zero the error count accumulator, since a new session gets started once the reset is done
            resetTimeCount();  // Reset time count
        }
    }
}

// Resets (and restarts) the time count (added in version 3.0, mainly as a refactor)
void DeviceWindow::resetTimeCount()
{
    timer_->start();  // Restart the timer using user settings
    time_.start();  // Restart counting the elapsed time from zero
    labelTime_->setText(tr("Time: 0s"));
}

// Saves logged data with prompt to the user
int DeviceWindow::saveDataPrompt()
{
    int retval = 0;
    QString csvstr = log_.toCSVString();  // Compiles acquired data points to CSV formatted string (note that implicit sharing is implemented in the QString class which, in this case, avoids unnecessary copying and minimizes RAM usage)
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Logged Data to File"), filepath_, tr("CSV files (*.csv);;All files (*)"));
    if (filename.isEmpty()) {  // Note that the previous dialog will return an empty string if the user cancels it
        retval = 1;
    } else {
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, tr("Error"), tr("Could not write to %1.\n\nPlease verify that you have write access to this file.").arg(QDir::toNativeSeparators(filename)));
            retval = 2;
        } else {
            QTextStream out(&file);
            out.setCodec("UTF-8");
            out << csvstr;
            file.close();
            filepath_ = filename;
            log_.noNewData();  // All data points were saved
        }
    }
    return retval;
}

// Enables or disables logging related actions (added in version 3.0)
void DeviceWindow::setLogActionsEnabled(bool value)
{
    ui->actionSaveData->setEnabled(value);
    ui->actionDeleteData->setEnabled(value);
}

// Prepares the device, performing basic configurations
void DeviceWindow::setupDevice()
{
    int errcnt = 0;
    QString errstr;
    device_.setup(errcnt, errstr);
    if (errcnt > 0) {
        if (device_.disconnected()) {  // Verification added in version 3.3
            device_.close();
            QMessageBox::critical(this, tr("Error"), tr("Device disconnected.\n\nPlease reconnect it and try again."));
        } else {
            device_.reset(errcnt, errstr);  // Try to reset the device for sanity purposes, but don't check if it was successful (device is reset before showing the dialog since version 3.2)
            device_.close();  // Close the device (implemented here since version 3.2)
            errstr.chop(1);  // Remove the last character, which is always a newline
            QMessageBox::critical(this, tr("Error"), tr("Setup operation returned the following error(s):\n– %1\n\nPlease try accessing the device again.", "", errcnt).arg(errstr.replace("\n", "\n– ")));
        }
        this->deleteLater();  // In a context where the window is already visible, it has the same effect as this->close()
    }
}

// Updates the view, including status and displayed value (added in version 3.0, as a refactor)
void DeviceWindow::updateView(bool up, bool ud, bool oc)
{
    if (up && ud) {
        ui->labelStatus->setText(tr("Connection enabled"));
    } else {
        ui->labelStatus->setText(tr("Connection disabled"));
    }
    float value;
    if (ui->radioButtonDisplayAvg->isChecked()) {
        value = metrics_.average();  // Pass the average to be displayed
    } else if (ui->radioButtonDisplayMin->isChecked()) {
        value = metrics_.minimum();  // Pass the minimum value to be displayed
    } else if (ui->radioButtonDisplayMax->isChecked()) {
        value = metrics_.maximum();  // Pass the maximum value to be displayed
    } else {
        value = metrics_.last();  // Pass the last input value to be displayed
    }
    if (value > 500) {
        ui->lcdNumberCurrent->setStyleSheet("color: darkred;");  // A dark red color indicates that the measured current exceeds the 500mA limit established by the USB 2.0 specification
    } else {
        ui->lcdNumberCurrent->setStyleSheet("");
    }
    if (value < 1000) {
        ui->lcdNumberCurrent->display(QString::number(value, 'f', 1));  // Display the value passed previously
    } else {
        ui->lcdNumberCurrent->display(QString("OL"));  // Display "OL"
    }
    if (oc) {
        ui->labelOCFault->setStyleSheet("color: red;");
        ui->labelOCFault->setText(tr("OC fault!"));
    } else {
        ui->labelOCFault->setStyleSheet("");  // Empty (default) style sheet
        if (!ui->actionPersistent->isChecked()) {
            ui->labelOCFault->clear();
        }
    }
    ui->checkBoxPower->setChecked(up);
    ui->checkBoxData->setChecked(ud);
    labelTime_->setText(tr("Time: %1s").arg(time_.elapsed() / 1000.0, 0, 'f', 1));
    labelLog_->setText(tr("Log: %1").arg(log_.size()));
    labelMeas_->setText(tr("Meas.: %1").arg(metrics_.numberOfMeasurements()));
}

// validateErrors() was integrated inside opCheck() since version 3.0
