#pragma once

#include <QWidget>
#include <QPixmap>
#include <unordered_map>
#include "Game/Game.h"

constexpr int TILE_SIZE = 60;

class MapWidget : public QWidget {
    Q_OBJECT
public:
    explicit MapWidget(Game* game, QWidget* parent = nullptr);
    void setGame(Game* game) { m_game = game; update(); }

    void loadTileImage(int tileType, const QString& path);
    void loadMonsterImage(const std::string& name, const QString& path);
    void loadPlayerImage(const QString& path);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void generatePlaceholders();

    Game* m_game;

    std::unordered_map<int, QPixmap>        m_tilePix;
    std::unordered_map<std::string, QPixmap> m_monsterPix;
    QPixmap m_playerPix;
    QPixmap m_defaultMonsterPix;
    QPixmap m_defaultItemPix;
    QPixmap m_darkWallRevealed;
};
