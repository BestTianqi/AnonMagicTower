#pragma once
#include <QWidget>
#include <QPixmap>
#include <QElapsedTimer>
#include <QTimer>
#include <QVariantAnimation>
#include "Game/SlidingPuzzleState.h"

class QLabel;
class QPushButton;

class SlidingPuzzleBoard : public QWidget {
    Q_OBJECT
public:
    explicit SlidingPuzzleBoard(const QPixmap& picture, QWidget* parent = nullptr);
    void newPuzzle(int size);
    void restart();
    void undo();
    void setPreview(bool on);
    void setNumbers(bool on);
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
};

class SlidingPuzzlePage : public QWidget {
    Q_OBJECT
public:
    explicit SlidingPuzzlePage(QWidget* parent = nullptr);
signals:
    void returnToMenu();
protected:
    void paintEvent(QPaintEvent*) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    void refresh();
    void resetClock();
    QPixmap m_picture;
    SlidingPuzzleBoard* m_board = nullptr;
    QLabel* m_moves = nullptr;
    QLabel* m_time = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* m_undo = nullptr;
    QElapsedTimer m_elapsed;
    QTimer m_clock;
    bool m_started = false;
    bool m_finished = false;
};
