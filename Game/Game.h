#pragma once

#include <vector>
#include "Entities/Player.h"

enum TileType { Tile_Empty = 0, Tile_Wall, Tile_Floor, Tile_StairsUp, Tile_StairsDown, Tile_Monster, Tile_Item };

class Game {
public:
    Game();
    bool loadDefaultMap();

    const std::vector<int>& map() const { return m_map; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    Player& player() { return m_player; }

private:
    std::vector<int> m_map;
    int m_width;
    int m_height;
    Player m_player;
};
