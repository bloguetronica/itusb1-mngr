This directory contains the QT project required for compiling ITUSB1 Manager.
A list of relevant files follows:
– aboutdialog.cpp;
– aboutdialog.h;
– aboutdialog.ui;
– devicewindow.cpp;
– devicewindow.h;
– devicewindow.ui;
– icons/active64.png
– icons/greyed64.png
– icons/selected64.png
– informationdialog.cpp;
– informationdialog.h;
– informationdialog.ui;
– itusb1device.cpp;
– itusb1device.h;
– itusb1-mngr.pro;
– libusb-extra.c;
– libusb-extra.h;
– main.cpp;
– mainwindow.cpp;
– mainwindow.h;
– mainwindow.ui;
– resources.qrc.

In order to compile successfully, you must have the packages
"build-essential", "libusb-1.0-0-dev" and "qt5-default" already installed.
Given that, if you wish to simply compile, change your working directory to
the current one on a terminal window, and invoke "qmake", followed by "make"
or "make all". Notice that invoking "qmake" is necessary to generate the
Makefile, but only needs to be done once.

It may be necessary to undo any previous operations. Invoking "make clean"
will delete all object code generated during earlier compilations. However,
the previously generated binary is preserved.
