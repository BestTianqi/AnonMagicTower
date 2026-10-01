#pragma once
#include <QWidget>
#include <QPixmap>
#include <QVariantAnimation>
#include <QJsonObject>
#include "Game/Game2048State.h"

class QLabel;
class QPushButton;
class QPainter;
class QComboBox;
class MiniGameSession;

class Game2048Board : public QWidget {
    Q_OBJECT
public:
    static constexpr int CharacterCount = 17;
    explicit Game2048Board(QWidget* parent = nullptr);
    const Game2048State& state() const { return m_state; }
    bool move(Game2048State::Direction direction);
    void restart();
    void undo();
    void setPaused(bool paused);
    QJsonObject snapshot() const;
    bool restoreSnapshot(const QJsonObject& data);
    static QString characterName(int level);
    static QPixmap characterImage(int level);
signals:
    void changed();
protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
private:
    QRectF tileRect(int index) const;
    void drawTile(QPainter& painter, int value, const QRectF& target);
    Game2048State m_state;
    std::array<QPixmap, CharacterCount> m_characters;
    QVariantAnimation m_animation;
    std::vector<Game2048State::Motion> m_motions;
    QPointF m_press;
    bool m_swiping = false;
    bool m_paused = false;
};

class Game2048Page : public QWidget {
    Q_OBJECT
public:
    explicit Game2048Page(QWidget* parent = nullptr);
    ~Game2048Page() override;
signals:
    void returnToMenu();
protected:
    void paintEvent(QPaintEvent*) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    void refresh();
    Game2048Board* m_board;
    QLabel* m_score;
    QLabel* m_bestLabel;
    QLabel* m_status;
    QPushButton* m_undo;
    QComboBox* m_musicChoice = nullptr;
    MiniGameSession* m_session = nullptr;
    int m_best = 0;
    QPixmap m_background;
};
