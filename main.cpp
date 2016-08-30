#include <QGuiApplication>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include "mainview.h"
#include "maincontroller.h"
#include "systemdefs.h"
#include "applicationsettings.h"
#include <signal.h>

void unixSignalHandler(int signum) {
    qDebug("[QMLVIEWER] main.cpp::unixSignalHandler(). signal = %s", strsignal(signum));

    /*
     * Make sure your Qt application gracefully quits.
     * NOTE - purpose for calling qApp->exit(0):
     *      1. Forces the Qt framework's "main event loop `qApp->exec()`" to quit looping.
     *      2. Also emits the QGuiApplication::aboutToQuit() signal. This signal is used for cleanup code.
     */
    qApp->exit(0);
}


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName("Reach Technology");
    QGuiApplication::setOrganizationDomain("reachtech.com");
    QGuiApplication::setApplicationName("Qml-Viewer");
    QGuiApplication::setApplicationVersion(APP_VERSION);

    /* Set a signal handler for a quit or a control-c for clean up purposes */
    struct sigaction actInt, actQuit;
    memset((void*)&actInt, 0, sizeof(struct sigaction));
    actInt.sa_flags = SA_INTERRUPT;
    actInt.sa_handler = &unixSignalHandler;
    sigaction(SIGINT, &actInt, NULL);

    memset((void*)&actQuit, 0, sizeof(struct sigaction));
    actQuit.sa_flags = SA_INTERRUPT;
    actQuit.sa_handler = &unixSignalHandler;
    sigaction(SIGQUIT, &actQuit, NULL);

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

    /* if this application is use by QtCreator we will look for the settings.json file in the qml project folder. */
    if (args.count() == 2)
    {
        sb = args[1];
        QFileInfo f(sb);
        sb = f.path();
        qDebug() << sb << endl;
    }

    sb.append(QDir::separator());
    sb.append(SETTINGS_FILE);

    // check to see if we have a settings file where we started from
    // if not fall back to system hard coded path
    QFileInfo file(sb.toLatin1());
    if (file.exists())
    {
        qDebug() << "[QMLVIEWER] using local settings file:" << sb;
        settingsFile.setFile(file.filePath());
    }
    else
    {
        //check if there is a settings.json file located at SYSTEM_SETTINGS_FILE.  If not we will create one.
        file.setFile(SYSTEM_SETTINGS_FILE);
        if (file.exists())
        {
            qDebug() << "[QMLVIEWER] using system defined settings file:" << SYSTEM_SETTINGS_FILE;
            settingsFile.setFile(SYSTEM_SETTINGS_FILE);
        }
        else
        {
            if (QFile::copy(":/settings.json", SYSTEM_SETTINGS_FILE))
            {
                 qDebug() << "[QMLVIEWER] created a settings.json file:" << SYSTEM_SETTINGS_FILE;
                 settingsFile.setFile(SYSTEM_SETTINGS_FILE);
            }
            else
                qDebug() << "[QMLVIEWER] error creating a settings.json file:" << SYSTEM_SETTINGS_FILE;
        }
    }

    MainController controller(&view, settingsFile.filePath().toLatin1());

    /* Fix the path if main_view does not contain a path entry.
       This can happen if a user does a copy from a Windows application to the module. */
    if (QSysInfo::buildCpuArchitecture() == "arm")
    {
        if (controller.getMainViewPath().indexOf("/") < 0)
            controller.setMainViewPath(controller.getMainViewPath().prepend("/application/src/"));
        else if (controller.getMainViewPath().at(0) != '/')
        {
            //We need to strip out the path
            QStringList list = controller.getMainViewPath().split("/");
            controller.setMainViewPath( "/application/src/" + list.at(list.length()-1));
        }

    }

    /* handle sigquit and sigint to stop the watchdog if it is running */
    QObject::connect(&app, SIGNAL(aboutToQuit()), &controller, SLOT(handleSigTerm()));

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
