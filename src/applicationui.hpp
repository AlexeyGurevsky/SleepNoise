#ifndef ApplicationUI_HPP_
#define ApplicationUI_HPP_

#include "src/WavPlayer.hpp"
#include "src/PlaybackTimeoutHandler.hpp"

#include <QObject>
#include <bb/cascades/Page>
#include <bb/multimedia/NowPlayingConnection>
#include <bb/cascades/Dropdown>
#include <bb/cascades/Button>

namespace bb
{
    namespace cascades
    {
        class LocaleHandler;
    }
}

class QTranslator;

class ApplicationUI: public QObject
{
Q_OBJECT
public:
    ApplicationUI();
    virtual ~ApplicationUI()
    {
    }

    static WavPlayer *player;
private:
    QTranslator* m_pTranslator;
    bb::cascades::LocaleHandler* m_pLocaleHandler;
    bb::multimedia::NowPlayingConnection *npc;
    bb::cascades::DropDown *dropDown;
    bb::cascades::DropDown *timerDropdown;
    PlaybackTimeoutHandler *playbackTimeoutHandler;
    bb::cascades::Button *btnStart;
    bb::cascades::Button *btnStop;
private slots:
    void onSystemLanguageChanged();
    void popFinished(bb::cascades::Page* page);
    void onBtnStartClicked();
    void onBtnStopClicked();
    void onPlayerStarted();
    void onPlayerStopped();
    void onNpcAcquired();
    void onNpcPause();
    void onNpcPlay();
    void onNpcRevoked();
    void onTimerSelectedIndexChanged(int);
};

#endif /* ApplicationUI_HPP_ */
