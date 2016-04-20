#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <QObject>
#include <QMutex>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include "mainview.h"
#include "stringserver.h"
#include "serialserver.h"
#include "translator.h"
#include "systemdefs.h"
#include "settings.h"
#include "screen.h"

class MainController : public QObject
{
    Q_OBJECT

public:
    explicit MainController(MainView *view, QJsonArray tcpServers , QJsonArray serialServers, QString translateFile,
                            bool enableAck, bool enableHeartBeat, int heartBeatInterval,
                            int screenSaverTimeout, int screenOriginalBrightness, int screenDimBrightness,
                            QObject *parent = 0);
    ~MainController();

signals:
    void readyToSend();
    void notReadyToSend();
    void noHeartbeat();
    void heartbeat();
    void lookupAckChanged(bool);

public slots:
    Q_INVOKABLE bool sendTCPMessage(QString msg, int port);
    Q_INVOKABLE bool sendSerialMessage(QString msg, QString portName);
    Q_INVOKABLE bool sendMessage(QString msg);
    Q_INVOKABLE void enableHeartbeat(int);
    Q_INVOKABLE void enableHeartbeat(int, QString, QString);
    Q_INVOKABLE void disableHeartbeat();
    Q_INVOKABLE void enableLookupAck();
    Q_INVOKABLE void disableLookupAck();


private slots:
    void onMessageAvailable(QByteArray ba, bool parseJson, bool translate, QString translateID);
    void onPrimaryConnectionAvailable();
    void onClientConnected(void);
    void onClientDisconnected(void);
    void onHeartbeatTimerTimeout();
    void setJsonProperty(QString object, QString property, QString value);
    void setProperty(QString object, QString property, QString value);

private:
    MainView *m_view;
    Settings *m_settings;
    Screen *m_screen;
    Translator *m_transLator;
    QList<StringServer*> m_stringServerList;\
    QList<SerialServer*> m_serialServerList;
    qint32 m_clients;
    QMutex m_mutex;
    bool m_parseJSON;
    QObject *m_primaryConnection;
    QString m_primaryConnectionClassName;
    bool    m_enableAck;
    bool    m_enableHearbeat;
    QString m_heartbeatText;
    QString m_heartbeatResponseText;
    int     m_heartbeat_interval;
    QTimer  *m_hearbeatTimer;
};

#endif // MAINCONTROLLER_H
