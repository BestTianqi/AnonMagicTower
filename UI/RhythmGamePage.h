#pragma once
#include <QWidget>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QPixmap>
#include "Game/RhythmState.h"
class QMediaPlayer;
class QAudioOutput;
class QComboBox;
class QPushButton;
class QSpinBox;
class RhythmGamePage : public QWidget {
    Q_OBJECT
public:
    explicit RhythmGamePage(QWidget* parent = nullptr);
signals:
    void returnToMenu();
protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    bool event(QEvent*) override;
private:
    enum Mode { Ready, Loading, Countdown, Playing, Paused, Finished } m_mode = Ready;
    void start();
    void pause();
    void finish();
    void selectSong();
    void tick();
    void strike(int lane);
    int songTime() const;
    QRectF board() const;
    QString recordKey() const;
    RhythmState m_state;
    QJsonArray m_tracks;
    QMediaPlayer* m_player;
    QAudioOutput* m_audio;
    QComboBox *m_song, *m_difficulty, *m_speed;
    QSpinBox* m_offset;
    QPushButton *m_start, *m_pause, *m_select;
    QPixmap m_background, m_scaledBackground, m_anon, m_soyo;
    QPixmap m_scaledAnon, m_scaledSoyo;
    QElapsedTimer m_clock, m_feedbackClock;
    QElapsedTimer m_countdown;
    int m_countdownDuration = 2000;
    qint64 m_position = 0;
    int m_duration = 1, m_best = 0;
    QString m_feedback, m_timingFeedback, m_error, m_audioDirectory;
    bool m_pressed[4] = {};
    int m_flash[4] = {};
};
