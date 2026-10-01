#include "RhythmGamePage.h"
#include "Audio/GameAudio.h"
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QCoreApplication>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QPainter>
#include <QLinearGradient>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QUrl>

namespace {
const QColor colors[] = {QColor("#f29abb"), QColor("#91caff"), QColor("#b6a0f8"), QColor("#f3cf91")};
int laneForKey(int key) {
    switch(key) { case Qt::Key_D: return 0; case Qt::Key_F: return 1;
        case Qt::Key_J: return 2; case Qt::Key_K: return 3; default: return -1; }
}
}

RhythmGamePage::RhythmGamePage(QWidget* parent) : QWidget(parent) {
    setObjectName("rhythmPage"); setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setStyleSheet(
        "QLabel { color: #f6e9ef; background: transparent; }"
        "QPushButton { border-image: none; background: #322a49; border: 1px solid #8d7299; border-radius: 9px; color: #fff0dc; padding: 9px 18px; font-size: 15px; }"
        "QPushButton:hover { background: #594260; border-color: #f5b3d0; }"
        "QPushButton:disabled { color: #777080; border-color: #494051; }"
        "QComboBox, QSpinBox { background: #231e35; color: #fff1ed; border: 1px solid #7d658c; border-radius: 7px; padding: 8px; font-size: 14px; }"
        "QComboBox QAbstractItemView { background: #231e35; color: white; selection-background-color: #705470; }"
        "QSlider::groove:horizontal { height: 5px; background: #645271; border-radius: 2px; }"
        "QSlider::handle:horizontal { width: 13px; margin: -5px 0; background: #ffc8da; border-radius: 6px; }");
    m_background.load(":/images/backgrounds/mujica_theater.png");
    m_anon.load(":/images/characters/portraits/anon.png");
    m_soyo.load(":/images/characters/portraits/soyo_stage.png");
    m_audioDirectory = QCoreApplication::applicationDirPath()+"/Audio/rhythm/";
    QFile chart(m_audioDirectory+"tracks.json");
    if (chart.open(QIODevice::ReadOnly)) m_tracks = QJsonDocument::fromJson(chart.readAll()).array();
    if (m_tracks.isEmpty()) m_error = QString::fromUtf8("音乐文件未找到，请保留程序旁的 Audio/rhythm 文件夹。");
    auto* layout = new QVBoxLayout(this); layout->setContentsMargins(30,20,30,18); layout->setSpacing(12);
    auto* top = new QHBoxLayout;
    auto* title = new QLabel(QString::fromUtf8("音符之间  /  MYGO RHYTHM"));
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: #ffd0e2;");
    top->addWidget(title); top->addStretch();
    auto* back = new QPushButton(QString::fromUtf8("返回主菜单")); back->setObjectName("rhythmBack");
    top->addWidget(back); layout->addLayout(top);
    auto* controls = new QHBoxLayout;
    m_song = new QComboBox; m_song->setObjectName("rhythmSong"); m_song->setMinimumWidth(245);
    for (const auto& track : m_tracks) m_song->addItem(track.toObject()["title"].toString());
    m_difficulty = new QComboBox; m_difficulty->setObjectName("rhythmDifficulty"); m_difficulty->addItems({QString::fromUtf8("轻奏 · EASY"),QString::fromUtf8("合奏 · NORMAL"),QString::fromUtf8("闪耀 · HARD")});
    m_difficulty->setCurrentIndex(1);
    m_start = new QPushButton(QString::fromUtf8("开始演奏")); m_start->setObjectName("rhythmStart");
    m_pause = new QPushButton(QString::fromUtf8("暂停 / 空格")); m_pause->setEnabled(false);
    m_select = new QPushButton(QString::fromUtf8("结束选曲")); m_select->setObjectName("rhythmSelect"); m_select->setEnabled(false);
    m_select->setToolTip(QString::fromUtf8("停止本次演奏并返回选曲，不保存未完成的成绩。"));
    controls->addWidget(m_song,2); controls->addWidget(m_difficulty,1); controls->addWidget(m_start); controls->addWidget(m_pause); controls->addWidget(m_select);
    layout->addLayout(controls);
    auto* tuning = new QHBoxLayout;
    tuning->addWidget(new QLabel(QString::fromUtf8("D F J K · 空格暂停"))); tuning->addStretch();
    tuning->addWidget(new QLabel(QString::fromUtf8("流速")));
    m_speed = new QComboBox; m_speed->setObjectName("rhythmSpeed");
    for(double speed : {.8,1.,1.25,1.5,1.8}) m_speed->addItem(QString::number(speed,'f',2)+"×",speed);
    m_speed->setCurrentIndex(qBound(0,QSettings("MyGO-Mota","MyGO-Mota").value("rhythmSpeed",1).toInt(),4));
    m_speed->setFocusPolicy(Qt::NoFocus); tuning->addWidget(m_speed); tuning->addSpacing(12);
    tuning->addWidget(new QLabel(QString::fromUtf8("音量")));
    auto* volume = new QSlider(Qt::Horizontal); volume->setRange(0,100); volume->setFixedWidth(110);
    QSettings settings("MyGO-Mota", "MyGO-Mota"); volume->setValue(settings.value("rhythmVolume",60).toInt());
    tuning->addWidget(volume); tuning->addSpacing(15); tuning->addWidget(new QLabel(QString::fromUtf8("判定偏移")));
    m_offset = new QSpinBox; m_offset->setRange(-300,300); m_offset->setSingleStep(5); m_offset->setSuffix(" ms");
    m_offset->setValue(settings.value("rhythmOffset",0).toInt());
    m_offset->setToolTip(QString::fromUtf8("正数：允许按键更晚到达。用于补偿声卡或蓝牙耳机延迟。"));
    tuning->addWidget(m_offset); layout->addLayout(tuning); layout->addStretch();
    for (auto* button : findChildren<QPushButton*>()) button->setFocusPolicy(Qt::NoFocus);
    m_song->setFocusPolicy(Qt::NoFocus); m_difficulty->setFocusPolicy(Qt::NoFocus); volume->setFocusPolicy(Qt::NoFocus);
    m_audio = new QAudioOutput(this); m_audio->setVolume(volume->value()/100.f);
    m_player = new QMediaPlayer(this); m_player->setAudioOutput(m_audio);
    connect(volume,&QSlider::valueChanged,this,[this](int value){ m_audio->setVolume(value/100.f); QSettings("MyGO-Mota","MyGO-Mota").setValue("rhythmVolume",value); });
    connect(m_offset,&QSpinBox::valueChanged,this,[](int value){ QSettings("MyGO-Mota","MyGO-Mota").setValue("rhythmOffset",value); });
    connect(m_speed,&QComboBox::currentIndexChanged,this,[this](int value){QSettings("MyGO-Mota","MyGO-Mota").setValue("rhythmSpeed",value);setFocus();});
    connect(back,&QPushButton::clicked,this,[this]{ m_player->stop(); emit returnToMenu(); });
    connect(m_start,&QPushButton::clicked,this,&RhythmGamePage::start);
    connect(m_pause,&QPushButton::clicked,this,&RhythmGamePage::pause);
    connect(m_select,&QPushButton::clicked,this,&RhythmGamePage::selectSong);
    connect(m_player,&QMediaPlayer::positionChanged,this,[this](qint64 pos){ m_position=pos; m_clock.restart(); });
    connect(m_player,&QMediaPlayer::playbackStateChanged,this,[this](QMediaPlayer::PlaybackState state){
        if (state == QMediaPlayer::PlayingState && m_mode == Loading) { m_mode=Playing; m_position=m_player->position(); m_clock.restart(); }
    });
    connect(m_player,&QMediaPlayer::mediaStatusChanged,this,[this](QMediaPlayer::MediaStatus status){
        if (status == QMediaPlayer::EndOfMedia && (m_mode==Playing || m_mode==Loading)) finish();
    });
    connect(m_player,&QMediaPlayer::errorOccurred,this,[this](QMediaPlayer::Error,const QString& error){
        m_error=QString::fromUtf8("音频播放失败：")+error; m_mode=Ready;
        m_song->setEnabled(true); m_difficulty->setEnabled(true); m_offset->setEnabled(true); m_pause->setEnabled(false); m_select->setEnabled(true); update();
    });
    auto* timer = new QTimer(this); timer->setTimerType(Qt::PreciseTimer); timer->setInterval(16);
    connect(timer,&QTimer::timeout,this,&RhythmGamePage::tick); timer->start();
    m_start->setEnabled(!m_tracks.isEmpty());
}

