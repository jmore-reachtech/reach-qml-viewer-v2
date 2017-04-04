#include "beep.h"

Beep::Beep(QObject *parent) :
    QObject(parent)
{
    m_wavePtr = NULL;
    m_open = false;
    m_duration = 2000;
    m_frequency = 50;
}

Beep::~Beep()
{
    deinit();
    if (m_wavePtr)
        delete(m_wavePtr);
}

bool Beep::init()
{
    if (isOpen()) {
        qDebug() << "{QML] sound card is already open";
        return true;
    }

    // Open audio card we wish to use for playback
    register int err;
    if ((err = snd_pcm_open(&m_playbackHandle, &SoundCardPortName[0], SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        qDebug("[QML] can't open audio %s: %s\n", &SoundCardPortName[0], snd_strerror(err));
        return false;
    }
    else
      qDebug() << "[QML] sound card is open";

    m_open = true;
    return true;
}

bool Beep::init(const int frequency, const int duration)
{
    m_frequency = frequency;
    m_duration = duration;
    qDebug("[QML] beeper frequency %d and duration %d set", frequency, duration);
    return true;
}

int Beep::duration()
{
    return m_duration;
}

int Beep::frequency()
{
    return m_frequency;
}

bool Beep::isSoundCard()
{
    QString test = execute("aplay -l");
    if (test.indexOf("Dummy") >= 0)
        return false;
    else
        return true;

}

void Beep::setVolume(int volume)
{
    if (volume >=0 && volume <= 100)
    {
        m_volume = volume;
        QString output = execute(QString("amixer set PCM ").append(QString::number(volume).append("%")));
        qDebug() << "[QML]" << output;
    }
    else
        qDebug() << "{QML] amixer error: volume must be set between 0 and 100";
}

int Beep::volume()
{
    return m_volume;
}

void Beep::deinit()
{
    // check if we need to close the sound card
    if (isOpen() && m_wavePtr) {
        snd_pcm_close(m_playbackHandle);
        m_open = false;
        qDebug() << "[QML] sound card closed";
    }
}

bool Beep::isOpen()
{
    return m_open;
}

bool Beep::openwave(const QString &path)
{
    // No wave data loaded yet
    m_wavePtr = NULL;

    if (!loadWaveFile(path.toUtf8().data())) {
        return false;
    }
    else {
        // Open audio card we wish to use for playback
        if (isOpen()) {

            switch (m_waveBits)
            {
                case 8:
                    m_format = SND_PCM_FORMAT_U8;
                    break;

                case 16:
                    m_format = SND_PCM_FORMAT_S16;
                    break;

                case 24:
                    m_format = SND_PCM_FORMAT_S24;
                    break;

                case 32:
                    m_format = SND_PCM_FORMAT_S32;
                    break;
            }

            register int err;
            // Set the audio card's hardware parameters (sample rate, bit resolution, etc)
            if ((err = snd_pcm_set_params(m_playbackHandle, m_format, SND_PCM_ACCESS_RW_INTERLEAVED, m_waveChannels, m_waveRate, 1, 100000)) < 0)
            {
                qDebug("[QML] can't set sound parameters: %s\n", snd_strerror(err));
                return false;
            }

        }
    }

    return true;
}

void Beep::play()
{
    if (m_wavePtr && isOpen())
    {
        register snd_pcm_uframes_t	count;
        int frames;

        snd_pcm_prepare(m_playbackHandle);

        // Output the wave data
        count = 0;
        do
        {
            frames = snd_pcm_writei(m_playbackHandle, m_wavePtr + count, m_waveSize - count);

            // If an error, try to recover from it
            if (frames < 0)
                frames = snd_pcm_recover(m_playbackHandle, frames, 0);
            if (frames < 0)
            {
                qDebug("[QML] error playing wave: %s\n", snd_strerror(frames));
                break;
            }

            // Update our pointer
            count += frames;

        } while (count < m_waveSize);

        // Wait for playback to completely finish
        if (count == m_waveSize)
            snd_pcm_drain(m_playbackHandle);
    }
    else
        play(m_frequency, m_duration);
}

void Beep::play(const int frequency, const int duration)
{
    execute(QString("beep -f ").append(QString::number(frequency).append(" -d ").append(QString::number(duration))));
}

QString Beep::execute(QString command)
{
    QProcess p(this);
    p.setProcessChannelMode(QProcess::MergedChannels);
    qDebug() << "executing " << command << "\n";

    p.start(command);

    QByteArray data;

    while(p.waitForReadyRead())
        data.append(p.readAll());

    return QString::fromLatin1(data.data());
}

unsigned char Beep::compareID(const unsigned char *id, unsigned char *ptr)
{
    register unsigned char i = 4;

    while (i--)
    {
        if ( *(id)++ != *(ptr)++ ) return(0);
    }
    return(1);
}

bool Beep::loadWaveFile(const char *fn)
{
    FILE_head head;
    register int inHandle;

    if ((inHandle = open(fn, O_RDONLY)) == -1)
    {
        qDebug() << "[QML] could not open wave file:" << fn ;
        return false;
    }
    else
    {
        // Read in IFF File header
        if (read(inHandle, &head, sizeof(FILE_head)) == sizeof(FILE_head))
        {
            // Is it a RIFF and WAVE?
            if (!compareID(&Riff[0], &head.ID[0]) || !compareID(&Wave[0], &head.Type[0]))
            {
                close(inHandle);
                qDebug() << "[QML] " << fn << "is not a wave file.";
                return false;
            }

            // Read in next chunk header
            while (read(inHandle, &head, sizeof(CHUNK_head)) == sizeof(CHUNK_head))
            {
                // ============================ Is it a fmt chunk? ===============================
                if (compareID(&Fmt[0], &head.ID[0]))
                {
                    FORMAT	format;

                    // Read in the remainder of chunk
                    if (read(inHandle, &format.wFormatTag, sizeof(FORMAT)) != sizeof(FORMAT)) break;

                    // Can't handle compressed WAVE files
                    if (format.wFormatTag != 1)
                    {
                        close(inHandle);
                        qDebug() << "[QML] compressed wave file is not supported";
                        return false;
                    }

                    m_waveBits = (unsigned char)format.wBitsPerSample;
                    m_waveRate = (unsigned short)format.dwSamplesPerSec;
                    m_waveChannels = format.wChannels;
                }

                // ============================ Is it a data chunk? ===============================
                else if (compareID(&Data[0], &head.ID[0]))
                {
                    // Size of wave data is head.Length. Allocate a buffer and read in the wave data
                    if (!(m_wavePtr = (unsigned char *)malloc(head.Length)))
                    {
                        close(inHandle);
                        qDebug() << "[QML] wave file won't fit in RAM";
                        return false;
                    }

                    if (read(inHandle, m_wavePtr, head.Length) != head.Length)
                    {
                        close(inHandle);
                        free(m_wavePtr);
                        return false;
                    }

                    // Store size (in frames)
                    m_waveSize = (head.Length * 8) / ((unsigned int)m_waveBits * (unsigned int)m_waveChannels);

                    close(inHandle);
                    break;
                }

                // ============================ Skip this chunk ===============================
                else
                {
                    if (head.Length & 1) ++head.Length;  // If odd, round it up to account for pad byte
                    lseek(inHandle, head.Length, SEEK_CUR);
                }
            }
        }
    }

    qDebug("[QML] beeper wave file loaded %s", fn);
    return true;

}


