#include <QQuickItem>
#include <QQmlContext>
#include "maincontroller.h"

MainController::MainController(MainView *view, QString settingsFilePath,
                               QObject *parent) :
  QObject(parent)
  ,m_view(view)
  ,m_settings(new Settings(this))
  ,m_heartbeatText(HEARTBEAT_TEXT)
  ,m_heartbeatResponseText(HEARTBEAT_RESPONSE_TEXT)
  ,m_heartbeat_interval(0)
  ,m_hearbeatTimer(new QTimer(this))
  ,m_errorTimer(new QTimer(this))
  ,m_appSettings(new ApplicationSettings(this))
{
    connect(m_appSettings, SIGNAL(error(QString)),this, SLOT(onAppSettingsError(QString)));

    /* load setting from the json file */
    if (m_appSettings->parseJSON(settingsFilePath))
    {
        m_screen = new Screen(view, m_appSettings->screenSaverTimeout(), m_appSettings->screenOriginalBrigtness(),
                              m_appSettings->screenDimBrigtness(), this);
        m_watchdog = new Watchdog(this, m_appSettings->enableWatchdog());
        m_beep = new Beep(this);

        /* Define objects that can be used in qml */
        m_view->rootContext()->setContextProperty("connection",this);
        m_view->rootContext()->setContextProperty("settings", m_settings);
        m_view->rootContext()->setContextProperty("screen", m_screen);
        m_view->rootContext()->setContextProperty("watchdog", m_watchdog);
        m_view->rootContext()->setContextProperty("beeper", m_beep);

        /* Enable or disable ack */
        if (m_appSettings->enableAck())
            enableLookupAck();
        else
            disableLookupAck();

        /* Enablle or disable heartbeat */
        connect(m_hearbeatTimer,SIGNAL(timeout()),this, SLOT(onHeartbeatTimerTimeout()));
        if (m_appSettings->enableHeartbeat())
            enableHeartbeat(m_appSettings->heartbeatInterval());
        else
            disableHeartbeat();

        /* Add a connection to the view statusChanged signal */
        connect(m_view, SIGNAL(statusChanged(QQuickView::Status)), this, SLOT(onViewStatusChanged(QQuickView::Status)));

        m_clients = 0;
        m_enableTranslator = false;
        m_startUpError = "";

        /* Create the TCP string servers and add connections */
        int i = 0;
        foreach(const StringServerSetting &server, m_appSettings->stringServers())
        {
            StringServer *stringServer =  new StringServer(this, server.port(), server.parseJson(),
                                                           server.translate(), server.translateId(),
                                                           server.primaryConnection());
            if (server.translate())
                m_enableTranslator = true;
            connect(stringServer, SIGNAL(PrimaryConnectionAvailable()), this, SLOT(onPrimaryConnectionAvailable()));
            connect(stringServer, SIGNAL(MessageAvailable(QByteArray, bool, bool, QString))
                    , this, SLOT(onMessageAvailable(QByteArray, bool, bool, QString)));
            connect(stringServer, SIGNAL(ClientConnected()), this, SLOT(onClientConnected()));
            connect(stringServer, SIGNAL(ClientDisconnected()), this, SLOT(onClientDisconnected()));

            if (stringServer->Start())
            {
                m_stringServerList.append(stringServer);
                i += 1;
            }
            else
            {
                delete stringServer;
            }
        }


        /* Create the Serial Servers and add the connections */
        i = 0;
        foreach(const SerialServerSetting &server, m_appSettings->serialServers())
        {
            SerialServer *serialServer = new SerialServer(server, this);
            connect(serialServer, SIGNAL(ClientConnected()), this, SLOT(onClientConnected()));
            connect(serialServer, SIGNAL(PrimaryConnectionAvailable()), this, SLOT(onPrimaryConnectionAvailable()));
            connect(serialServer, SIGNAL(MessageAvailable(QByteArray, bool, bool, QString))
                    , this, SLOT(onMessageAvailable(QByteArray, bool, bool, QString)));
            connect(serialServer, SIGNAL(Error(QString)), this, SLOT(showError(QString)));

            if (server.translate())
                m_enableTranslator = true;

            if (serialServer->Start())
            {
                m_serialServerList.append(serialServer);
                i += 1;
            }
            else
            {
                delete serialServer;
            }

        }

        if (m_enableTranslator){
            /* Initialize the Translator object */
            m_transLator = new Translator(m_appSettings->translateFile(), this);
            /* Load and parse the translate file. */
            m_transLator->loadTranslations();
        }

        /* check if we need to load a language translate file */
        if (m_appSettings->languageFile().length() > 0)
        {
            loadLanguageTranslator(m_appSettings->languageFile());
        }

        setMainViewPath(m_appSettings->mainView());
        m_view->show();
    }
}


