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
    void setMovementAnimationEnabled(bool enabled) { m_movementAnimationEnabled = enabled; }
    bool movementAnimationEnabled() const { return m_movementAnimationEnabled; }
    void snapPlayerToGame();

    void loadTileImage(int tileType, const QString& path);
    void loadItemImage(const std::string& name, const QString& path);
    void loadNPCImage(const std::string& name, const QString& path);
    void loadDarkWallRevealedImage(const QString& path);
    void loadMonsterImage(const std::string& name, const QString& path);
    void loadPlayerImage(const QString& path);
    void loadPlayerSpriteSheet(const QString& path);
    // 注册并切换主角服装；每套可为 4×4 旧表或 8×8 精细表。
    void loadPlayerOutfitSpriteSheet(const QString& outfitId, const QString& path);
    bool setPlayerOutfit(const QString& outfitId);
    QString playerOutfit() const;
    void setPlayerDirection(int dx, int dy);
    // 播放鼠标点击的逐格路径；游戏状态仍由 Game 保持最终格坐标。
    void playPlayerPath(const std::vector<std::pair<int, int>>& path);
    bool isPlayerMoving() const { return m_motionInitialized && m_playerMotion.isMoving(); }
    void playMonsterMovement(const std::vector<Game::MonsterMovementAnimation>& movements);
    bool isMonsterMoving() const { return m_monsterMotionActive; }
    bool isSceneAnimating() const {
        return isPlayerMoving() || !m_scriptedPlayerPath.empty() || isMonsterMoving();
    }
    void loadBackgroundImage(const QString& path);
    QSize sizeHint() const override;

signals:
    void tileClicked(int x, int y);
    void playerMotionFinished();
    // 所有逐格怪物移动完成后发出；用于把剧情对白严格排在行走动画之后。
    void monsterMotionFinished();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    void generatePlaceholders();
    void advancePlayerMotion();
    void advanceMonsterMotion();
    void syncPlayerMotionTarget();

    struct MonsterMotion {
        std::string name;
        QPointF from;
        QPointF to;
    };

    Game* m_game;

    std::unordered_map<int, QPixmap>        m_tilePix;
    std::unordered_map<std::string, QPixmap> m_monsterPix;
    std::unordered_map<std::string, QPixmap> m_itemPix;
    std::unordered_map<std::string, QPixmap> m_npcPix;
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

    std::array<QPixmap, 64> m_playerFrames;
    std::unordered_map<std::string, QString> m_playerOutfitPaths;
    std::string m_activePlayerOutfit;
    bool m_hasPlayerSheet = false;
    int m_playerSheetColumns = 4;
    int m_playerSheetRows = 4;
    int m_playerDirectionRow = 0; // down, left, right, up
    int m_playerFrame = 1;
    int m_lastPlayerTileX = 0;
    int m_lastPlayerTileY = 0;
    bool m_motionInitialized = false;
    PlayerMotionState m_playerMotion;
    QTimer m_motionTimer;
    QElapsedTimer m_motionClock;
    QElapsedTimer m_monsterMotionClock;
    std::vector<MonsterMotion> m_monsterMotions;
    size_t m_monsterMotionIndex = 0;
    bool m_monsterMotionActive = false;
    bool m_movementAnimationEnabled = true;
    std::vector<std::pair<int, int>> m_scriptedPlayerPath;
    size_t m_scriptedPlayerPathIndex = 0;
};
