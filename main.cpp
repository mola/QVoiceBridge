#include "mainwindow.h"

#include <QApplication>
#include <QSettings>

int  main(int argc, char *argv[])
{
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QCoreApplication::setOrganizationName("Datall");
    QCoreApplication::setApplicationName("QVoiceBridge");
    QCoreApplication::setApplicationVersion("0.1.0");

    QApplication  a(argc, argv);
    MainWindow    w;

    w.show();

    return a.exec();
}