MainController::~MainController()
{
    if (m_screen)
        delete m_screen;

    if (m_settings)
        delete m_settings;

    if(!m_stringServerList.isEmpty())
        qDeleteAll(m_stringServerList);

    if (!m_serialServerList.isEmpty())
        qDeleteAll(m_serialServerList);

    if (m_enableTranslator && m_transLator)
        delete m_transLator;

    if (m_hearbeatTimer)
        delete m_hearbeatTimer;

    if (m_watchdog)
        delete m_watchdog;

    if (m_errorTimer)
        delete m_errorTimer;

    if (m_appSettings)
        delete m_appSettings;

    if (m_beep)
        delete(m_beep);
}


bool MainController::sendTCPMessage(QString msg, int port)
{
    QString translatedMessage = msg;
    int numServer = -1;

    /* find the server */
    for (int i=0; i < m_stringServerList.length(); i++)
    {
        if (m_stringServerList[i]->getPort() == port)
        {
            numServer = i;
            break;
        }
    }

    if (numServer == -1)
    {
        /* Show message that the server with port was not found */
        qDebug() << "[QMLVIEWER] Error Could not sendTCPMessage.  TCP Server on port " << port << " was not found.";
        return false;
    }


    /* Translate the message if we need to. */
    if (m_enableTranslator && m_stringServerList.at(numServer)->getTranslate())
    {
        translatedMessage = m_transLator->translateGuiMessage(msg);
        if (translatedMessage.length() == 0)
        {
            qDebug() << "[QMLVIEWER] Unable to translate message:" << msg;
            return false;
        }
    }


    m_stringServerList.at(numServer)->Send(translatedMessage);
    return true;
}


bool MainController::sendSerialMessage(QString msg, QString portName)
{
    QString translatedMessage = msg;
    int numServer = -1;

    /* Find the server */
    for (int i=0; i < m_serialServerList.length(); i++)
    {
        if (m_serialServerList[i]->getPortName() == portName)
        {
            numServer = i;
            break;
        }
    }

    if (numServer == -1)
    {
        /* Show message that the server with port was not found */
        qDebug() << "[QMLVIEWER] Error Could not sendSerialMessage.  Serial Server on port " << portName << "was not found.";
        return false;
    }

    /* Translate the message if we need to. */
    if (m_enableTranslator && m_serialServerList.at(numServer)->getTranslate())
    {
        translatedMessage = m_transLator->translateGuiMessage(msg);
        if (translatedMessage.length() == 0)
        {
            qDebug() << "[QMLVIEWER] Unable to translate message:" << msg;
            return false;
        }
    }

    m_serialServerList.at(numServer)->Send(translatedMessage);
    return true;
}


bool MainController::sendMessage(QString msg)
{
    QString translatedMessage = msg;

    if (m_primaryConnectionClassName == "StringServer")
    {
        /* Translate the message if we need to. */
        if (m_enableTranslator && qobject_cast<StringServer*>(m_primaryConnection)->getTranslate())
        {
            translatedMessage = m_transLator->translateGuiMessage(msg);
            if (translatedMessage.length() == 0)
            {
                qDebug() << "[QMLVIEWER] Unable to translate message:" << msg;
                return false;
            }
        }

        qobject_cast<StringServer*>(m_primaryConnection)->Send(translatedMessage);
        return true;
    }
    else if (m_primaryConnectionClassName == "SerialServer")
    {
        /* Translate the message if we need to. */
        if (m_enableTranslator && qobject_cast<SerialServer*>(m_primaryConnection)->getTranslate())
        {
            translatedMessage = m_transLator->translateGuiMessage(msg);
            if (translatedMessage.length() == 0)
            {
                qDebug() << "[QMLVIEWER] Unable to translate message:" << msg;
                return false;
            }
        }

        qobject_cast<SerialServer*>(m_primaryConnection)->Send(translatedMessage);
        return true;
    }

    return false;
}


