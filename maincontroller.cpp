#include <QQuickItem>
#include <QQmlContext>
#include "maincontroller.h"

MainController::MainController(MainView *view, QJsonArray tcpServers, QJsonArray serialServers,
                               QString translateFile, bool enableAck, bool heartbeat, int heartbeatInterval,
                               int screenSaverTimeout, int screenOriginalBrightness, int screenDimBrightness,
                               QObject *parent) :
  QObject(parent)
  ,m_view(view)
  ,m_settings(new Settings(this))
  ,m_screen(new Screen(view, screenSaverTimeout, screenOriginalBrightness, screenDimBrightness, this))
  ,m_heartbeatText(HEARTBEAT_TEXT)
  ,m_heartbeatResponseText(HEARTBEAT_RESPONSE_TEXT)
  ,m_heartbeat_interval(0)
  ,m_hearbeatTimer(new QTimer(this))
{
   /* Define objects that can be used in qml */
    m_view->rootContext()->setContextProperty("connection",this);
    m_view->rootContext()->setContextProperty("settings", m_settings);
    m_view->rootContext()->setContextProperty("screen", m_screen);

    /* Enable or disable ack */
    if (enableAck)
        enableLookupAck();
    else
        disableLookupAck();

    /* Enablle or disable heartbeat */
    connect(m_hearbeatTimer,SIGNAL(timeout()),this,SLOT(onHeartbeatTimerTimeout()));
    if (heartbeat)
        enableHeartbeat(heartbeatInterval);
    else
        disableHeartbeat();

    /* Add a connection to the view statusChanged signal */
    connect(m_view, SIGNAL(statusChanged(QQuickView::Status)), this, SLOT(onViewStatusChanged(QQuickView::Status)));

    m_clients = 0;

    m_enableTranslator = false;

    /* Create the TCP string servers and add connections */
    int i = 0;
    foreach(const QJsonValue &v, tcpServers)
    {
        if (v.toObject().value("enabled").toBool())
        {
            StringServer *stringServer =  new StringServer(this, v.toObject().value("port").toInt(), v.toObject().value("parse_json").toBool(),
                                                           v.toObject().value("translate").toBool(), v.toObject().value("translate_id").toString(),
                                                           v.toObject().value("primary_connection").toBool());
            if (v.toObject().value("translate").toBool() == true)
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
    }


    /* Create the Serial Servers and add the connections */
    i = 0;
    foreach(const QJsonValue &v, serialServers)
    {
        if (v.toObject().value("enabled").toBool())
        {
            SerialServer *serialServer = new SerialServer(v, this);
            connect(serialServer, SIGNAL(ClientConnected()), this, SLOT(onClientConnected()));
            connect(serialServer, SIGNAL(PrimaryConnectionAvailable()), this, SLOT(onPrimaryConnectionAvailable()));
            connect(serialServer, SIGNAL(MessageAvailable(QByteArray, bool, bool, QString))
                    , this, SLOT(onMessageAvailable(QByteArray, bool, bool, QString)));

            if (v.toObject().value("translate").toBool() == true)
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

    }

    if (m_enableTranslator){
        /* Initialize the Translator object */
        m_transLator = new Translator(translateFile, this);
        /* Load and parse the translate file. */
        m_transLator->loadTranslations();
    }

    m_view->show();
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

    if (m_transLator)
        delete m_transLator;

    if (m_hearbeatTimer)
        delete m_hearbeatTimer;
}


bool MainController::sendTCPMessage(QString msg, int port)
{
    QString translatedMessage = msg;

    /* Translate the message if we need to. */
    if (m_enableTranslator)
    {
        translatedMessage = m_transLator->translateGuiMessage(msg);
        if (translatedMessage.length() == 0)
        {
            qDebug() << "[QMLVIEWER] Unable to translate message:" << msg;
            return false;
        }
    }

    for (int i=0; i < m_stringServerList.length(); i++)
    {
        if (m_stringServerList[i]->getPort() == port)
        {
            m_stringServerList.at(i)->Send(translatedMessage);
            qDebug() << "[QMLVIEWER SENT]: " << translatedMessage;
            return true;
        }
    }

    return false;
}


bool MainController::sendSerialMessage(QString msg, QString portName)
{
    QString translatedMessage = msg;

    /* Translate the message if we need to. */
    if (m_enableTranslator)
    {
        translatedMessage = m_transLator->translateGuiMessage(msg);
        if (translatedMessage.length() == 0)
        {
            qDebug() << "[QMLVIEWER] Unable to translate message:" << msg;
            return false;
        }
    }

    for (int i=0; i < m_serialServerList.length(); i++)
    {
        if (m_serialServerList[i]->getPortName() == portName)
        {
            m_serialServerList.at(i)->Send(translatedMessage);
            qDebug() << "[QMLVIEWER SENT]: " << translatedMessage;
            return true;
        }
    }

    return false;
}


bool MainController::sendMessage(QString msg)
{
    QString translatedMessage = msg;

    /* Translate the message if we need to. */
    if (m_enableTranslator)
    {
        translatedMessage = m_transLator->translateGuiMessage(msg);
        if (translatedMessage.length() == 0)
        {
            qDebug() << "[QMLVIEWER] Unable to translate message:" << msg;
            return false;
        }
    }

    if (m_primaryConnectionClassName == "StringServer")
    {
       qobject_cast<StringServer*>(m_primaryConnection)->Send(translatedMessage);
    }
    else if (m_primaryConnectionClassName == "SerialServer")
    {
        qobject_cast<SerialServer*>(m_primaryConnection)->Send(translatedMessage);
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
