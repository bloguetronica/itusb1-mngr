#!/bin/sh

echo Rolling back configurations...
rm -f /etc/udev/rules.d/71-bgtn-itusb1.rules
service udev restart
echo Removing application...
rm -f /usr/local/share/applications/itusb1-mngr.desktop
rmdir --ignore-fail-on-non-empty /usr/local/share/applications
rm -f /usr/local/share/icons/hicolor/128x128/apps/itusb1-mngr.png
rmdir --ignore-fail-on-non-empty /usr/local/share/icons/hicolor/128x128/apps
rmdir --ignore-fail-on-non-empty /usr/local/share/icons/hicolor/128x128
rmdir --ignore-fail-on-non-empty /usr/local/share/icons/hicolor
rmdir --ignore-fail-on-non-empty /usr/local/share/icons
rm -f /usr/local/bin/itusb1-mngr
echo Removing source code files...
rm -rf /usr/local/src/itusb1-mngr
echo Done!
