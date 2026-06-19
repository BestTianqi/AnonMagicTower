#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include "Entities/Player.h"
#include "Entities/Items.h"
#include "Entities/Monster.h"
#include "Entities/NPC.h"

enum TileType {
    Tile_Empty = 0,
    Tile_Wall,
    Tile_Floor,
    Tile_StairsUp,
    Tile_StairsDown,
    Tile_Monster,
    Tile_Item,
    Tile_DoorRed,
    Tile_DoorBlue,
    Tile_DoorGreen,
    Tile_NPC,
    Tile_Shop
};

constexpr int MAP_SIZE = 15;

struct ShopData {
    int potionPrice = 0;
    int weaponPrice = 0;
    int armorPrice  = 0;
    int potionValue = 200;
    int weaponValue = 5;
    int armorValue  = 8;
};

struct FloorData {
    std::vector<int> map;
    std::unordered_map<int, Monster> monsters;
    std::unordered_map<int, std::unique_ptr<Item>> items;
    std::unordered_map<int, NPC> npcs;
    std::unordered_map<int, ShopData> shops;
};

class Game {
public:
    Game();
    bool loadDefaultMap();

    const std::vector<int>& map() const { return m_currentFloor->map; }
    int width()  const { return m_width; }
    int height() const { return m_height; }

    Player& player() { return m_player; }
    int currentFloor() const { return m_floor; }

    enum MoveResult { Move_Ok, Move_Block, Move_Pickup, Move_Encounter,
                      Move_NPC, Move_StairsUp, Move_StairsDown, Move_PlayerDead,
                      Move_DoorLocked, Move_Shop };

    int tileAt(int x, int y) const;
    void setTile(int x, int y, int tile);
    MoveResult tryMovePlayer(int nx, int ny);

    enum FightResult { Fight_PlayerWin, Fight_PlayerDead };
    FightResult fightAt(int x, int y, std::vector<std::string>& outLog);

    void spawnMonster(int x, int y, const Monster& m);
    bool hasMonsterAt(int x, int y) const;
    Monster* monsterAt(int x, int y);

    void addItemAt(int x, int y, std::unique_ptr<Item> item);
    const Item* itemAt(int x, int y) const;
    std::unique_ptr<Item> takeItemAt(int x, int y);

    void addNPCAt(int x, int y, NPC npc);
    NPC* npcAt(int x, int y);

    void addShopAt(int x, int y, const ShopData& s);
    const ShopData* shopAt(int x, int y) const;

    bool breakWall(int x, int y);
    void goUpFloor(int srcX, int srcY);
    void goDownFloor(int srcX, int srcY);
    void initFloor(int floor);

    int posKey(int x, int y) const { return y * m_width + x; }

    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

private:
    int m_width;
    int m_height;
    int m_floor = 1;
    Player m_player;

    std::unordered_map<int, FloorData> m_floors;
    FloorData* m_currentFloor = nullptr;

    FloorData& currentFloorData() { return *m_currentFloor; }
    const FloorData& currentFloorData() const { return *m_currentFloor; }
};
