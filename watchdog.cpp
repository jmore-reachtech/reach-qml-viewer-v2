#include "watchdog.h"

Watchdog::Watchdog(QObject *parent, bool startWatchdog) :
    QObject(parent)
  ,m_timer(new QTimer(this))
{
    m_started = false;

    if (startWatchdog && start())
    {
        connect(m_timer,SIGNAL(timeout()),this,SLOT(keepAlive()));
        m_timer->start(30000);  //kick the dog every 30 seconds
    }
}

Watchdog::~Watchdog()
{
    delete m_timer;
}

bool Watchdog::start()
{
    //If the watchdog is already started then don't start
    if (m_started)
    {
        qDebug() << "[QML] watchdog error: watchdog has already been started";
        emit watchdogError(QString("Watchdog error: watchdog has already been started."));
        return false;
    }

    qDebug() << "[QML] starting watchdog timer";
    m_started = true;
    return true;
}

bool Watchdog::isStarted()
{
    return m_started;
}

void Watchdog::stop()
{
    m_started = false;

    //Shut down the timer if it is running
    if (m_timer->isActive())
        m_timer->stop();

    qDebug() << "[QML] stopped watchdog timer";
}

bool Watchdog::setInterval(int interval)
{
    if (interval < 30 || interval > 128)
    {
        qDebug() << "[QML] Watchdog error : set interval failed.  Interval must be >= 30 seconds and <= 128 seconds.";
        emit watchdogError("Interval must be >= 30 seconds and <= 128 seconds.");
        return false;
    }

    m_interval = interval;

    //Check to see if the timer is running
    if (m_timer->isActive())
    {
        m_timer->stop();
        m_timer->start(static_cast<int>(interval/2) * 1000);
    }

    qDebug() << "[QML] watchdog set interval: " << interval;
    return true;
}

int Watchdog::getInterval()
{
    return m_interval;
}

bool Watchdog::keepAlive()
{
    int size = 1;
    qDebug() << "[QML] watchdog kicked.";
    return size;
}

bool Watchdog::lastBootByWatchDog()
{
    return false;
}
