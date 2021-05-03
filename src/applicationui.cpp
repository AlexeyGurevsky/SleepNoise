#include "applicationui.hpp"

#include <bb/cascades/Application>
#include <bb/cascades/QmlDocument>
#include <bb/cascades/NavigationPane>
#include <bb/cascades/Page>
#include <bb/cascades/LocaleHandler>
#include <bb/cascades/Button>
#include <bb/cascades/Container>
#include <bb/cascades/StackLayout>
#include <bb/cascades/StackLayoutProperties>
#include <bb/cascades/LayoutOrientation>
#include <bb/cascades/TitleBar>
#include <bb/cascades/Label>
#include <bb/cascades/UIConfig>
#include <bb/cascades/Option>
#include <bb/cascades/TitleBarScrollBehavior>

#include <bb/multimedia/MediaState>
#include <bb/multimedia/MetaData>

#include <bb/ApplicationInfo>

using namespace bb::cascades;

WavPlayer* ApplicationUI::player = 0;

ApplicationUI::ApplicationUI() :
        QObject()
{
    // prepare the localization
    m_pTranslator = new QTranslator(this);
    m_pLocaleHandler = new LocaleHandler(this);
    player = new WavPlayer(this);
    playbackTimeoutHandler = new PlaybackTimeoutHandler(player, this);

    npc = new bb::multimedia::NowPlayingConnection("sleepnoise", this);
    npc->setOverlayStyle(bb::multimedia::OverlayStyle::Fancy);
    npc->setPreviousEnabled(false);
    npc->setNextEnabled(false);
    npc->setIconUrl(QUrl("file://" + QDir::currentPath() + "/app/native/icon.png"));

    QObject::connect(m_pLocaleHandler, SIGNAL(systemLanguageChanged()), this,
            SLOT(onSystemLanguageChanged()));

    onSystemLanguageChanged();

    NavigationPane *navigationPane = NavigationPane::create();
    QObject::connect(navigationPane, SIGNAL(popTransitionEnded(bb::cascades::Page*)), this,
            SLOT(popFinished(bb::cascades::Page*)));

    Container *root = Container::create();
    root->setTopPadding(root->ui()->du(2));

    dropDown = DropDown::create().title("Noise");
    QStringList filters;
    filters << "*.wav";
    QString appFolder(QDir::homePath());
    appFolder.chop(4);
    QDir assetsFolder = QDir(appFolder + "app/native/assets/sounds");
    assetsFolder.setSorting(QDir::Name);
    assetsFolder.setNameFilters(filters);
    assetsFolder.setFilter(QDir::Files);
    QFileInfoList files = assetsFolder.entryInfoList();
    for (int i = 0; i < files.count(); i++) {
        QString fileName = files[i].fileName();
        bool selected = fileName == "Pink.wav";
        QString fileNameWithoutExtension = fileName.left(fileName.indexOf('.'));

        dropDown->add(
                Option::create().text(fileNameWithoutExtension).value(files[i].filePath()).selected(
                        selected));
    }

    timerDropdown = DropDown::create().title("Timer");
    timerDropdown->add(Option::create().text("None").value(0).selected(true));
    timerDropdown->add(Option::create().text("5 minutes").value(5 * 60));
    timerDropdown->add(Option::create().text("10 minutes").value(10 * 60));
    timerDropdown->add(Option::create().text("15 minutes").value(15 * 60));
    timerDropdown->add(Option::create().text("20 minutes").value(20 * 60));
    timerDropdown->add(Option::create().text("30 minutes").value(30 * 60));
    timerDropdown->add(Option::create().text("45 minutes").value(45 * 60));
    timerDropdown->add(Option::create().text("1 hour").value(60 * 60));
    timerDropdown->add(Option::create().text("1.5 hours").value(1.5 * 60));
    timerDropdown->add(Option::create().text("2 hours").value(2 * 60 * 60));
    timerDropdown->add(Option::create().text("3 hours").value(3 * 60 * 60));
    timerDropdown->add(Option::create().text("4 hours").value(4 * 60 * 60));
    timerDropdown->add(Option::create().text("6 hours").value(6 * 60 * 60));
    timerDropdown->add(Option::create().text("8 hours").value(8 * 60 * 60));
    timerDropdown->add(Option::create().text("12 hours").value(12 * 60 * 60));
    timerDropdown->add(Option::create().text("24 hours").value(24 * 60 * 60));

    Container *buttonContainer = Container::create();
    StackLayout *stackLayout = new StackLayout();
    stackLayout->setOrientation(LayoutOrientation::LeftToRight);
    buttonContainer->setLayout(stackLayout);
    btnStart = Button::create("Start").layoutProperties(StackLayoutProperties::create().spaceQuota(1));
    btnStop = Button::create("Stop").layoutProperties(StackLayoutProperties::create().spaceQuota(1)).enabled(false);
    root->add(dropDown);
    root->add(timerDropdown);
    buttonContainer->add(btnStart);
    buttonContainer->add(btnStop);
    root->add(buttonContainer);

    TitleBar* tb = new TitleBar(bb::cascades::TitleBarScrollBehavior::Sticky);
    tb->setTitle("Sleep Noise");

    Container *aboutContainer = Container::create();
    UIConfig *ui = aboutContainer->ui();
    aboutContainer->setTopPadding(ui->du(2));
    aboutContainer->setRightPadding(ui->du(1));
    aboutContainer->setBottomPadding(ui->du(1));
    aboutContainer->setLeftPadding(ui->du(1));
    aboutContainer->add(
            Label::create().multiline(true).text(
                    "Author: Alexey Gurevski\nVersion:" + bb::ApplicationInfo().version()
                            + "\nFor more info visit the CrackBerry Forum."));
    root->add(aboutContainer);

    QObject::connect(btnStart, SIGNAL(clicked()), this, SLOT(onBtnStartClicked()));
    QObject::connect(btnStop, SIGNAL(clicked()), this, SLOT(onBtnStopClicked()));
    QObject::connect(player, SIGNAL(started()), this, SLOT(onPlayerStarted()));
    QObject::connect(player, SIGNAL(stopped()), this, SLOT(onPlayerStopped()));
    QObject::connect(npc, SIGNAL(acquired()), this, SLOT(onNpcAcquired()));
    QObject::connect(npc, SIGNAL(pause()), this, SLOT(onNpcPause()));
    QObject::connect(npc, SIGNAL(play()), this, SLOT(onNpcPlay()));
    QObject::connect(npc, SIGNAL(revoked()), this, SLOT(onNpcRevoked()));
    QObject::connect(timerDropdown, SIGNAL(selectedIndexChanged(int)), this, SLOT(onTimerSelectedIndexChanged(int)));

    navigationPane->push(Page::create().content(root).titleBar(tb));

    Application::instance()->setScene(navigationPane);
}

