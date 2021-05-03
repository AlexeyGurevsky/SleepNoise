#include "WavPlayer.hpp"
#include "WavPlayerPrivate.hpp"
#include <QtCore/QtConcurrentrun.h>

WavPlayer::WavPlayer(QObject *parent) :
        QObject(parent), isPlaying(false), future(0)
{
    player = new WavPlayerPrivate(parent);

    QObject::connect(player, SIGNAL(started()), this, SLOT(onPlayerStarted()));
    QObject::connect(player, SIGNAL(stopped()), this, SLOT(onPlayerStopped()));
}

WavPlayer::~WavPlayer()
{
    stop();
}

void WavPlayer::setPath(QString pathToFile)
{
    path = pathToFile;
}

void WavPlayer::play()
{
    if (path == "") {
        return;
    }

    stop();

    player->setPath(path);
    player->start();

    future = new QFuture<void>;
    watcher = new QFutureWatcher<void>;
    *future = QtConcurrent::run(player, &WavPlayerPrivate::playInner);
    watcher->setFuture(*future);
}

void WavPlayer::stop()
{
    player->stop();

    if (future && future->isRunning()) {
        future->waitForFinished();
    }
}

void WavPlayer::onPlayerStarted()
{
    isPlaying = true;
    emit started();
}

void WavPlayer::onPlayerStopped()
{
    isPlaying = false;
    emit stopped();
}

bool WavPlayer::playing()
{
    return isPlaying;
}
