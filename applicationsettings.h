#ifndef APPLICATIONSETTINGS_H
#define APPLICATIONSETTINGS_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

class SerialServerSetting
{
public:
    QString portName() const { return m_portName; }
    QString physicalName() const { return m_physicalPort; }
    QString winPort() const { return m_winPort; }
    QString linuxVMPort() const { return m_linuxVMPort; }
    QString linuxTargetPort() const { return m_linuxTargetPort; }
    int baudRate() const { return m_baudRate; }
    int stopBits() const { return m_stopBits; }
    int dataBits() const { return m_dataBits; }
    QString parity() const { return m_parity; }
    QString flowControl() const { return m_flowControl; }
    bool parseJson() const { return m_parseJson; }
    bool translate() const { return m_translate; }
    QString translateId() const { return m_translateId; }
    bool primaryConnection() const { return m_primaryConnection; }
    QString error() const { return m_error; }

    bool setMembers(QJsonObject jsonObj);

private:
    QString m_portName;
    QString m_physicalPort;
    QString m_winPort;
    QString m_linuxVMPort;
    QString m_linuxTargetPort;
    int m_baudRate;
    int m_stopBits;
    int m_dataBits;
    QString m_parity;
    QString m_flowControl;
    bool m_parseJson;
    bool m_translate;
    QString m_translateId;
    bool m_primaryConnection;
    QString m_error;
};


class StringServerSetting
{
public:
    int port() const { return m_port; }
    bool parseJson() const { return m_parseJson; }
    bool translate() const { return m_translate; }
    QString translateId() const { return m_translateId; }
    bool primaryConnection() const { return m_primaryConnection; }
    QString error() const { return m_error; }

    bool setMembers(QJsonObject jsonObj);

private:
    int m_port;
    bool m_parseJson;
    bool m_translate;
    QString m_translateId;
    bool m_primaryConnection;
    QString m_error;
};


class ApplicationSettings : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationSettings(QObject *parent = 0);

    QString mainView() const;
    bool fullScreen() const;
    bool hideCursor() const;
    bool enableAck() const;
    bool enableHeartbeat() const;
    int heartbeatInterval() const;
    int screenSaverTimeout() const;
    int screenOriginalBrigtness() const;
    int screenDimBrigtness() const;
    bool enableWatchdog() const;
    QString translateFile() const;
    QString languageFile() const;

    QList<SerialServerSetting> serialServers() const;
    QList<StringServerSetting> stringServers() const;

    bool parseJSON(QString settingsFile);

signals:
    void warning(QString);
    void error(QString);

public slots:

private:
    QString m_mainView;
    bool m_fullScreen;
    bool m_hideCursor;
    bool m_enableAck;
    bool m_enableHeartbeat;
    int m_heartbeatInterval;
    int m_screenSaverTimeout;
    int m_screenOriginalBrigtness;
    int m_screenDimBrigtness;
    bool m_enableWatchdog;
    QString m_translateFile;
    QString m_languageFile;
    QList<SerialServerSetting> m_serialServers;
    QList<StringServerSetting> m_stringServers;

    bool setMembers(QJsonObject jsonObj);
    bool validateSettingsFile(QJsonObject jsonObj);
};

#endif // APPLICATIONSETTINGS_H
