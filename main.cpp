#include <QGuiApplication>
#include "mainview.h"
#include "maincontroller.h"
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include "applicationsettings.h"
#include "systemdefs.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName("Reach Technology");
    QGuiApplication::setOrganizationDomain("reachtech.com");
    QGuiApplication::setApplicationName("Qml-Viewer");
    QGuiApplication::setApplicationVersion(APP_VERSION);

    MainView view;

    QFileInfo settingsFile;
    QString sb(QGuiApplication::applicationDirPath());

    QStringList args = app.arguments();
    foreach (QString item, args) {
        if(item == "--version" || item == "-v") {
            qDebug() << "QML Viewer " << APP_VERSION;
            return 0;
        }
    }

    if (args.count() == 2)
    {
        sb = args[1];
        QFileInfo f(sb);
        sb = f.path();
        qDebug() << sb << endl;
    }

    sb.append(QDir::separator());
    sb.append("settings.json");

    // check to see if we have a settings file where we started from
    // if not fall back to system hard coded path
    QFileInfo file(sb.toLatin1());
    if (file.exists()) {
        qDebug() << "[QMLVIEWER] using local settings file:" << sb;
        settingsFile.setFile(file.filePath());
    } else {
            if (QFile::copy(":/settings.json", file.filePath()))
            {
                 qDebug() << "[QMLVIEWER] created a settings.json file:" << file.filePath();
                 settingsFile.setFile(file.filePath());
            }
            else
                qDebug() << "[QMLVIEWER] error creating a settings.json file:" << file.filePath();
    }

    MainController controller(&view, settingsFile.filePath().toLatin1());

    /* fix the mainview path in case someone copied it from the module */
    controller.setMainViewPath(controller.getMainViewPath().replace("/application/src/", ""));

    //If there is trouble opening up a serial port don't open the main qml file
    if (controller.getStartUpError().length() == 0)
    {
        qDebug() << "[QMLVIEWER] Loading main qml file:" << controller.getMainViewPath();
        view.setSource(QUrl::fromLocalFile(controller.getMainViewPath()));
        view.setResizeMode(QQuickView::SizeRootObjectToView);

        if (controller.showFullScreen()) {
            view.showFullScreen();
        }

        if (controller.hideCursor()) {
            view.setCursor(QCursor( Qt::BlankCursor ));
        }
    }

    return app.exec();
}
