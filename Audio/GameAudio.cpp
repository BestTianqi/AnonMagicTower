#include "GameAudio.h"
#include "StoryVoice.h"

#include <QApplication>
#include <QAudioOutput>
#include <QDir>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QSettings>
#include <QSoundEffect>
#include <QUrl>

namespace {
const char* filename(GameAudio::Cue cue) {
    switch (cue) {
    case GameAudio::Cue::MenuSelect: return "menu_select";
    case GameAudio::Cue::Step: return "step";
    case GameAudio::Cue::Door: return "door";
    case GameAudio::Cue::Pickup: return "pickup";
    case GameAudio::Cue::Battle: return "battle";
    case GameAudio::Cue::Victory: return "victory";
    case GameAudio::Cue::Blocked: return "blocked";
    case GameAudio::Cue::Stairs: return "stairs";
    case GameAudio::Cue::Dialogue: return "dialogue";
    case GameAudio::Cue::Shop: return "shop";
    case GameAudio::Cue::PuzzleSlide: return "puzzle_slide";
    case GameAudio::Cue::Undo: return "undo";
    case GameAudio::Cue::PuzzleSolved: return "puzzle_solved";
    case GameAudio::Cue::MergeMove: return "merge_move";
    case GameAudio::Cue::MergeSmall: return "merge_small";
    case GameAudio::Cue::MergeLarge: return "merge_large";
    case GameAudio::Cue::MergeGoal: return "merge_goal";
    }
    return "menu_select";
}

QString musicSource(int track) {
    return track == 1
        ? QStringLiteral("qrc:/Audio/music/killkiss_8bit.wav")
        : QStringLiteral("qrc:/Audio/music/haruhikage_8bit.wav");
}
}

GameAudio::GameAudio(QObject* parent) : QObject(parent) {
    applySettings();
    for (int index = int(Cue::MenuSelect); index <= int(Cue::MergeGoal); ++index) {
        const auto cue = Cue(index);
        auto* effect = new QSoundEffect(this);
        effect->setSource(QUrl(QStringLiteral("qrc:/Audio/sfx/%1.wav")
            .arg(QString::fromLatin1(filename(cue)))));
        effect->setVolume(m_enabled ? m_volume / 100.0f : 0.0f);
        m_effects.insert(index, effect);
    }
    m_musicOutput = new QAudioOutput(this);
    m_musicPlayer = new QMediaPlayer(this);
    m_musicPlayer->setAudioOutput(m_musicOutput);
    m_musicPlayer->setSource(QUrl(musicSource(m_musicTrack)));
    m_musicPlayer->setLoops(QMediaPlayer::Infinite);
    m_musicOutput->setVolume(m_musicEnabled ? m_musicVolume / 100.0f : 0.0f);
    m_voiceOutput = new QAudioOutput(this);
    m_voicePlayer = new QMediaPlayer(this);
    m_voicePlayer->setAudioOutput(m_voiceOutput);
    m_voiceOutput->setVolume(m_enabled ? m_volume / 100.0f : 0.0f);
}

GameAudio& GameAudio::instance() {
    static auto* audio = new GameAudio(qApp);
    return *audio;
}

void GameAudio::prepare() { instance(); }

void GameAudio::play(Cue cue) { instance().playCue(cue); }

void GameAudio::refreshSettings() { instance().applySettings(); }

void GameAudio::startMiniGameMusic() { instance().startMusic(); }

void GameAudio::stopMiniGameMusic() { instance().stopMusic(); }
void GameAudio::pauseMiniGameMusic() {
    auto& audio=instance();audio.m_musicRequested=false;audio.m_musicPlayer->pause();
}

bool GameAudio::playStoryVoice(const QString& speaker, const QString& text) {
    auto& audio = instance();
    audio.m_voicePlayer->stop();
    if (!audio.m_enabled || audio.m_volume == 0 || storyVoiceModel(speaker).isEmpty()) return false;
    const QString file = QDir(qApp->applicationDirPath()).filePath(
        QStringLiteral("Audio/voice/") + storyVoiceFilename(speaker, text));
    if (!QFileInfo::exists(file)) return false;
    audio.m_voicePlayer->setSource(QUrl::fromLocalFile(file));
    audio.m_voicePlayer->play();
    return true;
}

void GameAudio::stopStoryVoice() { instance().m_voicePlayer->stop(); }

int GameAudio::miniGameMusicTrack() {
    const QSettings settings(QSettings::defaultFormat(), QSettings::UserScope,
                             QStringLiteral("MyGO-Mota"), QStringLiteral("MyGO-Mota"));
    return qBound(0, settings.value(QStringLiteral("backgroundMusicTrack"), 0).toInt(), 1);
}

void GameAudio::setMiniGameMusicTrack(int track) {
    QSettings settings(QSettings::defaultFormat(), QSettings::UserScope,
                       QStringLiteral("MyGO-Mota"), QStringLiteral("MyGO-Mota"));
    settings.setValue(QStringLiteral("backgroundMusicTrack"), qBound(0, track, 1));
    settings.sync();
    instance().applySettings();
}

void GameAudio::applySettings() {
    const QSettings settings(QSettings::defaultFormat(), QSettings::UserScope,
                             QStringLiteral("MyGO-Mota"), QStringLiteral("MyGO-Mota"));
    m_enabled = settings.value(QStringLiteral("soundEffectsEnabled"), true).toBool();
    m_volume = qBound(0, settings.value(QStringLiteral("soundEffectsVolume"), 65).toInt(), 100);
    m_musicEnabled = settings.value(QStringLiteral("backgroundMusicEnabled"), true).toBool();
    m_musicVolume = qBound(0, settings.value(QStringLiteral("backgroundMusicVolume"), 38).toInt(), 100);
    const int selectedTrack = qBound(0, settings.value(QStringLiteral("backgroundMusicTrack"), 0).toInt(), 1);
    if (m_musicTrack != selectedTrack) {
        m_musicTrack = selectedTrack;
        if (m_musicPlayer) {
            m_musicPlayer->stop();
            m_musicPlayer->setSource(QUrl(musicSource(m_musicTrack)));
            m_musicPlayer->setLoops(QMediaPlayer::Infinite);
        }
    }
    for (auto* effect : m_effects)
        effect->setVolume(m_enabled ? m_volume / 100.0f : 0.0f);
    if (m_musicOutput)
        m_musicOutput->setVolume(m_musicEnabled ? m_musicVolume / 100.0f : 0.0f);
    if (m_voiceOutput) {
        m_voiceOutput->setVolume(m_enabled ? m_volume / 100.0f : 0.0f);
        if (!m_enabled || m_volume == 0) m_voicePlayer->stop();
    }
    if (m_musicPlayer && m_musicRequested) {
        if (m_musicEnabled && m_musicVolume > 0)
            m_musicPlayer->play();
        else
            m_musicPlayer->pause();
    }
}

void GameAudio::startMusic() {
    m_musicRequested = true;
    if (m_musicEnabled && m_musicVolume > 0)
        m_musicPlayer->play();
}

void GameAudio::stopMusic() {
    m_musicRequested = false;
    m_musicPlayer->stop();
}

void GameAudio::playCue(Cue cue) {
    if (!m_enabled || m_volume == 0) return;
    if (auto* effect = m_effects.value(int(cue), nullptr)) {
        effect->stop();
        effect->play();
    }
}
