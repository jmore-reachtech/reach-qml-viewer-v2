#ifndef SERIALSERVER_H
#define SERIALSERVER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QIODevice>
#include <QJsonObject>
#include <QMetaEnum>
#include <QDebug>

class SerialServer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString portName READ getPortName)

public:
    explicit SerialServer(const QJsonValue portInfo, QObject *parent = 0);
    ~SerialServer();

    QString getPortName() const {
           return m_portName;
    }
signals:
    void MessageAvailable(QByteArray ba, bool parseJson, bool translate, QString translateID);
    void PrimaryConnectionAvailable();
    void ClientConnected(void);
    void Error(QString error);

public slots:
    int Send(QString msg);
    bool getParseJon();
    bool getTranslate();
    QString getTranslateID();
    QString getPortName();
    bool Start();

private slots:
    void onClientReadyRead(void);
    void onClientError(QSerialPort::SerialPortError error);
    QSerialPort::StopBits getStopBitsEnum(int stopBits);
    QSerialPort::Parity getParityEnum(QString parity);
    QSerialPort::FlowControl getFlowControlEnum(QString flowControl);
    QSerialPort::DataBits getDataBitsEnum(int dataBits);

private:
    QSerialPort *m_server;
    QJsonObject m_jsonObj;
    bool m_parseJson;
    bool m_translate;
    QString m_translateID;
    bool m_primaryConnection;
    QString m_portName;

};

#endif // SERIALSERVER_H