void ApplicationUI::popFinished(bb::cascades::Page* page)
{
    delete page;
}

void ApplicationUI::onSystemLanguageChanged()
{
    QCoreApplication::instance()->removeTranslator(m_pTranslator);
    QString locale_string = QLocale().name();
    QString file_name = QString("SleepNoise_%1").arg(locale_string);
    if (m_pTranslator->load(file_name, "app/native/qm")) {
        QCoreApplication::instance()->installTranslator(m_pTranslator);
    }
}

void ApplicationUI::onBtnStartClicked()
{
    player->setPath(dropDown->selectedValue().toString());
    player->play();
}

void ApplicationUI::onBtnStopClicked()
{
    player->stop();
    npc->revoke();
}

void ApplicationUI::onPlayerStarted()
{
    npc->acquire();
    npc->setMediaState(bb::multimedia::MediaState::Started);

    QVariantMap metadata;
    metadata[bb::multimedia::MetaData::Title] = dropDown->selectedOption()->text();
    metadata[bb::multimedia::MetaData::Artist] = "SleepNoise";
    npc->setMetaData(metadata);

    btnStart->setEnabled(false);
    btnStop->setEnabled(true);
}

void ApplicationUI::onPlayerStopped()
{
    npc->setMediaState(bb::multimedia::MediaState::Paused);
    btnStart->setEnabled(true);
    btnStop->setEnabled(false);
}

void ApplicationUI::onNpcAcquired()
{
    if (!player->playing()) {
        player->play();
        npc->setMediaState(bb::multimedia::MediaState::Started);
    }
}

void ApplicationUI::onNpcPause()
{
    player->stop();
}

void ApplicationUI::onNpcPlay()
{
    if (!player->playing()) {
        player->play();
    }
}

void ApplicationUI::onNpcRevoked()
{
    player->stop();
}

void ApplicationUI::onTimerSelectedIndexChanged(int index)
{
    playbackTimeoutHandler->setInterval(timerDropdown->selectedValue().toInt());
}
