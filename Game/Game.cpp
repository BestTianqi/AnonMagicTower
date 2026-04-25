#include "Game.h"

Game::Game()
    : m_width(10), m_height(8)
{
}

bool Game::loadDefaultMap()
{
    m_map.assign(m_width * m_height, Tile_Floor);
    // create border walls
    for (int x = 0; x < m_width; ++x) {
        m_map[x] = Tile_Wall;
        m_map[(m_height-1)*m_width + x] = Tile_Wall;
    }
    for (int y = 0; y < m_height; ++y) {
        m_map[y*m_width] = Tile_Wall;
        m_map[y*m_width + (m_width-1)] = Tile_Wall;
    }

    // place some example tiles
    m_map[1*m_width + 1] = Tile_StairsUp;
    m_map[2*m_width + 3] = Tile_Monster;
    m_map[4*m_width + 5] = Tile_Item;

    // place player in the map
    m_player.x = 1;
    m_player.y = 2;
    m_player.hp = 100;
    m_player.atk = 10;

    return true;
}
