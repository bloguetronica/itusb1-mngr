This directory contains the QT project required for compiling ITUSB1 Manager.
A list of relevant files follows:
– aboutdialog.cpp;
– aboutdialog.h;
– aboutdialog.ui;
– cp2130.cpp;
– cp2130.h;
– datalog.cpp;
– datalog.h;
– datapoint.h;
– devicewindow.cpp;
– devicewindow.h;
– devicewindow.ui;
– icons/active64.png;
– icons/greyed64.png;
– icons/itusb1-mngr.png;
– icons/selected64.png;
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
– metrics.cpp;
– metrics.h;
– misc/itusb1-mngr.desktop;
– resources.qrc;
– translations/itusb1-mngr_en.qm;
– translations/itusb1-mngr_en.ts;
– translations/itusb1-mngr_en_US.qm;
– translations/itusb1-mngr_en_US.ts;
– translations/itusb1-mngr_pt.qm;
– translations/itusb1-mngr_pt.ts;
– translations/itusb1-mngr_pt_PT.qm;
– translations/itusb1-mngr_pt_PT.ts.

In order to compile successfully, you must have the packages
"build-essential", "libusb-1.0-0-dev" and "qt5-default" (or "qtbase5-dev"
instead, in many recent distributions) already installed. Given that, if you
wish to simply compile, change your working directory to the current one on a
terminal window, and invoke "qmake", followed by "make" or "make all". Notice
that invoking "qmake" is necessary to generate the Makefile, but only needs to
be done once.

You can also install using make. To do so, after invoking "qmake", you should
simply run "sudo make install". If you wish to force a rebuild before the
installation, then you must invoke "sudo make clean install" instead.

It may be necessary to undo any previous operations. Invoking "make clean"
will delete all object code generated during earlier compilations. However,
the previously generated binary is preserved. It is important to note that it
is possible to undo previous installation operations as well, by invoking
"sudo make uninstall". Such approach is not recommended, though.

P.S.:
Notice that any make operation containing the actions "install" or "uninstall"
(e.g. "make install" or "make uninstall") requires root permissions, or in
other words, must be run with sudo.