QString RhythmGamePage::recordKey() const { return "rhythmBest/"+m_tracks[m_song->currentIndex()].toObject()["id"].toString()+"/"+QString::number(m_difficulty->currentIndex()); }
QRectF RhythmGamePage::board() const { double w=qMin(540.,width()*.46); return {(width()-w)/2.,174.,w,qMax(260.,height()-220.)}; }
int RhythmGamePage::songTime() const {
    return int(m_position)+(m_mode==Playing && m_clock.isValid() ? int(qMin(qint64(250),m_clock.elapsed())) : 0)-m_offset->value();
}
void RhythmGamePage::start() {
    if (m_tracks.isEmpty()) return;
    m_mode=Ready; m_player->stop(); m_state={}; m_error.clear(); m_feedback.clear(); m_timingFeedback.clear();
    std::fill(std::begin(m_pressed),std::end(m_pressed),false);
    const auto track=m_tracks[m_song->currentIndex()].toObject();
    m_duration=track["duration"].toInt();
    std::vector<RhythmNote> source;
    for(const auto& v:track["notes"].toArray()) {const auto n=v.toArray();source.push_back({n[0].toInt(),n[1].toInt()});}
    m_state.notes=makeRhythmChart(source,m_difficulty->currentIndex());
    QSettings settings("MyGO-Mota","MyGO-Mota"); m_best=settings.value(recordKey(),0).toInt();
    m_position=0; m_clock.invalidate(); m_mode=Countdown; m_countdownDuration=2000; m_countdown.start();
    m_song->setEnabled(false); m_difficulty->setEnabled(false); m_pause->setEnabled(true);
    m_select->setEnabled(true); m_offset->setEnabled(false);
    m_start->setText(QString::fromUtf8("重新开始")); m_pause->setText(QString::fromUtf8("暂停 / 空格"));
    GameAudio::stopMiniGameMusic();
    m_player->setSource(QUrl::fromLocalFile(m_audioDirectory+track["id"].toString()+".mp3")); setFocus(); update();
}
void RhythmGamePage::pause() {
    if(m_mode==Playing || m_mode==Countdown || m_mode==Loading) { m_position=m_player->position(); m_mode=Paused; m_player->pause(); m_pause->setText(QString::fromUtf8("继续 / 空格")); m_offset->setEnabled(true); }
    else if(m_mode==Paused) { m_mode=Countdown; m_countdownDuration=1000; m_countdown.restart(); m_pause->setText(QString::fromUtf8("暂停 / 空格")); m_offset->setEnabled(false); }
    std::fill(std::begin(m_pressed),std::end(m_pressed),false); setFocus(); update();
}
void RhythmGamePage::selectSong() {
    m_mode=Ready; m_player->stop(); m_state={}; m_position=0; m_clock.invalidate();
    m_feedback.clear(); m_timingFeedback.clear(); m_error.clear();
    std::fill(std::begin(m_pressed),std::end(m_pressed),false);
    std::fill(std::begin(m_flash),std::end(m_flash),0);
    m_song->setEnabled(true); m_difficulty->setEnabled(true); m_offset->setEnabled(true);
    m_select->setEnabled(false); m_pause->setEnabled(false); m_start->setText(QString::fromUtf8("开始演奏")); setFocus(); update();
}
void RhythmGamePage::finish() {
    m_state.advance(m_duration+1000); m_mode=Finished; m_player->stop();
    m_pause->setEnabled(false); m_song->setEnabled(true); m_difficulty->setEnabled(true); m_offset->setEnabled(true);
    QSettings settings("MyGO-Mota","MyGO-Mota");
    m_best=std::max(m_best,m_state.score()); settings.setValue(recordKey(),m_best); update();
}
void RhythmGamePage::tick() {
    for(int& f:m_flash) f=qMax(0,f-16);
    if(m_mode==Countdown && m_countdown.elapsed()>=m_countdownDuration) {m_mode=Loading; m_player->play();}
    if(m_mode==Playing) {
        int misses=m_state.miss; m_state.advance(songTime());
        if(m_state.miss>misses) { m_feedback="MISS"; m_timingFeedback.clear(); m_feedbackClock.restart(); }
    }
    update();
}
void RhythmGamePage::strike(int lane) {
    if(m_mode!=Playing || lane<0 || lane>3) return;
    m_flash[lane]=180;
    int grade=m_state.hit(lane,songTime());
    if(grade) {
        m_feedback=grade==3 ? "PERFECT" : grade==2 ? "GREAT" : "GOOD";
        m_timingFeedback=std::abs(m_state.lastError)<16 ? "ON TIME" : QString("%1 %2 ms").arg(m_state.lastError<0?"EARLY":"LATE").arg(std::abs(m_state.lastError));
        m_feedbackClock.restart();
    }
    update();
}
void RhythmGamePage::keyPressEvent(QKeyEvent* e) {
    int lane=laneForKey(e->key());
    if(lane>=0) { if(!e->isAutoRepeat() && !m_pressed[lane]) {m_pressed[lane]=true; strike(lane);} e->accept(); return; }
    if(e->key()==Qt::Key_Space || e->key()==Qt::Key_Escape) { if(!e->isAutoRepeat()) pause(); e->accept(); return; }
    if((e->key()==Qt::Key_Return || e->key()==Qt::Key_Enter) && (m_mode==Ready || m_mode==Finished)) { if(!e->isAutoRepeat())start();e->accept();return; }
    QWidget::keyPressEvent(e);
}
void RhythmGamePage::keyReleaseEvent(QKeyEvent* e) {
    int lane=laneForKey(e->key()); if(lane>=0) {if(!e->isAutoRepeat())m_pressed[lane]=false; e->accept();return;} QWidget::keyReleaseEvent(e);
}
void RhythmGamePage::mousePressEvent(QMouseEvent* e) {
    setFocus(); const auto b=board(); if(e->button()==Qt::LeftButton && b.contains(e->position())) strike(qBound(0,int((e->position().x()-b.x())/(b.width()/4)),3));
}
bool RhythmGamePage::event(QEvent* e) {
    if(e->type()==QEvent::WindowDeactivate && (m_mode==Playing || m_mode==Countdown || m_mode==Loading)) pause();
    return QWidget::event(e);
}
void RhythmGamePage::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e); m_scaledBackground=m_background.scaled(size(),Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation);
    const auto b=board();
    m_scaledAnon=m_anon.scaled(qMax(1,int(b.left())-45),qMax(1,height()-300),Qt::KeepAspectRatio,Qt::SmoothTransformation);
    m_scaledSoyo=m_soyo.scaled(qMax(1,width()-int(b.right())-45),qMax(1,height()-300),Qt::KeepAspectRatio,Qt::SmoothTransformation);
}
void RhythmGamePage::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.fillRect(rect(),QColor("#100e1c"));
    if(!m_scaledBackground.isNull()) { p.setOpacity(.28); p.drawPixmap((width()-m_scaledBackground.width())/2,(height()-m_scaledBackground.height())/2,m_scaledBackground); p.setOpacity(1); }
    p.fillRect(rect(),QColor(13,10,24,100));
    const QRectF b=board(); const double laneW=b.width()/4, hitY=b.bottom()-66, travel=hitY-b.top()-12;
    auto portrait=[&](const QPixmap& image,QRect area){ if(image.isNull())return; p.setOpacity(.7); p.drawPixmap(area.center().x()-image.width()/2,area.bottom()-image.height(),image);p.setOpacity(1); };
    portrait(m_scaledAnon,QRect(20,275,int(b.left())-45,height()-300));
    portrait(m_scaledSoyo,QRect(int(b.right())+25,275,width()-int(b.right())-45,height()-300));
    p.setPen(QPen(QColor("#8e748f"),1)); p.setBrush(QColor(14,12,28,235)); p.drawRoundedRect(b,14,14);
    for(int lane=0;lane<4;++lane) {
        const double x=b.left()+lane*laneW;
        if(m_pressed[lane] || m_flash[lane]>0) { QLinearGradient glow(x,b.top(),x,hitY); auto c=colors[lane];c.setAlpha(0);glow.setColorAt(0,c);c.setAlpha(95);glow.setColorAt(1,c);p.fillRect(QRectF(x+1,b.top()+5,laneW-2,b.height()-10),glow); }
        p.setPen(QColor(220,190,230,30));p.drawLine(QPointF(x,b.top()+10),QPointF(x,b.bottom()-10));
        p.setBrush(colors[lane]);p.setPen(Qt::NoPen);p.drawRoundedRect(QRectF(x+9,hitY-3,laneW-18,6),3,3);
        p.setPen(colors[lane]); QFont keyFont("Segoe UI",19,QFont::Bold);p.setFont(keyFont);
        p.drawText(QRectF(x,hitY+18,laneW,38),Qt::AlignCenter,QString("DFJK").mid(lane,1));
        if(m_flash[lane]>0) {
            const double phase=1.-m_flash[lane]/180.; auto c=colors[lane];c.setAlpha(int(200*(1-phase)));
            p.setPen(QPen(c,2));p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPointF(x+laneW/2,hitY),12+phase*38,5+phase*19);
        }
    }
    const int time=songTime(); const int approach=int(2000/m_speed->currentData().toDouble());
    p.save();p.setClipRect(b.adjusted(3,3,-3,-4));
    for(const auto& note:m_state.notes) {
        if(note.result || note.time-time>approach || note.time-time < -150) continue;
        double y=hitY-(note.time-time)*travel/approach;
        QRectF tile(b.left()+note.lane*laneW+10,y-9,laneW-20,18);
        auto c=colors[note.lane];p.setPen(QPen(c.lighter(150),1.5));p.setBrush(c);p.drawRoundedRect(tile,5,5);
        p.setPen(QColor(255,255,255,180));p.drawLine(tile.topLeft()+QPointF(7,4),tile.topRight()+QPointF(-7,4));
    }
    p.restore();
    auto text=[&](QRectF r,const QString& s,int size,QColor color=QColor("#f7eaf1")) {p.setFont(QFont("Microsoft YaHei",size,QFont::DemiBold));p.setPen(color);p.drawText(r,Qt::AlignCenter,s);};
    const double side=qMax(190.,b.left()-30);
    text(QRectF(15,185,side,28),"SCORE",12,colors[0]);
    text(QRectF(15,215,side,42),QString::number(m_state.score()).rightJustified(7,'0'),27);
    text(QRectF(b.right()+15,185,side,28),"ACCURACY",12,colors[1]);
    text(QRectF(b.right()+15,215,side,42),QString::number(m_state.accuracy(),'f',2)+"%",25);
    if(m_mode==Playing) {
        if(m_state.combo>1) { text(QRectF(b.x(),b.y()+65,b.width(),64),QString::number(m_state.combo),40);text(QRectF(b.x(),b.y()+122,b.width(),28),"COMBO",12,colors[2]); }
        if(m_feedbackClock.isValid() && m_feedbackClock.elapsed()<650) {
            text(QRectF(b.x(),hitY-125,b.width(),42),m_feedback,24,m_feedback=="MISS"?QColor("#ff8399"):QColor("#ffdf9e"));
            text(QRectF(b.x(),hitY-82,b.width(),24),m_timingFeedback,11,colors[1]);
        }
        double progress=qBound(0.,double(m_position)/m_duration,1.);p.fillRect(QRectF(b.x(),height()-30,b.width()*progress,3),colors[0]);
        text(QRectF(b.x(),height()-26,b.width(),22),QString::fromUtf8("%1 / %2 秒  ·  主歌／副歌节选 · 初版谱面").arg(m_position/1000).arg(m_duration/1000),10);
    } else {
        p.setBrush(QColor(18,14,32,235));p.setPen(QColor("#af8aa8"));p.drawRoundedRect(b.adjusted(20,45,-20,-105),12,12);
        const auto panel=b.adjusted(26,65,-26,-105);
        if(m_mode==Countdown) {
            text(QRectF(panel.x(),panel.y()+40,panel.width(),65),QString::fromUtf8("准备演奏"),24,colors[0]);
            text(QRectF(panel.x(),panel.y()+112,panel.width(),100),QString::number(qMax(1,int((m_countdownDuration-m_countdown.elapsed()+999)/1000))),64,colors[3]);
            text(QRectF(panel.x(),panel.y()+226,panel.width(),45),"D     F     J     K",22,colors[1]);
        } else if(m_mode==Finished) {
            QString rank=m_state.accuracy()>=95?"S":m_state.accuracy()>=85?"A":m_state.accuracy()>=70?"B":"C";
            text(QRectF(panel.x(),panel.y(),panel.width(),75),rank,48,colors[3]);
            text(QRectF(panel.x(),panel.y()+78,panel.width(),45),QString::fromUtf8("演奏完成  ·  %1").arg(m_state.miss==0?"FULL COMBO":"LIVE CLEAR"),18);
            text(QRectF(panel.x(),panel.y()+135,panel.width(),130),QString("PERFECT %1     GREAT %2\nGOOD %3     MISS %4\nMAX COMBO %5\nBEST %6").arg(m_state.perfect).arg(m_state.great).arg(m_state.good).arg(m_state.miss).arg(m_state.maxCombo).arg(m_best),15);
            const double total=qMax(1,int(m_state.notes.size())); double x=panel.x()+12;
            const int counts[]={m_state.perfect,m_state.great,m_state.good,m_state.miss};
            const QColor bars[]={QColor("#f3cf91"),QColor("#91caff"),QColor("#b6a0f8"),QColor("#ef8194")};
            for(int i=0;i<4;++i){double w=(panel.width()-24)*counts[i]/total;p.fillRect(QRectF(x,panel.y()+281,w,8),bars[i]);x+=w;}
        } else {
            text(QRectF(panel.x(),panel.y()+25,panel.width(),60),m_mode==Paused?QString::fromUtf8("演奏已暂停"):m_mode==Loading?QString::fromUtf8("正在加载音乐…"):QString::fromUtf8("与她们，一起合奏"),24,colors[0]);
            text(QRectF(panel.x(),panel.y()+100,panel.width(),150),m_mode==Paused?QString::fromUtf8("空格继续\n\n切回窗口后不会自动开始"):QString::fromUtf8("选择歌曲与难度，点击开始演奏\n\n音符落到判定线时按 D / F / J / K\n空格暂停 · 点击轨道也可操作\n\n建议使用有线耳机"),14);
        }
        if(!m_error.isEmpty()) text(QRectF(panel.x(),panel.bottom()-60,panel.width(),60),m_error,11,QColor("#ffa0a0"));
    }
}
