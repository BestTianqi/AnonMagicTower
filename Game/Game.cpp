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
    m_map[4*m_width + 5] = Tile_Item;

    // place player in the map
    m_player.x = 1;
    m_player.y = 2;
    m_player.hp = 100;
    m_player.atk = 10;
    m_player.def = 5;

    // spawn a monster at (3,2)
    spawnMonster(3,2, Monster("Goblin", 20, 6, 1));
    m_map[2*m_width + 3] = Tile_Monster;

    return true;
}

int Game::tileAt(int x, int y) const
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height) return Tile_Empty;
    return m_map[y * m_width + x];
}

void Game::setTile(int x, int y, int tile)
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height) return;
    m_map[y * m_width + x] = tile;
}

void Game::spawnMonster(int x, int y, const Monster& m)
{
    int key = y*m_width + x;
    m_monsters.emplace(key, m);
}

bool Game::hasMonsterAt(int x, int y) const
{
    int key = y*m_width + x;
    return m_monsters.find(key) != m_monsters.end();
}

Monster* Game::monsterAt(int x, int y)
{
    int key = y*m_width + x;
    auto it = m_monsters.find(key);
    if (it == m_monsters.end()) return nullptr;
    return &it->second;
}

Game::MoveResult Game::tryMovePlayer(int nx, int ny)
{
    if (nx < 0 || ny < 0 || nx >= m_width || ny >= m_height) return Move_Block;
    int tile = tileAt(nx, ny);
    switch (tile) {
    case Tile_Wall:
        return Move_Block;
    case Tile_Floor:
        m_player.x = nx; m_player.y = ny; return Move_Ok;
    case Tile_Monster:
        if (hasMonsterAt(nx, ny)) {
            // do not remove monster here; handle combat in UI
            return Move_Encounter;
        } else {
            // no monster object, treat as floor
            m_player.x = nx; m_player.y = ny; return Move_Ok;
        }
    case Tile_Item:
        // pick up a key for demo
        m_player.AddKey(KeyType::Red, 1);
        setTile(nx, ny, Tile_Floor);
        m_player.x = nx; m_player.y = ny;
        return Move_Pickup;
    case Tile_StairsUp:
        m_player.x = nx; m_player.y = ny; return Move_StairsUp;
    case Tile_StairsDown:
        m_player.x = nx; m_player.y = ny; return Move_StairsDown;
    default:
        m_player.x = nx; m_player.y = ny; return Move_Ok;
    }
}

Game::FightResult Game::fightAt(int x, int y)
{
    Monster* m = monsterAt(x, y);
    if (!m) return Fight_PlayerWin; // 没有怪物则视为胜利

    // 简易回合制：玩家先手
    while (true) {
        int dmgToMonster = m_player.atk;
        m->TakeDamage(dmgToMonster);
        if (m->IsDead()) {
            // remove monster and mark tile as floor
            int key = y*m_width + x;
            m_monsters.erase(key);
            setTile(x, y, Tile_Floor);
            return Fight_PlayerWin;
        }

        // monster attack
        int dmgToPlayer = m->Attack() - m_player.def;
        if (dmgToPlayer < 0) dmgToPlayer = 0;
        m_player.hp -= dmgToPlayer;
        if (m_player.hp <= 0) {
            return Fight_PlayerDead;
        }
    }
}
