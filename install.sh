#!/bin/sh

echo Obtaining required packages...
apt-get -qq update
apt-get -qq install build-essential
apt-get -qq install libusb-1.0-0-dev
apt-get -qq install qt5-default
echo Copying source code files...
mkdir -p /usr/local/src/itusb1-mngr/icons
cp -f src/aboutdialog.cpp /usr/local/src/itusb1-mngr/.
cp -f src/aboutdialog.h /usr/local/src/itusb1-mngr/.
cp -f src/aboutdialog.ui /usr/local/src/itusb1-mngr/.
cp -f src/devicewindow.cpp /usr/local/src/itusb1-mngr/.
cp -f src/devicewindow.h /usr/local/src/itusb1-mngr/.
cp -f src/devicewindow.ui /usr/local/src/itusb1-mngr/.
cp -f src/icons/active64.png /usr/local/src/itusb1-mngr/icons/.
cp -f src/icons/greyed64.png /usr/local/src/itusb1-mngr/icons/.
cp -f src/icons/selected64.png /usr/local/src/itusb1-mngr/icons/.
cp -f src/informationdialog.cpp /usr/local/src/itusb1-mngr/.
cp -f src/informationdialog.h /usr/local/src/itusb1-mngr/.
cp -f src/informationdialog.ui /usr/local/src/itusb1-mngr/.
cp -f src/itusb1-mngr.pro /usr/local/src/itusb1-mngr/.
cp -f src/itusb1device.cpp /usr/local/src/itusb1-mngr/.
cp -f src/itusb1device.h /usr/local/src/itusb1-mngr/.
cp -f src/libusb-extra.c /usr/local/src/itusb1-mngr/.
cp -f src/libusb-extra.h /usr/local/src/itusb1-mngr/.
cp -f src/main.cpp /usr/local/src/itusb1-mngr/.
cp -f src/mainwindow.cpp /usr/local/src/itusb1-mngr/.
cp -f src/mainwindow.h /usr/local/src/itusb1-mngr/.
cp -f src/mainwindow.ui /usr/local/src/itusb1-mngr/.
cp -f src/resources.qrc /usr/local/src/itusb1-mngr/.
echo Building and installing application...
cd /usr/local/src/itusb1-mngr
qmake
make all clean
mv -f itusb1-mngr /usr/local/bin/.
echo Applying configurations...
cat > /etc/udev/rules.d/70-bgtn-itusb1.rules << EOF
SUBSYSTEM=="usb", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="8c96", MODE="0666"
SUBSYSTEM=="usb_device", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="8c96", MODE="0666"
EOF
service udev restart
echo Done!
