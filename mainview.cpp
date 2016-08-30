#include "mainview.h"

MainView::MainView(QWindow *parent) : QQuickView(parent)
{
    /* set the viewer background to transparent */
    QColor color;
    color.setRedF(0.0);
    color.setGreenF(0.0);
    color.setBlueF(0.0);
    color.setAlphaF(0.0);

    setColor(color);
    setClearBeforeRendering(true);
}
