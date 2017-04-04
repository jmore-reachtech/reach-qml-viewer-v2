#ifndef BEEP_H
#define BEEP_H

#include <QObject>
#include <QDebug>
#include <QProcess>
#include <alsa/asoundlib.h>
#include <alsa/control.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#pragma pack (1)
/////////////////////// WAVE File Stuff /////////////////////
// An IFF file header looks like this
typedef struct _FILE_head
{
    unsigned char	ID[4];	// could be {'R', 'I', 'F', 'F'} or {'F', 'O', 'R', 'M'}
    unsigned int	Length;	// Length of subsequent file (including remainder of header). This is in
                                    // Intel reverse byte order if RIFF, Motorola format if FORM.
    unsigned char	Type[4];	// {'W', 'A', 'V', 'E'} or {'A', 'I', 'F', 'F'}
} FILE_head;


// An IFF chunk header looks like this
typedef struct _CHUNK_head
{
    unsigned char ID[4];	// 4 ascii chars that is the chunk ID
    unsigned int	Length;	// Length of subsequent data within this chunk. This is in Intel reverse byte
                            // order if RIFF, Motorola format if FORM. Note: this doesn't include any
                            // extra byte needed to pad the chunk out to an even size.
} CHUNK_head;

// WAVE fmt chunk
typedef struct _FORMAT {
    short			wFormatTag;
    unsigned short	wChannels;
    unsigned int	dwSamplesPerSec;
    unsigned int	dwAvgBytesPerSec;
    unsigned short	wBlockAlign;
    unsigned short	wBitsPerSample;
  // Note: there may be additional fields here, depending upon wFormatTag
} FORMAT;
#pragma pack()

// Size of the audio card hardware buffer. Here we want it
// set to 1024 16-bit sample points. This is relatively
// small in order to minimize latency. If you have trouble
// with underruns, you may need to increase this, and PERIODSIZE
// (trading off lower latency for more stability)
#define BUFFERSIZE	(2*1024)

// How many sample points the ALSA card plays before it calls
// our callback to fill some more of the audio card's hardware
// buffer. Here we want ALSA to call our callback after every
// 64 sample points have been played
#define PERIODSIZE	(2*64)

// The name of the ALSA port we output to. In this case, we're
// directly writing to hardware card 0,0 (ie, first set of audio
// outputs on the first audio card)
static const char SoundCardPortName[] = "default";

// For WAVE file loading
static const unsigned char Riff[4]	= { 'R', 'I', 'F', 'F' };
static const unsigned char Wave[4] = { 'W', 'A', 'V', 'E' };
static const unsigned char Fmt[4] = { 'f', 'm', 't', ' ' };
static const unsigned char Data[4] = { 'd', 'a', 't', 'a' };

// For the modules with no soundcard
#define BEEPER "/sys/kernel/beeper/beep"
#define VOLUME "/sys/kernel/beeper/vol"
#define FREQUENCY "/sys/kernel/beeper/freq"
#define DURATION "/sys/kernel/beeper/duration"

class Beep : public QObject
{
    Q_OBJECT
public:
    explicit Beep(QObject *parent = 0);
    ~Beep();

signals:

public slots:
    bool openwave(const QString &path);
    void deinit();
    void play();
    void play(const int frequency, const int duration);
    bool isOpen();
    bool init();
    bool init(const int frequency, const int duration);
    int duration();
    int frequency();
    bool isSoundCard();
    void setVolume(int volume);
    int volume();

private slots:
    unsigned char compareID(const unsigned char * id, unsigned char * ptr);
    bool loadWaveFile(const char *fn);
    QString execute(QString command);


private:
    // Handle to ALSA (audio card's) playback port
    snd_pcm_t *m_playbackHandle;
    // Handle to our callback thread
    //snd_async_handler_t	*m_callbackHandle;
    // Points to loaded WAVE file's data
    unsigned char *m_wavePtr;
    // Size (in frames) of loaded WAVE file's data
    snd_pcm_uframes_t m_waveSize;
    // Sample rate
    unsigned short m_waveRate;
    // Bit resolution
    unsigned char m_waveBits;
    // Number of channels in the wave file
    unsigned char m_waveChannels;
    // bit format of wave file
    snd_pcm_format_t m_format;
    // is the sound card open
    bool m_open;
    // sound card volume
    int m_volume;

    // For the modules with no soundcard
    int m_duration;
    int m_frequency;
};

#endif // BEEP_H
