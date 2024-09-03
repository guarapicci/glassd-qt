#ifndef VIZ_WRANGLER_H
#define VIZ_WRANGLER_H

#include <QObject>

#include "Glassd.h"

class Viz_wrangler : public QObject
{
    Q_OBJECT
public:
    Viz_wrangler(QObject *parent = nullptr):  QObject(parent) {};
    // void cycle_visualization();

private:

signals:
};

#endif // VIZ_WRANGLER_H
