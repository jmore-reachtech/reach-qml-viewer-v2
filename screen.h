#ifndef SCREEN_H
#define SCREEN_H

#include <QObject>
#include <QImage>
#include <QQuickView>
#include <QTimer>
#include <QSettings>
#include "systemdefs.h"
#include <QFile>
#include <QTextStream>
#include <QScreen>
#include <QGuiApplication>

#define BRIGHTNESS "/sys/class/backlight/backlight.22/brightness"

class Screen : public QObject
{
    Q_OBJECT
public:
    explicit Screen(QQuickView *view, int screenSaverTimeout, int screenOriginalBrightness, int screenDimBrightness, QObject *parent = 0);

signals:

public slots:
     bool save(const QString &path);
     void setOriginalBrightness();
     bool isDim();
     bool isScreenSaverEnabled();
     int getScreenWidth();
     int getScreenHeight();


private slots:
    void onScreenSaverTimerTimeout();

private:
    QQuickView *m_view;
    QTimer *m_screenSaverTimer;
    int m_screenSaverTimeout;
    int m_screenOriginalBrightness;
    int m_screenDimBrightness;
    bool m_dim;
    bool m_screenSaverEnabled;

    bool eventFilter(QObject *obj, QEvent *event);
    void setBrightness(int val);
};

#endif // SCREEN_H
