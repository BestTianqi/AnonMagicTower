#pragma once

#include <vector>
#include <unordered_map>
#include "Entities/Player.h"
#include "Entities/Items.h"
#include "Entities/Monster.h"

enum TileType { Tile_Empty = 0, Tile_Wall, Tile_Floor, Tile_StairsUp, Tile_StairsDown, Tile_Monster, Tile_Item };

class Game {
public:
    Game();
    bool loadDefaultMap();

    const std::vector<int>& map() const { return m_map; }
    int width() const { return m_width; }
    int height() const { return m_height; }

    Player& player() { return m_player; }

    // 新增：移动相关接口
    enum MoveResult { Move_Ok, Move_Block, Move_Pickup, Move_Encounter, Move_StairsUp, Move_StairsDown, Move_PlayerDead };

    int tileAt(int x, int y) const;
    void setTile(int x, int y, int tile);
    MoveResult tryMovePlayer(int nx, int ny);

    // 战斗结果
    enum FightResult { Fight_PlayerWin, Fight_PlayerDead };
    FightResult fightAt(int x, int y);

    // 怪物管理
    void spawnMonster(int x, int y, const Monster& m);
    bool hasMonsterAt(int x, int y) const;
    Monster* monsterAt(int x, int y);

private:
    std::vector<int> m_map;
    int m_width;
    int m_height;
    Player m_player;

    // key = y*width + x
    std::unordered_map<int, Monster> m_monsters;
};
