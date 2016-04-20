#ifndef SETTINGS_H
#define SETTINGS_H

#include "systemdefs.h"
#include <QObject>
#include <QVariant>

class Settings : public QObject
{
    Q_OBJECT
public:
    explicit Settings(QObject *parent = 0);
    ~Settings();

signals:

public slots:
    QVariant getValue(const QString & key, const QVariant & defaultValue = QVariant()) const;
    void setValue(const QString & key, const QVariant & value);
    void remove(const QString & key );

};

#endif // SETTINGS_H
