#ifndef STRINGSERVER_H
#define STRINGSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

class StringServer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int portName READ getPortName)

public:
    explicit StringServer(QObject *parent = 0, int port = 4000, bool parseJson = false, bool translate = false, QString translateID = "",
                          bool primaryConnection = false);
    ~StringServer();

    int getPortName() {
           return m_port;
    }

signals:
    void MessageAvailable(QByteArray ba, bool parseJson, bool translate, QString translateID);
    void ClientConnected(void);
    void ClientDisconnected(void);
    void PrimaryConnectionAvailable();

public slots:
    bool Send(QString msg);
    int getPort();
    bool getTranslate();
    QString getTranslateID();
    bool Start();

private slots:
    void onClientConnected(void);
    void onClientReadyRead(void);
    void onClientDisconnected(void);

private:
    QTcpServer *m_server;
    QList<QTcpSocket*> m_clients;
    int m_port;
    bool m_parseJson;
    bool m_translate;
    QString m_translateID;
    bool m_primaryConnection;

};

#endif // STRINGSERVER_H
