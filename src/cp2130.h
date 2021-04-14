/* CP2130 class for Qt - Version 0.2.3 for Debian Linux
   Copyright (c) 2021 Samuel Lourenço

   This library is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or (at your
   option) any later version.

   This library is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
   License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this library.  If not, see <https://www.gnu.org/licenses/>.


   Please feel free to contact me via e-mail: samuel.fmlourenco@gmail.com */


#ifndef CP2130_H
#define CP2130_H

// Includes
#include <QStringList>  // Also includes QString
#include <libusb-1.0/libusb.h>

class CP2130
{
private:
    libusb_context *context_;
    libusb_device_handle *handle_;
    bool kernelAttached_;

public:
    //Class definitions
    static const quint16 VID = 0x10C4;     // Default USB vendor ID
    static const quint16 PID = 0x87A0;     // Default USB product ID
    static const bool CSMODEOD = false;    // Boolean corresponding to chip select open drain mode, applicable to SPIMode/configureSPIMode()
    static const bool CSMODEPP = true;     // Boolean corresponding to chip select push-pull mode, applicable to SPIMode/configureSPIMode()
    static const quint8 CFRQ12M = 0x00;    // Value corresponding to a clock frequency of 12MHz, applicable to SPIMode/configureSPIMode()
    static const quint8 CFRQ6M = 0x01;     // Value corresponding to a clock frequency of 6MHz, applicable to SPIMode/configureSPIMode()
    static const quint8 CFRQ3M = 0x02;     // Value corresponding to a clock frequency of 3MHz, applicable to SPIMode/configureSPIMode()
    static const quint8 CFRQ1500K = 0x03;  // Value corresponding to a clock frequency of 1.5MHz, applicable to SPIMode/configureSPIMode()
    static const quint8 CFRQ750K = 0x04;   // Value corresponding to a clock frequency of 750KHz, applicable to SPIMode/configureSPIMode()
    static const quint8 CFRQ375K = 0x05;   // Value corresponding to a clock frequency of 375KHz, applicable to SPIMode/configureSPIMode()
    static const quint8 CFRQ1875= 0x06;    // Value corresponding to a clock frequency of 187.5KHz, applicable to SPIMode/configureSPIMode()
    static const quint8 CFRQ938 = 0x07;    // Value corresponding to a clock frequency of 93.8KHz, applicable to SPIMode/configureSPIMode()
    static const bool CPOL0 = false;       // Boolean corresponding to CPOL = 0, applicable to SPIMode/configureSPIMode()
    static const bool CPOL1 = true;        // Boolean corresponding to CPOL = 1, applicable to SPIMode/configureSPIMode()
    static const bool CPHA0 = false;       // Boolean corresponding to CPHA = 0, applicable to SPIMode/configureSPIMode()
    static const bool CPHA1 = true;        // Boolean corresponding to CPHA = 1, applicable to SPIMode/configureSPIMode()

    struct SPIDelays {
        bool cstglen;       // CS toggle enable
        bool prdasten;      // Pre-deassert delay enable
        bool pstasten;      // Post-assert delay enable
        bool itbyten;       // Inter-byte delay enable
        quint16 prdastdly;  // Pre-deassert delay (10us units, big-endian)
        quint16 pstastdly;  // Post-assert delay (10us units, big-endian)
        quint16 itbytdly;   // Inter-byte delay (10us units, big-endian)
    };

    struct SPIMode {
        bool csmode;  // Chip select mode (false for open drain, true for push-pull)
        quint8 cfrq;  // Clock frequency
        bool cpol;    // Clock polarity
        bool cpha;    // Clock phase
    };

    CP2130();
    ~CP2130();

    int bulkTransfer(quint8 endpoint, unsigned char *data, int length, int *transferred, int &errcnt, QString &errstr) const;
    void configureSPIDelays(quint8 channel, const SPIDelays &delays, int &errcnt, QString &errstr) const;
    void configureSPIMode(quint8 channel, const SPIMode &mode, int &errcnt, QString &errstr) const;
    int controlTransfer(quint8 bmRequestType, quint8 bRequest, quint16 wValue, quint16 wIndex, unsigned char *data, quint16 wLength, int &errcnt, QString &errstr) const;
    void disableCS(quint8 channel, int &errcnt, QString &errstr) const;
    void disableSPIDelays(quint8 channel, int &errcnt, QString &errstr) const;
    bool getGPIO1(int &errcnt, QString &errstr) const;
    bool getGPIO2(int &errcnt, QString &errstr) const;
    bool getGPIO3(int &errcnt, QString &errstr) const;
    bool getGPIO4(int &errcnt, QString &errstr) const;
    bool getGPIO5(int &errcnt, QString &errstr) const;
    bool getGPIO6(int &errcnt, QString &errstr) const;
    bool getGPIO7(int &errcnt, QString &errstr) const;
    bool getGPIO8(int &errcnt, QString &errstr) const;
    bool getGPIO9(int &errcnt, QString &errstr) const;
    bool getGPIO10(int &errcnt, QString &errstr) const;
    quint8 getMajorRelease(int &errcnt, QString &errstr) const;
    QString getManufacturer(int &errcnt, QString &errstr) const;
    quint8 getMaxPower(int &errcnt, QString &errstr) const;
    quint8 getMinorRelease(int &errcnt, QString &errstr) const;
    QString getProduct(int &errcnt, QString &errstr) const;
    QString getSerial(int &errcnt, QString &errstr) const;
    SPIDelays getSPIDelays(quint8 channel, int &errcnt, QString &errstr) const;
    SPIMode getSPIMode(quint8 channel, int &errcnt, QString &errstr) const;
    bool isOpen() const;
    void reset(int &errcnt, QString &errstr) const;
    void selectCS(quint8 channel, int &errcnt, QString &errstr) const;
    void setGPIO1(bool value, int &errcnt, QString &errstr) const;
    void setGPIO2(bool value, int &errcnt, QString &errstr) const;
    void setGPIO3(bool value, int &errcnt, QString &errstr) const;
    void setGPIO4(bool value, int &errcnt, QString &errstr) const;
    void setGPIO5(bool value, int &errcnt, QString &errstr) const;
    void setGPIO6(bool value, int &errcnt, QString &errstr) const;
    void setGPIO7(bool value, int &errcnt, QString &errstr) const;
    void setGPIO8(bool value, int &errcnt, QString &errstr) const;
    void setGPIO9(bool value, int &errcnt, QString &errstr) const;
    void setGPIO10(bool value, int &errcnt, QString &errstr) const;

    void close();
    int open(quint16 vid, quint16 pid, const QString &serial);

    static QStringList listDevices(quint16 vid, quint16 pid, int &errcnt, QString &errstr);
};

#endif  // CP2130_H
