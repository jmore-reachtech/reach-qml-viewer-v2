#include "applicationsettings.h"

ApplicationSettings::ApplicationSettings(QObject *parent) : QObject(parent)
{
}


QString ApplicationSettings::mainView() const
{
    return m_mainView;
}


bool ApplicationSettings::fullScreen() const
{
    return m_fullScreen;
}


bool ApplicationSettings::hideCursor() const
{
    return m_hideCursor;
}


bool ApplicationSettings::enableAck() const
{
    return m_enableAck;
}


QList<SerialServerSetting> ApplicationSettings::serialServers() const
{
    return m_serialServers;
}


QList<StringServerSetting> ApplicationSettings::stringServers() const
{
    return m_stringServers;
}


int ApplicationSettings::screenSaverTimeout() const
{
    return m_screenSaverTimeout;
}


int ApplicationSettings::screenOriginalBrigtness() const
{
    return m_screenOriginalBrigtness;
}


int ApplicationSettings::screenDimBrigtness() const
{
    return m_screenDimBrigtness;
}


bool ApplicationSettings::enableWatchdog() const
{
    return m_enableWatchdog;
}


QString ApplicationSettings::translateFile() const
{
    return m_translateFile;
}


int ApplicationSettings::heartbeatInterval() const
{
    return m_heartbeatInterval;
}


bool ApplicationSettings::enableHeartbeat() const
{
    return m_enableHeartbeat;
}


bool ApplicationSettings::validateSettingsFile(QJsonObject jsonObj)
{
    /* We do some basic validation here.
     * Check to see if we have at least one TCP/Server or Serial Port Server enabled.
     * Check to see if we have translate path if translate is true in any enabled servers
     * Check to see that all enabled servers have unique translate_id's if translate is true.
     * Make sure there is only one primary connection if one or more is set.
     */

    int primaryConnectionCount = 0;
    int enabledServerCount = 0;
    int translateCount = 0;
    QString errorMessage = "";
    QStringList translateIdList;
    QStringList tcpServerPortList;
    QStringList serialServerPortList;

    QJsonArray tcpServers = jsonObj.value("tcp_servers").toArray();
    foreach (const QJsonValue &v, tcpServers)
    {
        if (v.toObject().value("enabled").toBool())
            enabledServerCount += 1;

        if (v.toObject().value("translate").toBool() == true && v.toObject().value("enabled").toBool())
        {
            QString translateId = v.toObject().value("translate_id").toString();
            translateCount += 1;
            if (translateId.length() == 0)
                errorMessage.append("Missing translate_id for tcp_servers on port: ").append(v.toObject().value("port").toString()).append("\n");

            if (!translateIdList.contains(translateId) && translateId.length() > 0)
                translateIdList << translateId;
            else
                errorMessage.append("JSON field translate_id was duplicated: ").append(translateId).append("\n");
        }

        QString port = QString::number(v.toObject().value("port").toInt());
        //Check if this is a duplicate port number
        if (!tcpServerPortList.contains(port))
            tcpServerPortList << port;
        else
            errorMessage.append("JSON field tcp_servers port was duplicated: ").append(port).append("\n");


        if (v.toObject().value("primary_connection").toBool() == true && v.toObject().value("enabled").toBool())
            primaryConnectionCount += 1;
    }

    QJsonArray serialServers = jsonObj.value("serial_port_servers").toArray();
    foreach (const QJsonValue &v, serialServers)
    {
        if (v.toObject().value("enabled").toBool())
            enabledServerCount += 1;

        if (v.toObject().value("translate").toBool() == true && v.toObject().value("enabled").toBool())
        {
            QString translateId = v.toObject().value("translate_id").toString();
            translateCount += 1;
            if (translateId.length() == 0  && QSysInfo::buildCpuArchitecture() == "arm")
                errorMessage.append("Missing translate_id for serial_port_servers on port: ").append(v.toObject().value("linux_target_port").toString()).append("\n");
            else if (translateId.length() == 0)
                errorMessage.append("Missing translate_id for serial_port_servers on port: ").append(v.toObject().value("linux_vm_port").toString()).append("\n");

            if (!translateIdList.contains(translateId) && translateId.length() > 0)
                translateIdList << translateId;
            else
                errorMessage.append("JSON field translate_id was duplicated: ").append(translateId).append("\n");            
        }

        QString port;
        if (QSysInfo::buildCpuArchitecture() == "arm")
            port = v.toObject().value("linux_target_port").toString();
        else
            port = v.toObject().value("linux_vm_port").toString();

        if (!serialServerPortList.contains(port))
            serialServerPortList << port;
        else
            errorMessage.append("JSON field serial_port_servers port was duplicated: ").append(port).append("\n");

        if (v.toObject().value("primary_connection").toBool() == true && v.toObject().value("enabled").toBool())
            primaryConnectionCount += 1;
    }

    if (enabledServerCount == 0)
        errorMessage.append("You must enable at least one tcp_servers or serial_port_servers.\n");

    if (primaryConnectionCount > 1)
        qDebug() << "[SETTINGS WARNING] More than 1 primary_connection field was set to true.";

    if (translateCount > 0 && jsonObj.value("translate_file").toString().length() == 0)
    {
        if (QSysInfo::buildCpuArchitecture() == "arm")
            qDebug() << "[SETTINGS WARNING] The JSON field translate_file is empty.";
        else
            errorMessage.append("The JSON field translate_file is empty.\n");
    }

    if (errorMessage.length() > 0)
    {
        errorMessage.prepend("[SETTINGS ERROR] settings.json file errors found:\n");
        emit error(errorMessage);
        return false;
    }

    return true;
}


