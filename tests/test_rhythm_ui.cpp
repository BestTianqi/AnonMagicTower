#include <QApplication>
#include <QMediaPlayer>
#include <QComboBox>
#include <QPushButton>
#include <QKeyEvent>
#include <QSettings>
#include <QTemporaryDir>
#include <QEventLoop>
#include <QTimer>
#include <iostream>
#include "UI/RhythmGamePage.h"
static void wait(int ms) { QEventLoop loop; QTimer::singleShot(ms,&loop,&QEventLoop::quit); loop.exec(); }
static void require(bool ok,const char* message) { if(!ok){ std::cerr<<message<<'\n'; std::exit(1); } }
int main(int argc,char** argv) {
    QApplication app(argc,argv);
    QTemporaryDir settings;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());
    RhythmGamePage page; page.resize(1600,900); page.show();
    auto* songs=page.findChild<QComboBox*>("rhythmSong");
    auto* start=page.findChild<QPushButton*>("rhythmStart");
    auto* player=page.findChild<QMediaPlayer*>();
    require(songs && songs->count()==6,"six playable songs required");
    require(page.findChild<QComboBox*>("rhythmSpeed")->count()==5,"five independent scroll speeds");
    for(int i=0;i<6;++i) {
        songs->setCurrentIndex(i); start->click(); wait(150);
        require(player->position()==0,"countdown must not consume audio or notes");
        wait(2300);
        require(player->error()==QMediaPlayer::NoError,"song must decode");
        require(player->duration()>80000,"excerpt must load");
        require(player->position()>0,"audio playback must advance");
        QKeyEvent pause(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier); QApplication::sendEvent(&page,&pause);
        require(player->playbackState()==QMediaPlayer::PausedState,"space must pause");
        QApplication::sendEvent(&page,&pause); wait(100);
        require(player->playbackState()==QMediaPlayer::PausedState,"resume countdown must keep music paused");
        wait(1150);
        require(player->playbackState()==QMediaPlayer::PlayingState,"space must resume");
        player->setPosition(player->duration()-100); wait(500);
        require(songs->isEnabled(),"end must allow choosing another song");
    }
    songs->setCurrentIndex(0); start->click(); wait(3300);
    page.grab().save(QCoreApplication::applicationDirPath()+"/rhythm-preview.png");
    QEvent deactivate(QEvent::WindowDeactivate); QApplication::sendEvent(&page,&deactivate);
    require(player->playbackState()==QMediaPlayer::PausedState,"focus loss must pause");
    page.findChild<QPushButton*>("rhythmSelect")->click();
    require(songs->isEnabled() && player->playbackState()==QMediaPlayer::StoppedState,"select song stops unfinished run");
    start->click(); wait(100);
    QEvent deactivateCountdown(QEvent::WindowDeactivate); QApplication::sendEvent(&page,&deactivateCountdown);
    wait(2100);
    require(player->playbackState()!=QMediaPlayer::PlayingState,"focus loss during countdown must not auto start");
    bool returned=false; QObject::connect(&page,&RhythmGamePage::returnToMenu,[&]{returned=true;});
    page.findChild<QPushButton*>("rhythmBack")->click();
    require(returned && player->playbackState()==QMediaPlayer::StoppedState,"back stops music");
    std::cout<<"six songs decode; countdown/pause/resume/end/select/focus-loss/back passed\n";
}
