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

void InformationDialog::setMaxPowerLabelText(uint8_t maxpower)  // Sets the labelMaxPower text
{
    QString maxpowerstr;
    maxpowerstr.append(QString("%1").arg(2 * maxpower));  // Append the maximum current value (reported as being the maximum power consumption by the USB-IF)
    maxpowerstr.append(QString(" [0x%1]").arg(maxpower, 2, 16, QChar('0')));  // Append the bMaxPower descriptor value between brackets
    ui->labelMaxPower->setText(maxpowerstr);
}

void InformationDialog::setRevisionLabelText(uint8_t majrelease, uint8_t minrelease)  // Sets the labelRevision text
{
    QString revision;
    if (majrelease > 1 && majrelease < 28)
        revision.append(QChar(majrelease + 'A' - 2));  // Append major revision letter (a major release number value of 2 corresponds to the letter "A" and so on)
    if (majrelease == 1 || minrelease != 0)
        revision.append(QString("%1").arg(minrelease));  // Append minor revision number
    revision.append(QString(" [0x%1]").arg(majrelease << 8 | minrelease, 4, 16, QChar('0')));  // Append the calculated bcdDevice descriptor value between brackets
    ui->labelRevision->setText(revision);
}

void InformationDialog::setSerialLabelText(const QString &serialstr)  // Sets the labelSerial text
{
    ui->labelSerial->setText(serialstr);
}
