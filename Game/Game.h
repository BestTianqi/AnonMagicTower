#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include "Entities/Player.h"
#include "Entities/Items.h"
#include "Entities/Monster.h"

enum TileType { Tile_Empty = 0, Tile_Wall, Tile_Floor, Tile_StairsUp, Tile_StairsDown, Tile_Monster, Tile_Item };

constexpr int MAP_SIZE = 15;

class Game {
public:
    Game();
    bool loadDefaultMap();

    const std::vector<int>& map() const { return m_map; }
    int width()  const { return m_width; }
    int height() const { return m_height; }

    Player& player() { return m_player; }

    enum MoveResult { Move_Ok, Move_Block, Move_Pickup, Move_Encounter, Move_StairsUp, Move_StairsDown, Move_PlayerDead };

    int tileAt(int x, int y) const;
    void setTile(int x, int y, int tile);
    MoveResult tryMovePlayer(int nx, int ny);

    enum FightResult { Fight_PlayerWin, Fight_PlayerDead };
    FightResult fightAt(int x, int y, std::vector<std::string>& outLog);

    void spawnMonster(int x, int y, const Monster& m);
    bool hasMonsterAt(int x, int y) const;
    Monster* monsterAt(int x, int y);

    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

private:
    std::vector<int> m_map;
    int m_width;
    int m_height;
    Player m_player;

    std::unordered_map<int, Monster> m_monsters;
};
