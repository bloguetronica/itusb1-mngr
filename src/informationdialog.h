#ifndef INFORMATIONDIALOG_H
#define INFORMATIONDIALOG_H

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
    void setMaxPowerLabelText(uint8_t maxpower);
    void setRevisionLabelText(uint8_t majrelease, uint8_t minrelease);
    void setSerialLabelText(const QString &serialstr);

private:
    Ui::InformationDialog *ui;
};

#endif // INFORMATIONDIALOG_H
