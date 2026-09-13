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
    Tile_Shop,
    Tile_DarkWall,
    Tile_DoorMagic,
    Tile_DoorIron,
    Tile_Lava,
    Tile_StarRiver
};

constexpr int MAP_SIZE = 15;

struct ShopData {
    int potionPrice = 0;
    int weaponPrice = 0;
    int armorPrice  = 0;
    int potionValue = 200;
    int weaponValue = 5;
    int armorValue  = 8;
    // 原版商人/罐子编号；0 表示编辑器自定义商店。
    int classicNpcId = 0;
    int classicShopFloor = 0;
    int classicPurchaseCount = 0; // 兼容旧存档；运行时统一使用 Player::shopUseCount
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
    void generateClassicTower();

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
    // UI 鼠标移动使用的连通性检查，不执行拾取、战斗或开门副作用。
    bool isTeleportReachable(int targetX, int targetY) const;
    // 将玩家移动到鼠标选中的可达格，并执行该格应有的交互。
    MoveResult teleportPlayerTo(int targetX, int targetY);

    enum FightResult { Fight_PlayerWin, Fight_PlayerDead, Fight_GameWin, Fight_Stalemate };
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
    ShopData* shopAt(int x, int y);

    bool breakWall(int x, int y);
    int useBomb();
    int useEarthquakeScroll();
    int useFreezeMagic();
    void goUpFloor(int srcX, int srcY, bool findStairs = true);
    void goDownFloor(int srcX, int srcY, bool findStairs = true);
    void initFloor(int floor);

    int posKey(int x, int y) const { return y * m_width + x; }

    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

    static std::unique_ptr<Item> createItemByName(const std::string& iname, int ival);
    // 将存档/编辑器中的中英文别名统一为游戏显示名；未知名称返回空字符串。
    static std::string canonicalItemName(const std::string& iname);
    static bool isKnownItemName(const std::string& iname);

    FloorData& currentFloorData() { return *m_currentFloor; }
    const FloorData& currentFloorData() const { return *m_currentFloor; }

private:
    int m_width;
    int m_height;
    int m_floor = 1;
    Player m_player;

    std::unordered_map<int, FloorData> m_floors;
    FloorData* m_currentFloor = nullptr;

    // 按原版楼层事件检查机关门，并在满足条件时自动开门。
    void openMechanismDoorsIfReady();
};
