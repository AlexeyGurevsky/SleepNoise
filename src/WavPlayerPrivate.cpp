#include "WavPlayerPrivate.hpp"

#include <errno.h>
#include <fcntl.h>
#include <gulliver.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/termio.h>
#include <sys/types.h>
#include <unistd.h>

static bool is_playing = false;

const char *kRiffId = "RIFF";
const char *kWaveId = "WAVE";

typedef struct
{
    char tag[4];
    long length;
} RiffTag;

typedef struct
{
    char Riff[4];
    long Size;
    char Wave[4];
} RiffHdr;

typedef struct
{
    short FormatTag;
    short Channels;
    long SamplesPerSec;
    long AvgBytesPerSec;
    short BlockAlign;
    short BitsPerSample;
} WaveHdr;

int WavPlayerPrivate::findTag(FILE * fp, const char *tag)
{
    int retVal;
    RiffTag tagBfr = { "", 0 };

    retVal = 0;

    // Keep reading until we find the tag or hit the EOF.
    while (fread((unsigned char *) &tagBfr, sizeof(tagBfr), 1, fp)) {

        // If this is our tag, set the length and break.
        if (strncmp(tag, tagBfr.tag, sizeof tagBfr.tag) == 0) {
            retVal = ENDIAN_LE32(tagBfr.length);
            break;
        }

        // Skip ahead the specified number of bytes in the stream
        fseek(fp, tagBfr.length, SEEK_CUR);
    }

    // Return the result of our operation
    return (retVal);
}

int WavPlayerPrivate::checkHdr(FILE * fp)
{
    RiffHdr riffHdr = { "", 0 };

    // Read the header and, if successful, play the file
    // file or WAVE file.
    if (fread((unsigned char *) &riffHdr, sizeof(RiffHdr), 1, fp) == 0)
        return 0;

    if (strncmp(riffHdr.Riff, kRiffId, strlen(kRiffId))
            || strncmp(riffHdr.Wave, kWaveId, strlen(kWaveId)))
        return -1;

    return 0;
}

int max(int a, int b)
{
    return a >= b ? a : b;
}

int min(int a, int b)
{
    return a <= b ? a : b;
}

WavPlayerPrivate::WavPlayerPrivate(QObject *parent) :
        QObject(parent)
{
}

WavPlayerPrivate::~WavPlayerPrivate()
{
    is_playing = false;
}

void WavPlayerPrivate::setPath(QString pathToFile)
{
    path = pathToFile;
}

