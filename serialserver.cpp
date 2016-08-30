#include "serialserver.h"
#include <QTimer>

SerialServer::SerialServer(const SerialServerSetting portInfo, QObject *parent) :
    QObject(parent)
   ,m_server(new QSerialPort(this))
{
    m_parseJson = portInfo.parseJson();
    m_translate = portInfo.translate();
    m_translateID = portInfo.translateId();
    m_primaryConnection = portInfo.primaryConnection();
    m_portName = portInfo.portName();
    m_server->setPortName(portInfo.winPort());
    m_server->setBaudRate(portInfo.baudRate(), QSerialPort::AllDirections);
    m_server->setStopBits(getStopBitsEnum(portInfo.stopBits()));
    m_server->setParity(getParityEnum(portInfo.parity()));
    m_server->setDataBits(getDataBitsEnum(portInfo.dataBits()));
    m_server->setFlowControl(getFlowControlEnum(portInfo.flowControl()));
}

SerialServer::~SerialServer()
{
    if (m_server->isOpen())
        m_server->close();
}

bool SerialServer::Start()
{
    if (m_server->open((QIODevice::ReadWrite)))
    {
        qDebug() << "[QMLVIEWER] Serial port opened:" << m_server->portName();
        emit ClientConnected();
        connect(m_server, SIGNAL(readyRead()), this, SLOT(onClientReadyRead()));
        connect(m_server, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(onClientError(QSerialPort::SerialPortError)));
        if (m_primaryConnection)
            emit PrimaryConnectionAvailable();        
        return true;
    }
    else
    {
        emit Error("Error with the settings.json file.\nCould not open serial port " + m_server->portName());
        qDebug() << "{QMLVIEWER] Error: Could not open serial port " << m_server->portName() << m_server->errorString();
        return false;
    }
}

int SerialServer::Send(QString msg)
{
    msg.append("\r\n");
    int bytes = 0;
    if (m_server->isOpen())
    {
        bytes = m_server->write(msg.toUtf8());
        if (bytes > 0)
            qDebug() << "[QMLVIEWER " << m_portName << " SENT]" << msg;
        else
            qDebug() << "[QMLVIEWER] Error: Message could not be sent:" << msg << ". Check connections.";
    }

    return bytes;
}


bool SerialServer::getParseJon()
{
    return m_parseJson;
}


bool SerialServer::getTranslate()
{
    return m_translate;
}


QString SerialServer::getTranslateID()
{
    return m_translateID;
}

QString SerialServer::getPortName()
{
    return m_portName;
}


void SerialServer::onClientReadyRead()
{
        while (m_server->bytesAvailable() && m_server->canReadLine()) {
            QByteArray ba = m_server->readLine();
            emit MessageAvailable(ba, m_parseJson, m_translate, m_translateID);
        }
}

void SerialServer::onClientError(QSerialPort::SerialPortError error)
{
    if(error != QSerialPort::NoError)
    {
        QMetaEnum metaEnum =  QSerialPort::staticMetaObject.enumerator(
                    QSerialPort::staticMetaObject.indexOfEnumerator("SerialPortError"));

        QString errStr(metaEnum.valueToKey(error));

        qDebug() << errStr;
    }
}

QSerialPort::StopBits SerialServer::getStopBitsEnum(int stopBits){
    if (stopBits == 1)
        return QSerialPort::OneStop;
    else if (stopBits == 3)
        return QSerialPort::OneAndHalfStop;
    else if (stopBits == 2)
        return QSerialPort::TwoStop;
    else return QSerialPort::UnknownStopBits;
}

QSerialPort::Parity SerialServer::getParityEnum(QString parity)
{
    if (parity == "none")
        return QSerialPort::NoParity;
    else if (parity == "even")
        return QSerialPort::EvenParity;
    else if (parity == "odd")
        return QSerialPort::OddParity;
    else if (parity == "space")
        return QSerialPort::SpaceParity;
    else if (parity == "mark")
        return QSerialPort::MarkParity;
    else
        return QSerialPort::UnknownParity;
}

QSerialPort::FlowControl SerialServer::getFlowControlEnum(QString flowControl)
{
    if (flowControl == "off")
        return QSerialPort::NoFlowControl;
    else if (flowControl == "on")
        return QSerialPort::SoftwareControl;
    else if (flowControl == "xon/xoff")
        return QSerialPort::HardwareControl;
    else
        return QSerialPort::UnknownFlowControl;
}

QSerialPort::DataBits SerialServer::getDataBitsEnum(int dataBits)
{
    if (dataBits == 8)
        return QSerialPort::Data8;
    else if (dataBits == 7)
        return QSerialPort::Data7;
    else
        return QSerialPort::UnknownDataBits;
}

