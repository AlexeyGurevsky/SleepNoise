#ifndef WAVPLAYER_HPP_
#define WAVPLAYER_HPP_

#include "WavPlayerPrivate.hpp"
#include <QFuture>
#include <QFutureWatcher>

class WavPlayer: public QObject
{
    Q_OBJECT
public:
    WavPlayer(QObject *parent = 0);
    virtual ~WavPlayer();
    void setPath(QString pathToFile);
    void play();
    void stop();
    bool playing();
private:
    QString path;
    WavPlayerPrivate* player;
    bool isPlaying;
    QFuture<void> *future;
    QFutureWatcher<void> *watcher;
private slots:
    void onPlayerStarted();
    void onPlayerStopped();
signals:
    void started();
    void stopped();
};

#endif /* WAVPLAYER_HPP_ */