void WavPlayerPrivate::playInner()
{
    int card = -1;
    int dev = 0;

    WaveHdr wavHdr;
    int samples;
    int sampleRate;
    int sampleChannels;
    int sampleBits;

    int rtn;
    snd_pcm_channel_info_t pi;
    snd_mixer_t *mixer_handle;
    snd_mixer_group_t group;
    snd_pcm_channel_params_t pp;
    snd_pcm_channel_setup_t setup;
    int bsize, n;
    fd_set rfds, wfds;

    if ((rtn = snd_pcm_open_preferred(&pcm_handle, &card, &dev, SND_PCM_OPEN_PLAYBACK)) < 0) {
        return;
    }

    if ((file = fopen(path.toLocal8Bit().constData(), "r")) == 0) {
        return;
    }

    if (checkHdr(file) == -1) {
        return;
    }

    samples = findTag(file, "fmt ");
    fread(&wavHdr, sizeof(wavHdr), 1, file);
    fseek(file, (samples - sizeof(WaveHdr)), SEEK_CUR);

    sampleRate = ENDIAN_LE32(wavHdr.SamplesPerSec);
    sampleChannels = ENDIAN_LE16(wavHdr.Channels);
    sampleBits = ENDIAN_LE16(wavHdr.BitsPerSample);

    memset(&pi, 0, sizeof(pi));
    pi.channel = SND_PCM_CHANNEL_PLAYBACK;
    if ((rtn = snd_pcm_plugin_info(pcm_handle, &pi)) < 0) {
        return;
    }

    memset(&pp, 0, sizeof(pp));

    pp.mode = SND_PCM_MODE_BLOCK;
    pp.channel = SND_PCM_CHANNEL_PLAYBACK;
    pp.start_mode = SND_PCM_START_FULL;
    pp.stop_mode = SND_PCM_STOP_STOP;

    pp.buf.block.frag_size = pi.max_fragment_size;
    pp.buf.block.frags_max = 1;
    pp.buf.block.frags_min = 1;

    pp.format.interleave = 1;
    pp.format.rate = sampleRate;
    pp.format.voices = sampleChannels;

    pp.format.format = (sampleBits == 8 ? SND_PCM_SFMT_U8 : SND_PCM_SFMT_S16_LE);

    if ((rtn = snd_pcm_plugin_params(pcm_handle, &pp)) < 0) {
        return;
    }

    if ((rtn = snd_pcm_plugin_prepare(pcm_handle, SND_PCM_CHANNEL_PLAYBACK)) < 0) {
        return;
    }

    memset(&setup, 0, sizeof(setup));
    memset(&group, 0, sizeof(group));
    setup.channel = SND_PCM_CHANNEL_PLAYBACK;
    setup.mixer_gid = &group.gid;
    if ((rtn = snd_pcm_plugin_setup(pcm_handle, &setup)) < 0) {
        return;
    }
    if (group.gid.name[0] == 0) {
        return;
    }
    if ((rtn = snd_mixer_open(&mixer_handle, card, setup.mixer_device)) < 0) {
        return;
    }

    samples = findTag(file, "data");
    bsize = setup.buf.block.frag_size;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    n = 1;

    int i = 0;
    int whatToPlayPos = 0;
    int whatToPlaySize = 200 * bsize; // magic constant, ~ 5 seconds
    char *whatToPlay = (char*) malloc(whatToPlaySize);
    char bfr;
    for (i = 0; i < whatToPlaySize; i++) {
        if ((n = fread(&bfr, 1, 1, file)) <= 0) {
            break;
        }
        whatToPlay[i] = bfr;
    }
    fclose(file);

    FD_SET(STDIN_FILENO, &rfds);
    FD_SET(snd_mixer_file_descriptor(mixer_handle), &rfds);
    FD_SET(snd_pcm_file_descriptor (pcm_handle, SND_PCM_CHANNEL_PLAYBACK), &wfds);

    rtn = max(snd_mixer_file_descriptor(mixer_handle),
            snd_pcm_file_descriptor(pcm_handle, SND_PCM_CHANNEL_PLAYBACK));

    if (select(rtn + 1, &rfds, &wfds, NULL, NULL) == -1) {
        goto dispose;
    }

    if (FD_ISSET(snd_pcm_file_descriptor (pcm_handle, SND_PCM_CHANNEL_PLAYBACK), &wfds)) {
        emit started();

        while (is_playing) {
            while (n > 0 && is_playing) {
                n = min(whatToPlaySize - whatToPlayPos, bsize);
                if (n <= 0) {
                    continue;
                }

                snd_pcm_channel_status_t status;
                int written = 0;
                written = snd_pcm_plugin_write(pcm_handle, whatToPlay + whatToPlayPos, n);

                if (written < n) {
                    memset(&status, 0, sizeof(status));
                    status.channel = SND_PCM_CHANNEL_PLAYBACK;
                    if (snd_pcm_plugin_status(pcm_handle, &status) < 0) {
                        goto dispose;
                    }

                    if (status.status == SND_PCM_STATUS_READY
                            || status.status == SND_PCM_STATUS_UNDERRUN) {
                        if (snd_pcm_plugin_prepare(pcm_handle, SND_PCM_CHANNEL_PLAYBACK) < 0) {
                            goto dispose;
                        }
                    }
                    if (written < 0) {
                        written = 0;
                    }

                    written += snd_pcm_plugin_write(pcm_handle,
                            whatToPlay + whatToPlayPos + written, n - written);
                }

                whatToPlayPos += n;
            }

            whatToPlayPos = 0;
            n = 1;
        }
    }

    dispose:

    snd_pcm_plugin_flush(pcm_handle, SND_PCM_CHANNEL_PLAYBACK);
    snd_mixer_close(mixer_handle);
    snd_pcm_close(pcm_handle);
    free(whatToPlay);

    emit stopped();
}

void WavPlayerPrivate::start()
{
    setIsPlaying(true);
}

void WavPlayerPrivate::stop()
{
    setIsPlaying(false);
}

void WavPlayerPrivate::setIsPlaying(bool isPlaying)
{
    is_playing = isPlaying;
}
