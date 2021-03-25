This package contains all the necessary files and scripts to install ITUSB1
Manager (a standalone graphical application that is used to control the ITUSB1
USB Test Switch directly). The scripts included here are compatible with most
32-bit and 64-bit Debian based operating systems (e.g. Kubuntu, etc). Prior to
installation, you must certify that your system is Debian based (or at least
uses apt-get) and that you have an active Internet connection.

A list of the included scripts follows:
– install.sh;
– uninstall.sh.

In order to compile and install ITUSB1 Manager for the first time, all you
have to do is to run "install.sh" by invoking "sudo ./install.sh" on a
terminal window, after changing your working directory to the current one.
This script will first obtain and install the required "build-essential",
"libusb-1.0-0-dev" and "qt5-default" packages (if they are not installed yet).
Then it will compile the application and move it to "/usr/local/bin/".

Conversely, to uninstall, you should run "uninstall.sh" by invoking "sudo
./uninstall.sh", again on a terminal window after making sure that your
working directory is this one. This will delete the previously installed
application and source code. However it won't remove the "build-essential" and
"qt5-default" packages, since they could be already installed before the first
installation.

P.S.:
Since the application was designed having KDE in mind, you should get the best
rendering on Kubuntu and other Debian based systems that employ the above
desktop environment. However, the application can be perfectly used on systems
that have other desktop environments (e.g. Gnome, Xfce).