void MainController::onMessageAvailable(QByteArray ba, bool parseJson, bool translate, QString translateID)
{
    QString message(ba);
    message.replace('\r', "");
    message.replace('\n', "");
    if (message.length() == 0) {
        return;
    }

    /* check for hearbeat - must be a pong */
    if(message.trimmed() == m_heartbeatResponseText) {
        qDebug() << "[QMLVIEWER] got " << m_heartbeatResponseText;
        if(m_hearbeatTimer->isActive()) {
            m_enableHearbeat = true;
            emit heartbeat();
            return;
        }
    }

    /* Translate the the message if we have to. */
    if (translate)
    {
        message = m_transLator->translateMCUMessage(translateID, message);
    }

    /* Our protocol is obj.prop=value, so split message. */
    if (message.contains("=") && message.contains("."))
    {

        QString value;
        int pos = message.indexOf("=");
        value = message.mid(pos + 1, message.length());
        QString item = message.mid(0, pos);
        QStringList items = item.split('.');

        if (value.length() == 0 || items.count() != 2) {
            qDebug() << "[QMLVIEWER] Message syntax error." << message;
            if (m_enableAck)
                sendMessage("SYNERR");
        }
        else
        {
            qDebug() << "[MCU " << translateID << "]: " << items[0] << "." << items[1] << ": " << value;
            if (parseJson)
                setJsonProperty(items[0], items[1], value);
            else
                setProperty(items[0], items[1], value);
        }
    }
    else {
        qDebug() << "[QMLVIEWER] Invalid message:" << message << " from " << ba;
        if (m_enableAck)
            sendMessage("SYNERR");
    }

}


void MainController::onPrimaryConnectionAvailable()
{
    m_primaryConnection = QObject::sender();
    m_primaryConnectionClassName = QObject::sender()->metaObject()->className();
    qDebug() << "[QMLVIEWER] Primary connection set to" << QObject::sender()->metaObject()->className() << ":" << QObject::sender()->property("portName").toString();
}


void MainController::enableHeartbeat(int interval)
{
    qDebug() << "[QMLVIEWER] hearbeat enabled ";
    m_heartbeat_interval = interval;
    m_hearbeatTimer->stop();
    m_hearbeatTimer->start((m_heartbeat_interval * 1000));
}


void MainController::enableHeartbeat(int interval, QString heartbeatText, QString heartbeatResponseText)
{
    qDebug() << "[QMLVIEWER] hearbeat enabled ";
    m_heartbeatText = heartbeatText;
    m_heartbeatResponseText = heartbeatResponseText;
    m_heartbeat_interval = interval;
    m_hearbeatTimer->stop();
    m_hearbeatTimer->start((m_heartbeat_interval * 1000));
}


void MainController::disableHeartbeat()
{
    qDebug() << "[QMLVIEWER] hearbeat disabled ";
    if(m_hearbeatTimer->isActive()) {
        m_hearbeatTimer->stop();
    }
}


void MainController::onHeartbeatTimerTimeout()
{
    /* if we have not received a pong emit signal*/
    if(!m_enableHearbeat) {
        emit noHeartbeat();
    }
    m_enableHearbeat = false;
    sendMessage(m_heartbeatText);
}


void MainController::disableLookupAck()
{
    m_enableAck = false;
    emit lookupAckChanged(false);
}


QString MainController::getStartUpError()
{
    return m_startUpError;
}


void MainController::handleSigTerm()
{
    // shut down the watchdog timer if it was started
    if (m_watchdog->isStarted())
        m_watchdog->stop();
}


void MainController::setMainViewPath(QString path)
{
    m_mainViewPath = path;
}


QString MainController::getMainViewPath()
{
    return m_mainViewPath;
}


bool MainController::showFullScreen()
{
    return m_appSettings->fullScreen();
}


bool MainController::hideCursor()
{
    return m_appSettings->hideCursor();
}


void MainController::enableLookupAck()
{
    m_enableAck = true;
    emit lookupAckChanged(true);
}


