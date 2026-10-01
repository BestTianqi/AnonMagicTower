#pragma once
#include <QWidget>
#include <QPixmap>
#include <QElapsedTimer>
#include <QTimer>
#include <QVariantAnimation>
#include <QJsonObject>
#include "Game/SlidingPuzzleState.h"

class QLabel;
class QPushButton;
class QComboBox;
class QCheckBox;
class MiniGameSession;

class SlidingPuzzleBoard : public QWidget {
    Q_OBJECT
public:
    explicit SlidingPuzzleBoard(const QPixmap& picture, QWidget* parent = nullptr);
    void newPuzzle(int size);
    void restart();
    void undo();
    void setPreview(bool on);
    void setNumbers(bool on);
    void setPaused(bool paused);
    void setPicture(const QPixmap& picture);
    QJsonObject snapshot() const;
    bool restoreSnapshot(const QJsonObject& data);
    bool moveByArrow(int key);
    const SlidingPuzzleState& state() const { return m_state; }
signals:
    void changed();
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
private:
    QRectF boardRect() const;
    QRectF tileRect(int index) const;
    int indexAt(const QPointF& position) const;
    void slide(int index);
    void drawTile(QPainter& painter, int value, const QRectF& target, bool highlight);
    QPixmap m_picture;
    SlidingPuzzleState m_state;
    SlidingPuzzleState m_initial;
    QVariantAnimation m_animation;
    int m_animatedValue = 0;
    int m_from = -1;
    int m_to = -1;
    int m_hover = -1;
    bool m_preview = false;
    bool m_numbers = true;
    bool m_paused = false;
};

class SlidingPuzzlePage : public QWidget {
    Q_OBJECT
public:
    explicit SlidingPuzzlePage(QWidget* parent = nullptr);
    ~SlidingPuzzlePage() override;
    bool importPicture(const QString& path);
signals:
    void returnToMenu();
protected:
    void paintEvent(QPaintEvent*) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    void refresh();
    void resetClock();
    void choosePicture(int index);
    QJsonObject snapshot() const;
    bool restoreSnapshot(const QJsonObject& data);
    QPixmap m_picture;
    SlidingPuzzleBoard* m_board = nullptr;
    QPixmap m_customPicture;
    QByteArray m_customImageData;
    QLabel* m_moves = nullptr;
    QLabel* m_time = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* m_undo = nullptr;
    QComboBox* m_musicChoice = nullptr;
    QComboBox* m_imageChoice = nullptr;
    QLabel* m_caption = nullptr;
    QCheckBox* m_numbers = nullptr;
    MiniGameSession* m_session = nullptr;
    QString m_pictureKey = QStringLiteral(":/images/backgrounds/anon_soyo_puzzle.jpg");
    QTimer m_clock;
    bool m_finished = false;
};
