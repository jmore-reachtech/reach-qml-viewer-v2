#include <QGuiApplication>
#include "mainview.h"
#include "maincontroller.h"
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
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
        qDebug() << "[QMLVIEWER] using system defined settings file:" << SYSTEM_SETTINGS_FILE;
        settingsFile.setFile(SYSTEM_SETTINGS_FILE);
    }

    QFile jsonFile;
    QString json;
    jsonFile.setFileName(settingsFile.filePath().toLatin1());
    if (jsonFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
          json = jsonFile.readAll();
          jsonFile.close();
    }
    else
    {
        qDebug() << "[QMLVIEWER] Could not open Settings file: " << settingsFile.filePath().toLatin1();
        QGuiApplication::quit();
    }

    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());;
    QJsonObject jsonObj;

    if(!doc.isNull())
    {
        if(doc.isObject())
        {
            jsonObj = doc.object();
        }
        else
        {
            qDebug() << "[QMLVIEWER] Settings file: Document is not an object.";
            QGuiApplication::quit();
        }
    }
    else
    {
        qDebug() << "[QMLVIEWER] Invalid JSON:\n" << json;
        QGuiApplication::quit();
    }

    /* set the viewer background to transparent */
    QColor color;
    color.setRedF(0.0);
    color.setGreenF(0.0);
    color.setBlueF(0.0);
    color.setAlphaF(0.0);
    view.setColor(color);
    view.setClearBeforeRendering(true);

    MainController controller(&view, jsonObj.value("tcp_servers").toArray(), jsonObj.value("serial_port_servers").toArray(),
                              jsonObj.value("translate_file").toString(), jsonObj.value("enable_ack").toBool(),
                              jsonObj.value("enable_heartbeat").toBool(), jsonObj.value("heartbeat_interval").toInt(),
                              jsonObj.value("screensaver_timeout").toInt(), jsonObj.value("screen_original_brigtness").toInt(),
                              jsonObj.value("screen_dim_brigtness").toInt());


    //If there is trouble opening up a serial port don't open the main qml file
    if (controller.getStartUpError().length() == 0)
    {
        qDebug() << "[QMLVIEWER] Loading main qml file:" << jsonObj.value("main_view").toString();
        view.setSource(QUrl::fromLocalFile(jsonObj.value("main_view").toString()));
        view.setResizeMode(QQuickView::SizeRootObjectToView);

        if (jsonObj.value("full_screen").toBool()) {
            view.showFullScreen();
        }

        if (jsonObj.value("hide_curosr").toBool()) {
            view.setCursor(QCursor( Qt::BlankCursor ));
        }
    }
    return app.exec();
}
