#!/bin/sh

echo Rolling back configurations...
rm -f /etc/udev/rules.d/70-bgtn-itusb1.rules
service udev restart
echo Removing application...
rm -f /usr/local/bin/itusb1-mngr
echo Removing source code files...
rm -rf /usr/local/src/itusb1-mngr
echo Done!
