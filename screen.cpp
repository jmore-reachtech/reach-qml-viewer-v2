#include "screen.h"
#include <QDebug>

Screen::Screen(QQuickView *view, int screenSaverTimeout, int screenOriginalBrightness, int screenDimBrightness, QObject *parent) :
    QObject(parent)
  ,m_view(view)
  ,m_screenSaverTimer(new QTimer(this))
{
    m_screenSaverEnabled = false;
    m_screenSaverTimeout = screenSaverTimeout;\
    m_screenOriginalBrightness = screenOriginalBrightness;
    m_screenDimBrightness = screenDimBrightness;
    m_dim = false;

    if (m_screenSaverTimeout > 0)
    {
        m_screenSaverEnabled = true;
        //set the original brighness in case the device was shutdown in dim mode
        setBrightness(m_screenOriginalBrightness);
        connect(m_screenSaverTimer, SIGNAL(timeout()),this,SLOT(onScreenSaverTimerTimeout()));
        view->installEventFilter(this);
        m_screenSaverTimer->start(m_screenSaverTimeout * 60 * 1000);
    }
}


bool Screen::save(const QString &path)
{
    QImage image = m_view->grabWindow();
    return image.save(path, 0, 80);
}


void Screen::setOriginalBrightness()
{
    setBrightness(m_screenOriginalBrightness);
    m_dim = false;
    m_screenSaverTimer->stop();
    m_screenSaverTimer->start(m_screenSaverTimeout * 60 * 1000);
}


bool Screen::isDim()
{
    return m_dim;
}


bool Screen::isScreenSaverEnabled()
{
    return m_screenSaverEnabled;
}

int Screen::getScreenWidth()
{
    foreach (QScreen *screen, QGuiApplication::screens())
    {
        return screen->availableGeometry().width();
    }

    return 640;
}

int Screen::getScreenHeight()
{
    foreach (QScreen *screen, QGuiApplication::screens())
    {
        return screen->availableGeometry().height();
    }

    return 480;
}


void Screen::onScreenSaverTimerTimeout()
{
    setBrightness(m_screenDimBrightness);
    m_dim = true;
}


void Screen::setBrightness(int val)
{
    QFile brightness_file(BRIGHTNESS);
    brightness_file.open(QIODevice::ReadWrite);
    QTextStream out(&brightness_file);

    out << QString::number(val).toLatin1() << endl;
    brightness_file.close();
}


bool Screen::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    switch (event->type()) {
    case QEvent::KeyRelease:
    case QEvent::KeyPress:
    case QEvent::MouseButtonPress:
    case QEvent::TouchBegin:
    case QEvent::MouseMove:
        if (m_dim)
            return true;
    case QEvent::MouseButtonRelease:
        if (m_dim)
        {
            setOriginalBrightness();
            return true;
        }
        else
        {
            m_screenSaverTimer->stop();
            m_screenSaverTimer->start(m_screenSaverTimeout * 60 * 1000);
        }
        break;
    default:
        break;
    }

    return false;
}
