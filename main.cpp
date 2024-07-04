#include <QGuiApplication>
#include <QTimer>
#include <QObject>

#include <QtQml>
#include <QtQuick>

#include <QtDBus>

// #include <QtQuick/QQuickWindow>
// #include <QtQml/>

#include "Glassd.h"
#include "Glassd_dbus.h"

int main(int argc, char *argv[])
{



    QGuiApplication a(argc, argv);

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

    QQmlEngine engine;

    QQmlComponent viz(&engine);
    QQuickWindow::setDefaultAlphaBuffer(true);
    viz.loadUrl(QUrl("qrc:/qml/viz_default.qml"));


    glassd_timer.start();

    QQuickView view;
    view.connect(view.engine(), &QQmlEngine::quit, &a, &QCoreApplication::quit);
    view.setSource(QUrl("qrc:/qml/viz_default.qml"));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.show();

    // glassd_timer
        return a.exec();
}

