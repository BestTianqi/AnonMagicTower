#pragma once

#include <QObject>
#include <QHash>

class QSoundEffect;
class QMediaPlayer;
class QAudioOutput;

class GameAudio final : public QObject {
public:
    enum class Cue {
        MenuSelect, Step, Door, Pickup, Battle, Victory, Blocked,
        Stairs, Dialogue, Shop, PuzzleSlide, Undo, PuzzleSolved,
        MergeMove, MergeSmall, MergeLarge, MergeGoal
    };

    static void prepare();
    static void play(Cue cue);
    static void refreshSettings();
    static void startMiniGameMusic();
    static void stopMiniGameMusic();
    static void pauseMiniGameMusic();
    static bool playStoryVoice(const QString& speaker, const QString& text);
    static void stopStoryVoice();
    static int miniGameMusicTrack();
    static void setMiniGameMusicTrack(int track);

private:
    explicit GameAudio(QObject* parent);
    static GameAudio& instance();
    void applySettings();
    void playCue(Cue cue);
    void startMusic();
    void stopMusic();

    QHash<int, QSoundEffect*> m_effects;
    bool m_enabled = true;
    int m_volume = 65;
    QMediaPlayer* m_musicPlayer = nullptr;
    QAudioOutput* m_musicOutput = nullptr;
    QMediaPlayer* m_voicePlayer = nullptr;
    QAudioOutput* m_voiceOutput = nullptr;
    bool m_musicRequested = false;
    bool m_musicEnabled = true;
    int m_musicVolume = 38;
    int m_musicTrack = 0;
};
