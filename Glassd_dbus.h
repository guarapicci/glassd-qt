#ifndef GLASSD_DBUS_H
#define GLASSD_DBUS_H

// #include <QtCore>
#include <QObject>
#include <QDBusAbstractAdaptor>

#include "Glassd.h"
extern "C" {
    #include <stdio.h>
}
class Glassd_dbus: public QDBusAbstractAdaptor{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.guarapicci.glassd")
public slots:
    int get_gesture_handler_status()
    {
        return static_cast<int>(target->get_status());
    } //current Glassd status
    int get_current_tracking_mode() //current gesture tracking mode
    {
        return static_cast<int>(target->get_current_tracking_mode());
    }
    QString get_application_canonical_name(){return QString("i.dont.have.it.lol");} //canonical name for the current focused application on the desktop, AKA "desktop file name" or "dbus service name"
    void set_application_canonical_name(const QString new_name){fprintf(stderr, "omniglass was informed that the current active application has changed.\n");}


signals:
    void tracking_mode_changed(int new_mode);
    void gesture_handler_changed_status(int new_status);

private slots:
    void transmit_tracking_mode(Glassd::tracking_mode mode){
        emit tracking_mode_changed(static_cast<int>(mode));
    }
    void transmit_handler_status(Glassd::status status){
        emit tracking_mode_changed(static_cast<int>(status));
    }
private:
    Glassd *target;


public:
    Glassd_dbus(Glassd *parent): QDBusAbstractAdaptor(parent){
        target = parent;
        QObject::connect(parent, &Glassd::touchpad_mode_switched, this, &Glassd_dbus::transmit_tracking_mode);
        // QObject::connect(parent, &Glassd::get_status, this, &Glassd_dbus::get_gesture_handler_status);
    }

};

#endif // GLASSD_DBUS_H
