#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <QObject>
#include <QTimer>
#include <QSettings>
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>
#include "systemdefs.h"

#define WATCHDOGDEV "/dev/watchdog"

class Watchdog : public QObject
{
    Q_OBJECT
public:
    explicit Watchdog(QObject *parent = 0, bool startWatchdog = false);
    ~Watchdog();

signals:
    void watchdogError(QString err);

public slots:
    bool start();
    bool isStarted();
    void stop();
    bool setInterval(int interval);
    int getInterval();
    bool keepAlive();
    bool lastBootByWatchDog();

private:
    int fd;
    bool m_started;
    QTimer *m_timer;
};

#endif // WATCHDOG_H
