/* CP2130 class for Qt - Version 1.0.0
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
#include <QString>
#include <QStringList>
#include <libusb-1.0/libusb.h>

class CP2130
{
private:
    libusb_context *context_;
    libusb_device_handle *handle_;
    bool kernelAttached_;

public:
    // Class definitions
    static const quint16 VID = 0x10C4;     // Default USB vendor ID
    static const quint16 PID = 0x87A0;     // Default USB product ID
    static const quint8 READ = 0x00;       // Read command, to be used with bulkTransfer()
    static const quint8 WRITE = 0x01;      // Write command, to be used with bulkTransfer()
    static const quint8 WRITEREAD = 0x02;  // WriteRead command, to be used with bulkTransfer()
    static const quint8 READWRTR = 0x04;   // ReadWithRTR command, to be used with bulkTransfer()

    // The following masks are applicable to the value returned by getLockWord()
    static const quint16 LWVID = 0x0001;      // Mask for the vendor ID lock bit
    static const quint16 LWPID = 0x0002;      // Mask for the product ID lock bit
    static const quint16 LWMAXPOW = 0x0004;   // Mask for the max power lock bit
    static const quint16 LWPOWMODE = 0x0008;  // Mask for the power mode lock bit
    static const quint16 LWREL = 0x0010;      // Mask for the release version lock bit
    static const quint16 LWMANUF = 0x0060;    // Mask for the manufacturer descriptor lock bits
    static const quint16 LWTRFPRIO = 0x0080;  // Mask for the transfer priority lock bit
    static const quint16 LWUSBCFG = 0x009F;   // Mask for the USB config lock bits
    static const quint16 LWPROD = 0x0300;     // Mask for the product descriptor lock bits
    static const quint16 LWSER = 0x0400;      // Mask for the serial descriptor lock bit
    static const quint16 LWPINCFG = 0x0800;   // Mask for the pin config lock bit
    static const quint16 LWALL = 0x0FFF;      // Mask for all but the reserved lock bits

    // The following values are applicable to SPIMode/configureSPIMode()/getSPIMode()
    static const bool CSMODEOD = false;    // Boolean corresponding to chip select open-drain mode
    static const bool CSMODEPP = true;     // Boolean corresponding to chip select push-pull mode
    static const quint8 CFRQ12M = 0x00;    // Value corresponding to a clock frequency of 12MHz
    static const quint8 CFRQ6M = 0x01;     // Value corresponding to a clock frequency of 6MHz
    static const quint8 CFRQ3M = 0x02;     // Value corresponding to a clock frequency of 3MHz
    static const quint8 CFRQ1500K = 0x03;  // Value corresponding to a clock frequency of 1.5MHz
    static const quint8 CFRQ750K = 0x04;   // Value corresponding to a clock frequency of 750KHz
    static const quint8 CFRQ375K = 0x05;   // Value corresponding to a clock frequency of 375KHz
    static const quint8 CFRQ1875= 0x06;    // Value corresponding to a clock frequency of 187.5KHz
    static const quint8 CFRQ938 = 0x07;    // Value corresponding to a clock frequency of 93.8KHz
    static const bool CPOL0 = false;       // Boolean corresponding to CPOL = 0
    static const bool CPOL1 = true;        // Boolean corresponding to CPOL = 1
    static const bool CPHA0 = false;       // Boolean corresponding to CPHA = 0
    static const bool CPHA1 = true;        // Boolean corresponding to CPHA = 1

    // The following values are applicable to PinConfig/getPinConfig()/writePinConfig()
    static const quint8 PCIN = 0x00;         // GPIO as input
    static const quint8 PCOUTOD = 0x01;      // GPIO as open-drain output
    static const quint8 PCOUTPP = 0x02;      // GPIO as push-pull output
    static const quint8 PCCS = 0x03;         // GPIO as chip select
    static const quint8 PCNRTR = 0x04;       // GPIO as !RTR input, only applicable to GPIO.3
    static const quint8 PCRTR = 0x05;        // GPIO as RTR input, only applicable to GPIO.3
    static const quint8 PCEVTCNTRRE = 0x04;  // GPIO as EVTCNTR rising edge input, only applicable to GPIO.4 - Also applicable to getEventCounter()/setEventCounter()
    static const quint8 PCEVTCNTRFE = 0x05;  // GPIO as EVTCNTR falling edge input, only applicable to GPIO.4 - Also applicable to getEventCounter()/setEventCounter()
    static const quint8 PCEVTCNTRNP = 0x06;  // GPIO as EVTCNTR negative pulse input, only applicable to GPIO.4 - Also applicable to getEventCounter()/setEventCounter()
    static const quint8 PCEVTCNTRPP = 0x07;  // GPIO as EVTCNTR positive pulse input, only applicable to GPIO.4 - Also applicable to getEventCounter()/setEventCounter()
    static const quint8 PCCLKOUT = 0x04;     // GPIO as CLKOUT push-pull output, only applicable to GPIO.5
    static const quint8 PCSPIACT = 0x04;     // GPIO as SPIACT push-pull output, only applicable to GPIO.8
    static const quint8 PCSSPND = 0x04;      // GPIO as SUSPEND push-pull output, only applicable to GPIO.9
    static const quint8 PCNSSPND = 0x04;     // GPIO as !SUSPEND push-pull output, only applicable to GPIO.10
    static const quint16 BMSCK = 0x0001;     // Bitmap for the SCK pin
    static const quint16 BMMISO = 0x0002;    // Bitmap for the MISO pin
    static const quint16 BMMOSI = 0x0004;    // Bitmap for the MOSI pin
    static const quint16 BMGPIO0 = 0x0008;   // Bitmap for the GPIO.0 pin - Also applicable to getGPIOs()/setGPIOs()
    static const quint16 BMGPIO1 = 0x0010;   // Bitmap for the GPIO.1 pin - Also applicable to getGPIOs()/setGPIOs()
    static const quint16 BMGPIO2 = 0x0020;   // Bitmap for the GPIO.2 pin - Also applicable to getGPIOs()/setGPIOs()
    static const quint16 BMGPIO3 = 0x0040;   // Bitmap for the GPIO.3 pin - Also applicable to getGPIOs()/setGPIOs()
    static const quint16 BMGPIO4 = 0x0080;   // Bitmap for the GPIO.4 pin - Also applicable to getGPIOs()/setGPIOs()
    static const quint16 BMGPIO5 = 0x0100;   // Bitmap for the GPIO.5 pin - Also applicable to getGPIOs()/setGPIOs()
    static const quint16 BMVPP = 0x0200;     // Bitmap for the VPP pin
    static const quint16 BMGPIO6 = 0x0400;   // Bitmap for the GPIO.6 pin - Also applicable to getGPIOs()/setGPIOs()
    static const quint16 BMGPIO7 = 0x0800;   // Bitmap for the GPIO.7 pin - Also applicable to getGPIOs()/setGPIOs()
    static const quint16 BMGPIO8 = 0x1000;   // Bitmap for the GPIO.8 pin - Also applicable to getGPIOs()/setGPIOs()
    static const quint16 BMGPIO9 = 0x2000;   // Bitmap for the GPIO.9 pin - Also applicable to getGPIOs()/setGPIOs()
    static const quint16 BMGPIO10 = 0x4000;  // Bitmap for the GPIO.10 pin - Also applicable to getGPIOs()/setGPIOs()
    static const quint16 BMGPIOS = 0x7DF8;   // Bitmap for all GPIO pins
    static const quint16 BMENABLE = 0x8000;  // Bitmap for suspend mode and level enable, only applicable to PinConfig.sspndmode (suspend pin mode bitmap)

    // The following values are applicable to USBConfig/getUSBConfig()/writeUSBConfig()
    static const quint8 PMBUSREGEN = 0x00;   // Value corresponding to USB bus-powered mode with voltage regulator enabled
    static const quint8 PMBUSREGDIS = 0x01;  // Value corresponding to USB bus-powered mode with voltage regulator disabled
    static const quint8 PMSELFREGEN = 0x02;  // Value corresponding to USB self-powered mode with voltage regulator enabled
    static const quint8 PRIOREAD = 0x00;     // Value corresponding to data transfer with high priority read
    static const quint8 PRIOWRITE = 0x01;    // Value corresponding to data transfer with high priority write

    struct EventCounter {
        bool overflow;  // Overflow flag
        quint8 mode;    // GPIO.4/EVTCNTR pin mode (see the values applicable to PinConfig/getPinConfig()/writePinConfig())
        quint16 value;  // Count value (big-endian)

        bool operator ==(const EventCounter &other) const;
        bool operator !=(const EventCounter &other) const;
    };

    struct PinConfig {
        quint8 gpio0;       // GPIO.0 pin config
        quint8 gpio1;       // GPIO.1 pin config
        quint8 gpio2;       // GPIO.2 pin config
        quint8 gpio3;       // GPIO.3 pin config
        quint8 gpio4;       // GPIO.4 pin config
        quint8 gpio5;       // GPIO.5 pin config
        quint8 gpio6;       // GPIO.6 pin config
        quint8 gpio7;       // GPIO.7 pin config
        quint8 gpio8;       // GPIO.8 pin config
        quint8 gpio9;       // GPIO.9 pin config
        quint8 gpio10;      // GPIO.10 pin config
        quint16 sspndlvl;   // Suspend pin level bitmap (big-endian - see Silicon Labs AN792 for details)
        quint16 sspndmode;  // Suspend pin mode bitmap (big-endian - see Silicon Labs AN792 for details)
        quint16 wkupmask;   // Wakeup pin mask bitmap (big-endian - see Silicon Labs AN792 for details)
        quint16 wkupmatch;  // Wakeup pin match bitmap (big-endian - see Silicon Labs AN792 for details)
        quint8 divider;     // GPIO.5/!CS5/CLKOUT OTP ROM clock divider value (see Silicon Labs AN792 for details)

        bool operator ==(const PinConfig &other) const;
        bool operator !=(const PinConfig &other) const;
    };

    struct SiliconVersion {
        quint8 maj;  // Major read-only version
        quint8 min;  // Minor read-only version

        bool operator ==(const SiliconVersion &other) const;
        bool operator !=(const SiliconVersion &other) const;
    };

    struct SPIDelays {
        bool cstglen;       // CS toggle enable
        bool prdasten;      // Pre-deassert delay enable
        bool pstasten;      // Post-assert delay enable
        bool itbyten;       // Inter-byte delay enable
        quint16 prdastdly;  // Pre-deassert delay (10us units, big-endian)
        quint16 pstastdly;  // Post-assert delay (10us units, big-endian)
        quint16 itbytdly;   // Inter-byte delay (10us units, big-endian)

        bool operator ==(const SPIDelays &other) const;
        bool operator !=(const SPIDelays &other) const;
    };

    struct SPIMode {
        bool csmode;  // Chip select mode (false for open drain, true for push-pull)
        quint8 cfrq;  // Clock frequency
        bool cpol;    // Clock polarity
        bool cpha;    // Clock phase

        bool operator ==(const SPIMode &other) const;
        bool operator !=(const SPIMode &other) const;
    };

    struct USBConfig {
        quint16 vid;     // Vendor ID (little-endian)
        quint16 pid;     // Product ID (little-endian)
        quint8 majrel;   // Major release version
        quint8 minrel;   // Minor release version
        quint8 maxpow;   // Maximum consumption current (raw value in 2mA units)
        quint8 powmode;  // Power mode
        quint8 trfprio;  // Transfer priority

        bool operator ==(const USBConfig &other) const;
        bool operator !=(const USBConfig &other) const;
    };

    CP2130();
    ~CP2130();

    void bulkTransfer(quint8 endpoint, unsigned char *data, int length, int *transferred, int &errcnt, QString &errstr) const;
    void configureSPIDelays(quint8 channel, const SPIDelays &delays, int &errcnt, QString &errstr) const;
    void configureSPIMode(quint8 channel, const SPIMode &mode, int &errcnt, QString &errstr) const;
    void controlTransfer(quint8 bmRequestType, quint8 bRequest, quint16 wValue, quint16 wIndex, unsigned char *data, quint16 wLength, int &errcnt, QString &errstr) const;
    void disableCS(quint8 channel, int &errcnt, QString &errstr) const;
    void disableSPIDelays(quint8 channel, int &errcnt, QString &errstr) const;
    void enableCS(quint8 channel, int &errcnt, QString &errstr) const;
    quint8 getClockDivider(int &errcnt, QString &errstr) const;
    bool getCS(quint8 channel, int &errcnt, QString &errstr) const;
    EventCounter getEventCounter(int &errcnt, QString &errstr) const;
    quint8 getFIFOThreshold(int &errcnt, QString &errstr) const;
    bool getGPIO0(int &errcnt, QString &errstr) const;
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
    quint16 getGPIOs(int &errcnt, QString &errstr) const;
    quint16 getLockWord(int &errcnt, QString &errstr) const;
    QString getManufacturerDesc(int &errcnt, QString &errstr) const;
    PinConfig getPinConfig(int &errcnt, QString &errstr) const;
    QString getProductDesc(int &errcnt, QString &errstr) const;
    QString getSerialDesc(int &errcnt, QString &errstr) const;
    SiliconVersion getSiliconVersion(int &errcnt, QString &errstr) const;
    SPIDelays getSPIDelays(quint8 channel, int &errcnt, QString &errstr) const;
    SPIMode getSPIMode(quint8 channel, int &errcnt, QString &errstr) const;
    USBConfig getUSBConfig(int &errcnt, QString &errstr) const;
    bool isOpen() const;
    bool isOTPBlank(int &errcnt, QString &errstr) const;
    bool isOTPLocked(int &errcnt, QString &errstr) const;
    bool isRTRActive(int &errcnt, QString &errstr) const;
    void lockOTP(int &errcnt, QString &errstr) const;
    void reset(int &errcnt, QString &errstr) const;
    void selectCS(quint8 channel, int &errcnt, QString &errstr) const;
    void setClockDivider(quint8 value, int &errcnt, QString &errstr) const;
    void setEventCounter(const EventCounter &evcntr, int &errcnt, QString &errstr) const;
    void setFIFOThreshold(quint8 threshold, int &errcnt, QString &errstr) const;
    void setGPIO0(bool value, int &errcnt, QString &errstr) const;
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
    void setGPIOs(quint16 bmValues, quint16 bmMask, int &errcnt, QString &errstr) const;
    void stopRTR(int &errcnt, QString &errstr) const;
    void writeLockWord(quint16 word, int &errcnt, QString &errstr) const;
    void writeManufacturerDesc(const QString &manufacturer, int &errcnt, QString &errstr) const;
    void writePinConfig(const PinConfig &config, int &errcnt, QString &errstr) const;
    void writeProductDesc(const QString &product, int &errcnt, QString &errstr) const;
    void writeSerialDesc(const QString &serial, int &errcnt, QString &errstr) const;
    void writeUSBConfig(const USBConfig &config, quint8 mask, int &errcnt, QString &errstr) const;

    void close();
    int open(quint16 vid, quint16 pid, const QString &serial);

    static QStringList listDevices(quint16 vid, quint16 pid, int &errcnt, QString &errstr);
};

#endif  // CP2130_H
