#pragma once
#include "Audio/GameAudio.h"
#include <QWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QElapsedTimer>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QSignalBlocker>
#include <QDateTime>
#include <functional>

// Shared in-window pause/save bar. Each game owns its payload and undo history.
class MiniGameSession : public QWidget {
public:
    using Capture=std::function<QJsonObject()>;
    using Restore=std::function<bool(const QJsonObject&)>;
    MiniGameSession(QString game, Capture capture, Restore restore,
                    std::function<void(bool)> pauseChanged, QWidget* parent)
        : QWidget(parent), m_game(std::move(game)), m_capture(std::move(capture)),
          m_restore(std::move(restore)), m_pauseChanged(std::move(pauseChanged)) {
        setObjectName(m_game+"Session");
        auto* row=new QHBoxLayout(this);row->setContentsMargins(0,0,0,0);row->setSpacing(8);
        m_pause=new QPushButton(QString::fromUtf8("暂停 · 空格"),this);m_pause->setObjectName(m_game+"Pause");
        m_pause->setCheckable(true);row->addWidget(m_pause);
        m_slot=new QComboBox(this);m_slot->setObjectName(m_game+"SaveSlot");
        m_slot->addItems({QString::fromUtf8("存档 1"),QString::fromUtf8("存档 2"),QString::fromUtf8("存档 3")});row->addWidget(m_slot);
        auto* save=new QPushButton(QString::fromUtf8("保存 · Ctrl+S"),this);save->setObjectName(m_game+"Save");row->addWidget(save);
        auto* load=new QPushButton(QString::fromUtf8("读取 · Ctrl+L"),this);load->setObjectName(m_game+"Load");row->addWidget(load);
        m_notice=new QLabel(QString::fromUtf8("自动保存每一步 · 存档独立于魔塔"),this);
        m_notice->setObjectName(m_game+"SaveNotice");m_notice->setStyleSheet("color:#cbb8d1;font-size:12px;");row->addWidget(m_notice,1);
        for(auto* button:findChildren<QPushButton*>())button->setFocusPolicy(Qt::NoFocus);
        connect(m_pause,&QPushButton::toggled,this,[this](bool paused){setPaused(paused);});
        connect(save,&QPushButton::clicked,this,[this]{saveSlot();});
        connect(load,&QPushButton::clicked,this,[this]{loadSlot();});
        m_debounce.setSingleShot(true);m_debounce.setInterval(250);
        connect(&m_debounce,&QTimer::timeout,this,[this]{saveAuto();});
        updateSlots();
    }
    static QString storageDirectory() {
        QSettings settings(QSettings::defaultFormat(),QSettings::UserScope,"MyGO-Mota","MyGO-Mota");
        const QString root=settings.format()==QSettings::IniFormat
            ? QFileInfo(settings.fileName()).absolutePath()
            : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        return root+"/minigames";
    }
    bool paused() const {return m_paused;}
    qint64 elapsedMs() const {return m_elapsedMs+(m_elapsed.isValid()?m_elapsed.elapsed():0);}
    void setPaused(bool paused) {
        if(m_paused==paused)return;
        settleClock();m_paused=paused;
        {QSignalBlocker blocker(m_pause);m_pause->setChecked(paused);}
        m_pause->setText(paused?QString::fromUtf8("继续 · 空格"):QString::fromUtf8("暂停 · 空格"));
        if(paused)GameAudio::pauseMiniGameMusic();else GameAudio::startMiniGameMusic();
        m_pauseChanged(paused);resumeClock();
        if(!m_loading && m_hasGame)m_debounce.start();
    }
    void togglePause(){setPaused(!m_paused);}
    void resetProgress(){settleClock();m_elapsedMs=0;m_started=false;m_finished=false;m_hasGame=true;setPaused(false);}
    void changed(bool started,bool finished) {
        if(m_loading)return;
        settleClock();m_started=m_started||started;m_finished=finished;m_hasGame=true;resumeClock();m_debounce.start();
    }
    void saveSlot(){save(QString::number(m_slot->currentIndex()+1));}
    bool loadSlot(){return load(QString::number(m_slot->currentIndex()+1));}
    bool restoreAuto(){return load("auto",true);}
    bool saveAuto(){m_debounce.stop();return !m_hasGame || save("auto");}
    void notify(const QString& message){m_notice->setText(message);m_notice->setToolTip(message);}
private:
    QString path(const QString& slot) const {return storageDirectory()+"/"+m_game+"-"+slot+".json";}
    void settleClock(){if(m_elapsed.isValid()){m_elapsedMs+=m_elapsed.elapsed();m_elapsed.invalidate();}}
    void resumeClock(){if(m_started && !m_paused && !m_finished)m_elapsed.start();}
    bool save(const QString& slot) {
        if(m_loading)return false;
        QJsonObject data{{"version",1},{"game",m_game},{"payload",m_capture()},
            {"elapsedMs",double(elapsedMs())},{"started",m_started},{"finished",m_finished},
            {"savedAt",QDateTime::currentDateTime().toString(Qt::ISODate)}};
        QDir dir;if(!dir.mkpath(storageDirectory())){notify(QString::fromUtf8("保存失败：无法创建存档目录"));return false;}
        QSaveFile file(path(slot));const auto bytes=QJsonDocument(data).toJson(QJsonDocument::Compact);
        if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()){
            notify(QString::fromUtf8("保存失败：%1").arg(file.errorString()));return false;
        }
        m_hasGame=true;notify((slot=="auto"?QString::fromUtf8("自动保存 "):QString::fromUtf8("存档 %1 已保存 ").arg(slot))+QTime::currentTime().toString("HH:mm:ss"));
        updateSlots();return true;
    }
    bool load(const QString& slot,bool quietMissing=false) {
        QFile file(path(slot));if(!file.exists()){if(!quietMissing)notify(QString::fromUtf8("这个存档位还是空的"));return false;}
        if(!file.open(QIODevice::ReadOnly) || file.size()>16*1024*1024){notify(QString::fromUtf8("存档读取失败，当前进度未改变"));return false;}
        QJsonParseError error;const auto data=QJsonDocument::fromJson(file.readAll(),&error).object();
        const double elapsed=data["elapsedMs"].toDouble(-1);
        if(error.error!=QJsonParseError::NoError || data["version"].toInt()!=1 || data["game"].toString()!=m_game || elapsed<0 || elapsed>31536000000.){
            notify(QString::fromUtf8("存档格式不正确，当前进度未改变"));return false;
        }
        m_loading=true;
        if(!m_restore(data["payload"].toObject())){m_loading=false;notify(QString::fromUtf8("存档内容不完整，当前进度未改变"));return false;}
        m_debounce.stop();m_elapsed.invalidate();m_elapsedMs=qint64(elapsed);
        m_started=data["started"].toBool();m_finished=data["finished"].toBool();m_hasGame=true;
        setPaused(true);m_pauseChanged(true);m_loading=false;
        notify(QString::fromUtf8("已恢复%1，点击「继续」").arg(slot=="auto"?QString::fromUtf8("上次进度"):QString::fromUtf8("存档 ")+slot));
        return true;
    }
    void updateSlots(){
        for(int i=0;i<3;++i){QFileInfo info(path(QString::number(i+1)));m_slot->setItemText(i,QString::fromUtf8("存档 %1 · %2").arg(i+1).arg(info.exists()?QString::fromUtf8("已保存"):QString::fromUtf8("空")));m_slot->setItemData(i,info.exists()?info.lastModified().toString("yyyy-MM-dd HH:mm:ss"):QString(),Qt::ToolTipRole);}
    }
    QString m_game;Capture m_capture;Restore m_restore;std::function<void(bool)> m_pauseChanged;
    QPushButton* m_pause;QComboBox* m_slot;QLabel* m_notice;
    QElapsedTimer m_elapsed;QTimer m_debounce;qint64 m_elapsedMs=0;
    bool m_paused=false,m_started=false,m_finished=false,m_loading=false,m_hasGame=false;
};