bool ApplicationSettings::setMembers(QJsonObject jsonObj)
{
    /* validate the settings.json file */
    if (validateSettingsFile(jsonObj))
    {
        try
        {
            /* set the members */
            m_enableAck = jsonObj.contains("enable_ack") ? jsonObj.value("enable_ack").toBool() : false;
            m_enableHeartbeat = jsonObj.contains("enable_heartbeat") ? jsonObj.value("enable_heartbeat").toBool() : false;
            m_enableWatchdog = jsonObj.contains("enable_watchdog") ? jsonObj.value("enable_watchdog").toBool() : false;
            m_fullScreen = jsonObj.contains("full_screen") ? jsonObj.value("full_screen").toBool() : true;
            m_heartbeatInterval = jsonObj.contains("heartbeat_interval") ? jsonObj.value("heartbeat_interval").toInt() : 0;
            m_hideCursor = jsonObj.contains("hide_cursor") ? jsonObj.value("hide_cursor").toBool() : false;
            m_mainView = jsonObj.contains("main_view") ? jsonObj.value("main_view").toString() : "";
            m_screenSaverTimeout =  jsonObj.contains("screensaver_timeout") ? jsonObj.value("screensaver_timeout").toInt() : 0;
            m_screenOriginalBrigtness = jsonObj.contains("screen_original_brigtness") ? jsonObj.value("screen_original_brigtness").toInt() : 7;
            m_screenDimBrigtness = jsonObj.contains("screen_dim_brigtness") ? jsonObj.value("screen_dim_brigtness").toInt() : 5;
            m_translateFile = jsonObj.contains("translate_file") ? jsonObj.value("translate_file").toString() : "";

            /* set serial port servers */
            foreach(const QJsonValue &v, jsonObj.value("serial_port_servers").toArray())
            {
                if (v.toObject().contains("enabled") && v.toObject().value("enabled").toBool())
                {
                    SerialServerSetting serialServer;
                    if (serialServer.setMembers(v.toObject()))
                        m_serialServers << serialServer;
                    else
                    {
                        qDebug() << "[SETTINGS ERROR] json not valid for serial_port_servers:" << v.toObject();
                        emit error(serialServer.error());
                        return false;
                    }
                }
            }

            /* set tcp servers */
            foreach(const QJsonValue &v, jsonObj.value("tcp_servers").toArray())
            {
                if (v.toObject().contains("enabled") && v.toObject().value("enabled").toBool())
                {
                    StringServerSetting stringServer;

                    if (stringServer.setMembers(v.toObject()))
                        m_stringServers << stringServer;
                    else
                    {
                        qDebug() << "[SETTINGS ERROR] json not valid for tcp_servers:" << v.toObject();
                        emit error(stringServer.error());
                        return false;
                    }
                }
            }

            return true;
        }
        catch (std::exception & e)
        {
            qDebug() << "[SETTINGS ERROR] " << e.what();
            emit error( QString("[SETTINGS ERROR] ").append(e.what()));
            return false;
        }
    }
    else
        return false;
}


