#include <QCoreApplication>
#include <QTimer>
#include <QObject>

#include "glassd.h"

int main(int argc, char *argv[])
{

    QCoreApplication a(argc, argv);

    glassd touchpad_tracker = glassd();
    touchpad_tracker.init();
    if(touchpad_tracker.get_status() != glassd::status::ready){
        fprintf(stderr, "could not get glassd ready.");
        return -1;
    }
    QTimer glassd_timer;
    glassd_timer.setInterval(15);
    // glassd_timer.connect(glassd_timer.timeout, touchpad_tracker.step);
    QObject::connect(&glassd_timer, &QTimer::timeout , &touchpad_tracker, &glassd::step);
    glassd_timer.start();
    // glassd_timer
        return a.exec();
}

