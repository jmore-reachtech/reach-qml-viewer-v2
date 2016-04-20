#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QHash>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "systemdefs.h"

struct KeyValue{
    QString marker;
    QString message;
    QString type;
};

struct DefaultGuiMessage{
    bool set;
    QString message;
};

class Translator : public QObject
{
    Q_OBJECT
public:
    explicit Translator(QString translateFile, QObject *parent = 0);
    ~Translator();
signals:

public slots:
    bool loadTranslations();
    QString translateGuiMessage(QString message);
    QString translateMCUMessage(QString origin, QString message);

private slots:
    bool traslateAddMapping(const QString line, int lineNumber);
    void onFileChanged(const QString & path);

private:
    QString m_translateFile;
    QHash<QString, KeyValue> m_guiHash;
    QHash<QString, KeyValue> m_mcuHash;
    QFileSystemWatcher *m_watcher;
    int m_translationCount;
    DefaultGuiMessage m_defaultGuiMessage;
    QHash<QString, QString> m_mcuDefaultMessages;
};

#endif // TRANSLATOR_H
