#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QObject>
#include <QQuickView>

class MainView : public QQuickView
{
    Q_OBJECT

public:
    MainView(QWindow *parent = 0);
};

#endif // MAINVIEW_H
