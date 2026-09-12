#pragma once

#include <QWidget>
#include <QMouseEvent>
#include <QPixmap>
#include <QTimer>
#include <QElapsedTimer>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include "Game/Game.h"
#include "PlayerMotion.h"

constexpr int TILE_SIZE = 60;

class MapWidget : public QWidget {
    Q_OBJECT
public:
    explicit MapWidget(Game* game, QWidget* parent = nullptr);
    void setGame(Game* game) { m_game = game; update(); }

    void loadTileImage(int tileType, const QString& path);
    void loadItemImage(const std::string& name, const QString& path);
    void loadDarkWallRevealedImage(const QString& path);
    void loadMonsterImage(const std::string& name, const QString& path);
    void loadPlayerImage(const QString& path);
    void loadPlayerSpriteSheet(const QString& path);
    void setPlayerDirection(int dx, int dy);
    bool isPlayerMoving() const { return m_motionInitialized && m_playerMotion.isMoving(); }
    void loadBackgroundImage(const QString& path);
    QSize sizeHint() const override;

signals:
    void tileClicked(int x, int y);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    void generatePlaceholders();
    void advancePlayerMotion();
    void syncPlayerMotionTarget();

    Game* m_game;

    std::unordered_map<int, QPixmap>        m_tilePix;
    std::unordered_map<std::string, QPixmap> m_monsterPix;
    std::unordered_map<std::string, QPixmap> m_itemPix;
    std::unordered_set<int>                  m_hasTileImage;    // 记录哪些 tile 类型有真实图片
    std::unordered_set<std::string>          m_hasMonsterImage; // 记录哪些怪物有真实图片
    bool m_hasPlayerImage = false;
    QPixmap m_playerPix;
    QPixmap m_defaultMonsterPix;
    QPixmap m_defaultItemPix;
    QPixmap m_darkWallRevealed;
    QPixmap m_backgroundPix;
    QPixmap m_backgroundScaled;
    QSize m_backgroundViewport;

    std::array<QPixmap, 16> m_playerFrames;
    bool m_hasPlayerSheet = false;
    int m_playerDirectionRow = 0; // down, left, right, up
    int m_playerFrame = 1;
    int m_lastPlayerTileX = 0;
    int m_lastPlayerTileY = 0;
    bool m_motionInitialized = false;
    PlayerMotionState m_playerMotion;
    QTimer m_motionTimer;
    QElapsedTimer m_motionClock;
};
