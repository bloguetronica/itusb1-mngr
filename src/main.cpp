/* ITUSB1 Manager - Version 3.3 for Debian Linux
   Copyright (c) 2020-2022 Samuel Lourenço

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   with this program.  If not, see <https://www.gnu.org/licenses/>.


   Please feel free to contact me via e-mail: samuel.fmlourenco@gmail.com */


// Includes
#include <QApplication>
#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTranslator translator;
    if (!translator.load("itusb1-mngr_" + QLocale::system().name(), ":/translations/translations")) {  // It the locale translation does not exist or cannot be loaded
        translator.load("itusb1-mngr_en_US", ":/translations/translations");  // Fall back to the en-US translation
    }
    QCoreApplication::installTranslator(&translator);
    MainWindow w;
    w.show();
    return a.exec();
}