bool ApplicationSettings::parseJSON(QString settingsFile)
{
    QFile jsonFile;
    QString json;
    jsonFile.setFileName(settingsFile);
    if (jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
          json = jsonFile.readAll();
          jsonFile.close();
    }
    else {
        emit error("[SETTINGS ERROR] Could not open Settings file: " + settingsFile);
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());;
    QJsonObject jsonObj;

    if (!doc.isNull())
    {
        if (doc.isObject())
            jsonObj = doc.object();
        else {
            emit error("[SETTINGS ERROR] Document is not an object.");
            return false;
        }
    }
    else {
        emit error("[SETTINGS ERROR] Settings file contains invalid JSON.");
        return false;
    }

    if (setMembers(jsonObj))
        return true;
    else
        return false;
}


bool SerialServerSetting::setMembers(QJsonObject jsonObj)
{
    m_error = "";
    if (jsonObj.contains("port_name"))
        m_portName = jsonObj.value("port_name").toString();
    else
    {
        m_error = "[SETTINGS ERROR] Missing a serial_port_servers port_name field.";
        return false;
    }

    m_physicalPort = jsonObj.contains("physical_port") ? jsonObj.value("physical_port").toString() : "";
    m_winPort = jsonObj.contains("win_port") ? jsonObj.value("win_port").toString() : "";

    if (QSysInfo::buildCpuArchitecture() == "arm")
    {
        if (jsonObj.contains("linux_target_port"))
            m_linuxTargetPort = jsonObj.value("linux_target_port").toString();
        else
        {
            m_error = "[SETTINGS ERROR] Missing a serial_port_servers linux_target_port field.";
            return false;
        }
    }
    else
    {
        if (jsonObj.contains("linux_vm_port"))
            m_linuxVMPort = jsonObj.value("linux_vm_port").toString();
        else
        {
            m_error = "[SETTINGS ERROR] Missing a serial_port_servers linux_vm_port field.";
            return false;
        }
    }

    m_baudRate = jsonObj.contains("baud_rate") ? jsonObj.value("baud_rate").toInt() : 115200;
    m_stopBits = jsonObj.contains("stop_bits") ? jsonObj.value("stop_bits").toInt() : 1;
    m_dataBits = jsonObj.contains("data_bits") ? jsonObj.value("data_bits").toInt() : 8;
    m_parity = jsonObj.contains("parity") ? jsonObj.value("parity").toString() : "none";
    m_flowControl= jsonObj.contains("flow_control") ? jsonObj.value("flow_control").toString() : "off";
    m_parseJson = jsonObj.contains("parse_json") ? jsonObj.value("parse_json").toBool() : false;
    m_translate = jsonObj.contains("translate") ? jsonObj.value("translate").toBool() : true;

    if (m_translate && jsonObj.contains("translate_id"))
        m_translateId = jsonObj.value("translate_id").toString();
    else if (m_translate)
    {
        m_error = "[SETTINGS ERROR] Missing a serial_port_servers translate_id field.";
        return false;
    }

    m_primaryConnection = jsonObj.contains("primary_connection") ? jsonObj.value("primary_connection").toBool() : false;

    return true;
}


bool StringServerSetting::setMembers(QJsonObject jsonObj)
{
    if (jsonObj.contains("port"))
        m_port = jsonObj.value("port").toInt();
    else
    {
        m_error = "[SETTINGS ERROR] Missing a tcp_servers port field.";
        return false;
    }

    m_parseJson = jsonObj.contains("parse_json") ? jsonObj.value("parse_json").toBool() : false;
    m_translate = jsonObj.contains("translate") ? jsonObj.value("translate").toBool() : true;

    if (m_translate && jsonObj.contains("translate_id"))
        m_translateId = jsonObj.value("translate_id").toString();
    else if (m_translate)
    {
        m_error = "[SETTINGS ERROR] Missing a tcp_servers translate_id field.";
        return false;
    }

    m_primaryConnection = jsonObj.contains("primary_connection") ? jsonObj.value("primary_connection").toBool() : false;

    return true;
}
