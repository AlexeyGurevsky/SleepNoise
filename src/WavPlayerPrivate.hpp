#ifndef WAVPLAYERPRIVATE_HPP_
#define WAVPLAYERPRIVATE_HPP_

#include <stdio.h>
#include <sys/asoundlib.h>

class WavPlayerPrivate: public QObject
{
    Q_OBJECT
public:
    WavPlayerPrivate(QObject *parent = 0);
    virtual ~WavPlayerPrivate();
    void setPath(QString pathToFile);
    void playInner();
    void start();
    void stop();
private:
    QString path;

    snd_pcm_t *pcm_handle;
    snd_mixer_t *mixer_handle;
    FILE *file;

    int findTag(FILE * fp, const char *tag);
    int checkHdr(FILE * fp);
    void setIsPlaying(bool isPlaying);
signals:
    void started();
    void stopped();
};

#endif /* WAVPLAYERPRIVATE_HPP_ */
