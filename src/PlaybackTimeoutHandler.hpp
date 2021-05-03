#ifndef PLAYBACKTIMEOUTHANDLER_HPP_
#define PLAYBACKTIMEOUTHANDLER_HPP_

#include "src/WavPlayer.hpp"

#include <bb/cascades/Application>
using namespace bb::cascades;

class PlaybackTimeoutHandler: public QObject
{
Q_OBJECT
public:
    PlaybackTimeoutHandler(WavPlayer *player, QObject *parent) :
            player(player), interval(0), QObject(parent)
    {
        timer = new QTimer(parent);
        timer->setSingleShot(true);

        QObject::connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));

        QObject::connect(Application::instance(), SIGNAL(thumbnail()), this, SLOT(onThumbnailed()));
        QObject::connect(Application::instance(), SIGNAL(fullscreen()), this, SLOT(onFullsreen()));
        QObject::connect(Application::instance(), SIGNAL(invisible()), this, SLOT(onInvisible()));
    }
    virtual ~PlaybackTimeoutHandler()
    {
    }
    void setInterval(int interval)
    {
        this->interval = interval;
    }
private slots:
    void onThumbnailed()
    {
        if (!timer->isActive() && interval > 0) {
            timer->start(interval * 1000);
        }
    }
    void onFullsreen()
    {
        if (timer->isActive()) {
            timer->stop();
        }
    }
    void onInvisible()
    {
        if (!timer->isActive() && interval > 0) {
            timer->start(interval * 1000);
        }
    }
    void onTimeout()
    {
        if (player->playing()) {
            player->stop();
        }
    }
private:
    QTimer *timer;
    WavPlayer *player;
    int interval;
};

#endif /* PLAYBACKTIMEOUTHANDLER_HPP_ */
