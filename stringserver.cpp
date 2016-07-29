#include <QDebug>
#include <QTimer>
#include "stringserver.h"

StringServer::StringServer(QObject *parent, int port, bool parseJson, bool translate, QString translateID, bool primaryConnection) : QObject(parent)
  ,m_server(new QTcpServer)
{
    m_port = port;
    m_parseJson = parseJson;
    m_translate = translate;
    m_translateID = translateID;
    m_primaryConnection = primaryConnection;
}


StringServer::~StringServer()
{
    if (m_server)
    {
        m_server->close();
        delete m_server;
    }
}


bool StringServer::Start()
{
    if (this->m_server->listen(QHostAddress::Any, m_port))
    {
        qDebug() << "[QMLVIEWER] TCP Server listening on port" << m_port;
        connect(m_server, SIGNAL(newConnection()), this, SLOT(onClientConnected()));
        if (m_primaryConnection)
            emit PrimaryConnectionAvailable();
        return true;
    }
    else
    {
        qDebug() << "[QMLVIEWER] Error: TCP Server cannot listen on port" << m_port;
        return false;
    }

}


bool StringServer::Send(QString msg)
{
    int count = m_clients.size();

    for(int i = 0; i < count; i++) {
       if(m_clients[i]->state() == QAbstractSocket::ConnectedState) {
           m_clients[i]->write(msg.append("\r\n").toLatin1());
       }
    }

    return true;
}


int StringServer::getPort()
{
    return m_port;
}

bool StringServer::getTranslate()
{
    return m_translate;
}

QString StringServer::getTranslateID()
{
    return m_translateID;
}


void StringServer::onClientConnected()
{
    qDebug() << "[QMLVIEWER] Handling new connection.";

    QTcpSocket *s = m_server->nextPendingConnection();
    connect(s, SIGNAL(readyRead()), this,SLOT(onClientReadyRead()));
    connect(s, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));

    m_clients.append(s);
    emit ClientConnected();
}


void StringServer::onClientReadyRead()
{
    int count = m_clients.size();

    for(int i = 0; i < count; i++) {
        while (m_clients[i]->bytesAvailable() && m_clients[i]->canReadLine()) {
            QByteArray ba = m_clients[i]->readLine();
            emit MessageAvailable(ba, m_parseJson, m_translate, m_translateID);
        }
    }
}


void StringServer::onClientDisconnected()
{
    int count = m_clients.size();

    for(int i = 0; i < count; i++) {
       if(m_clients[i]->state() == QAbstractSocket::UnconnectedState) {
           qDebug() << "[QMLVIEWER] Removing client:" << i;
           disconnect(m_clients[i], SIGNAL(readyRead()), this,SLOT(onClientReadyRead()));
           disconnect(m_clients[i], SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
           m_clients[i]->deleteLater();
           m_clients.removeAt(i);
           emit ClientDisconnected();
           break;
       }
    }
}