void MainController::onClientConnected()
{
    m_mutex.lock();
    m_clients++;
    m_mutex.unlock();
    emit readyToSend();
    qDebug() << "[QMLVIEWER] Clients connected:" << m_clients;
}


void MainController::onClientDisconnected()
{
    m_mutex.lock();
    m_clients--;
    m_mutex.unlock();

    if (m_clients == 0)
        emit notReadyToSend();

    qDebug() << "[QMLVIEWER] Clients connected:" << m_clients;
}


void MainController::setJsonProperty(QString object, QString property, QString value)
{
    QQuickItem *obj =  m_view->rootObject()->findChild<QQuickItem*>(object);
    if (!obj) {
        qDebug() << "[QMLVIEWER] no item with objectName:" << object;
        if (m_enableAck)
            sendMessage("LUNO");
        return;
    }

    //Now lets parse the json
    QJsonDocument doc(QJsonDocument::fromJson(value.toUtf8()));
    QVariant jsonVariant;

    if(!doc.isNull())
    {
        if(doc.isObject() || doc.isArray())
        {
            jsonVariant = doc.toVariant();
        }
        else
        {
            qDebug() << "[QMLVIEWER] Document is not an object" << endl;
            return;
        }
    }
    else
    {
        qDebug() << "[QMLVIEWER] Invalid JSON...\n" << value << endl;
        return;
    }

    bool found = obj->setProperty(property.toLatin1(), jsonVariant);

    if (!found) {
        if (m_enableAck)
            sendMessage("LUNP");
        qDebug() << "[QMLVIEWER] no property on objectName:" << property;
    }

    if (m_enableAck)
        sendMessage("LUOK");

}


void MainController::setProperty(QString object, QString property, QString value)
{
    QQuickItem *obj =  m_view->rootObject()->findChild<QQuickItem*>(object);
    if (!obj) {
        qDebug() << "[QMLVIEWER] no item with objectName:" << object;
        if (m_enableAck)
            sendMessage("LUNO");
        return;
    }

    bool found = obj->setProperty(property.toLatin1(), value);

    if (!found) {
        if (m_enableAck)
            sendMessage("LUNP");
        qDebug() << "[QMLVIEWER] no property on objectName:" << property;
    }

    if (m_enableAck)
        sendMessage("LUOK");

}


void MainController::onViewStatusChanged(QQuickView::Status status)
{
    /* Emit the signal right away if the serialserver list is greater than 0.                       */
    /* If the serial server list is zero, we wait for TCP/IP connections before we emit the signal. */
    if (status == 1 && m_serialServerList.count() > 0)
        emit readyToSend();
    else if (status == 0)
        emit notReadyToSend();
}


void MainController::showError(QString errorMessage)
{
    //Load error.qml file and show the user an error message
    m_startUpError = errorMessage;
    m_view->setSource(QUrl(QStringLiteral("qrc:/error.qml")));
#ifdef Q_OS_WIN
    connect(m_errorTimer, SIGNAL(timeout()), this, SLOT(onErrorTimerTimeOut()));
    m_errorTimer->start(5000);
#endif
}

void MainController::onErrorTimerTimeOut()
{
    m_errorTimer->stop();
    m_view->setSource(QUrl::fromLocalFile(m_mainViewPath));
#ifdef Q_OS_WIN
    emit readyToSend();
#endif
}


void MainController::onAppSettingsError(QString msg)
{
    /* error.qml uses the screen object */
    m_screen = new Screen(m_view, 0, 7, 5, this);

    /* Define object that can be used in qml */
    m_view->rootContext()->setContextProperty("connection",this);
    m_view->rootContext()->setContextProperty("screen", m_screen);

    qDebug() << qPrintable(msg.trimmed());
    m_startUpError = msg.trimmed();
    m_view->setSource(QUrl(QStringLiteral("qrc:/error.qml")));
    m_view->show();
}


void MainController::loadLanguageTranslator(QString languageFile)
{
    QTranslator languageTranslator;
    if (languageTranslator.load(languageFile)) {
        qDebug() << "[QML] translation file loaded" << languageFile;
        QGuiApplication::installTranslator(&languageTranslator);
        // clear the component cache to allow for translations
        m_view->engine()->clearComponentCache();
    }
    else
        qDebug() << "[QML] translation file load failed for" << languageFile;

}
