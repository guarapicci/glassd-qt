#include <QCoreApplication>
#include <QTimer>
#include <QObject>

#include <QtDBus>

// #include <QtQuick/QQuickWindow>
// #include <QtQml/>

#include "Glassd.h"
#include "Glassd_dbus.h"

int main(int argc, char *argv[])
{



    QCoreApplication a(argc, argv);

    Glassd touchpad_tracker = Glassd();
    touchpad_tracker.init();
    if(touchpad_tracker.get_status() != Glassd::status::ready){
        fprintf(stderr, "could not get glassd ready.");
        return -1;
    }
    QTimer glassd_timer;
    glassd_timer.setInterval(15);
    QObject::connect(&glassd_timer, &QTimer::timeout, &touchpad_tracker, &Glassd::step);

    Glassd_dbus *glassdbus = new Glassd_dbus(&touchpad_tracker); //hook up a dbus adaptor to the glassd gesture handler core

    QDBusConnection dbus_connection = QDBusConnection::sessionBus();
    dbus_connection.registerObject("/glassd",&touchpad_tracker);
    dbus_connection.registerService("com.guarapicci.glassd");
    // QQuickWindow viz();
    // viz.

    glassd_timer.start();



    // glassd_timer
        return a.exec();
}

