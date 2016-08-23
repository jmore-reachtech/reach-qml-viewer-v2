#include "translator.h"

Translator::Translator(QString translateFile , QObject *parent) :
    QObject(parent)
  ,m_watcher (new QFileSystemWatcher(this))
{
    m_translateFile = translateFile;
    if (m_translateFile.length() == 0)
        m_translateFile = TRANSLATION_FILE_PATH;

#ifdef Q_OS_LINUX
    /* Fix the translate file path.  This incase someone copies a project from Windows */
    if (m_translateFile.indexOf("/") < 0)
        m_translateFile.prepend("/application/src/");
#endif

    /* Add a watcher to the translate file, so we can reload it when updated. */
    connect(m_watcher, SIGNAL(fileChanged(const QString &)), this, SLOT(onFileChanged(const QString &)));
    m_watcher->addPath(m_translateFile.toLatin1());

    m_defaultGuiMessage.set = false;
    m_defaultGuiMessage.message = "";
}


Translator::~Translator()
{
    if (m_watcher)
        delete m_watcher;
}


bool Translator::loadTranslations()
{
    m_guiHash.clear();
    m_mcuHash.clear();
    m_translationCount = 0;
    m_defaultGuiMessage.set = false;
    m_defaultGuiMessage.message = "";
    m_mcuDefaultMessages.clear();

    QFile inputFile;    
    inputFile.setFileName(m_translateFile.toLatin1());

    if (inputFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "[TRANSLATE] Loading" << m_translateFile;

        QTextStream in(&inputFile);
        int i = 1;
        while (!in.atEnd())
        {
            if (m_translationCount > MAX_MSG_MAP_SIZE)
            {
                qDebug() << "[TRANSLATE] Too many translation rules, maximum allowed: " << MAX_MSG_MAP_SIZE;
                break;
            }

            QString line = in.readLine();
            if (line.isEmpty() || line.startsWith("#") || line.startsWith("/"))
                qDebug() << "[TRANSLATE] Ignoring line" << i << ":" << line;
            else
                traslateAddMapping(line, i);
            i+=1;
        }

        inputFile.close();
        return true;
    }
    else
    {
        qDebug() << "[QMLVIEWER] Could not open translate file: " << inputFile.fileName().toLatin1();
        return false;
    }
}


QString Translator::translateGuiMessage(QString message)
{
    /* Check for empty message */
    if (message.isEmpty() || message.isNull())
        return "";

    /* see if we have a key */
    QString key = message.mid(0, message.indexOf("=")+1);
    QString value = message.mid(message.indexOf("=")+1);

    if (m_guiHash.contains(key))
    {
        qDebug() << "[TRANSLATE] GUI key found:" << key;
        return m_guiHash.value(key).message + value;
    }
    else if (m_defaultGuiMessage.set)
        return message;

    return "";
}

QString Translator::translateMCUMessage(QString origin, QString message)
{
    /* Check for empty message */
    if (message.isEmpty() || message.isNull())
        return "";

    /* see if we have a key */
    QString key = origin + ":" + message.mid(0, message.indexOf("=")+1);
    QString value = message.mid(message.indexOf("=")+1, message.length());

    if (m_mcuHash.contains(key))
    {
        qDebug() << "[TRANSLATE] MCU key found:" << key;
        return m_mcuHash.value(key).message + value;
    }
    else if (m_mcuDefaultMessages.contains(origin))
        return message;

    return "";
}

bool Translator::traslateAddMapping(const QString line, int lineNumber)
{
    /*
         * Translations are one per line in the form of:
         * O:K,M:G\n
         *
         * where:
         *  O = origin (G = GUI, M = micro)
         *  K = key, a string to match* or % for default translation
         *  M = marker
         *  G = message
         *
         *  * The key can contain a "setter" of the form "=%d" or "=%s" which
         *    allows for numeric or string substitutions into the message.
         */

    QString origin, key, marker, message;

    if (line.startsWith("G") || line.startsWith("M"))
    {
        //Split the line on a comma
        QStringList list = line.split(",");
        if (list.length() != 2)
        {
            qDebug() << "[TRANSLATE] line in wrong format" << lineNumber << ":" << line;
            return false;
        }

        //Split both lines on colon
        QStringList originKey = list[0].split(":");
        QStringList markerMessage = list[1].split(":");

        if (originKey.length() != 2 || markerMessage.length() != 2)
        {
            qDebug() << "[TRANSLATE] line in wrong format" << lineNumber << ":" << line;
            return false;
        }

        //Add to Struct and Hash
        QString origin = originKey[0];
        if (origin != "G" && !origin.startsWith("M"))
        {
            qDebug() << "[TRANSLATE] line in wrong format" << lineNumber << ":" << line;
            return false;
        }
        QString key = originKey[1];
        if (key.indexOf("%d") < 0 && key.indexOf("%s") < 0)
        {
            qDebug() << "[TRANSLATE] line in wrong format" << lineNumber << ":" << line;
            return false;
        }

        QString marker = markerMessage[0];
        if (marker != "T")
        {
            qDebug() << "[TRANSLATE] line in wrong format" << lineNumber << ":" << line;
            return false;
        }
        QString message = markerMessage[1];
        if (message.indexOf("%d") < 0 && message.indexOf("%s") < 0)
        {
            qDebug() << "[TRANSLATE] line in wrong format" << lineNumber << ":" << line;
            return false;
        }


        KeyValue kv;

        if (key.indexOf("%d") > 0)
        {
            kv.type = "d";
            key.replace("%d", "");
        }
        else
        {
            kv.type = "s";
            key.replace("%s", "");
        }

        if (message.indexOf("%d") > 0)
        {
            message = message.mid(0, message.indexOf("%d"));
        }
        else
        {
            message = message.mid(0, message.indexOf("%s"));
        }


        kv.marker = marker;
        kv.message = message;

        /* Add items to HASH */
        if (origin == "G")
        {
            if (key.length() == 0)
                m_defaultGuiMessage.set = true;
            else if (!m_guiHash.contains(key))
                m_guiHash.insert(key, kv);
            else
            {
                qDebug() << "[TRANSLATE] line " << lineNumber << " not added. GUI key already exists:" << key;
                return false;
            }

        }
        else
        {
            if (key.length() == 0)
            {
                if (!m_mcuDefaultMessages.contains(origin))
                    m_mcuDefaultMessages.insert(origin, key);
                else
                {
                    qDebug() << "[TRANSLATE] line " << lineNumber << " not added. MCU key already exists:" << key;
                    return false;
                }
            }
            else if (!m_mcuHash.contains(key))
                m_mcuHash.insert(origin + ":" + key, kv);
            else
            {
                qDebug() << "[TRANSLATE] line " << lineNumber << " not added. MCU key already exists:" << key;
                return false;
            }
        }

        m_translationCount += 1;
        return true;
    }
    else
    {
        qDebug() << "[TRANSLATE] Ignoring line" << lineNumber << ":" << line;
        return false;
    }
}

void Translator::onFileChanged(const QString &path)
{
    qDebug() << "[TRANSLATE] File has changed:" << path;
    m_watcher->addPath(TRANSLATION_FILE_PATH);
    loadTranslations();
}
