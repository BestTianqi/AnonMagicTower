#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
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
    struct MonsterMovementAnimation {
        Monster monster;
        int fromX = 0;
        int fromY = 0;
        int toX = 0;
        int toY = 0;
    };

    Game();
    bool loadDefaultMap();
    void generateClassicTower();

    const std::vector<int>& map() const { return m_currentFloor->map; }
    int width()  const { return m_width; }
    int height() const { return m_height; }

    Player& player() { return m_player; }
    int currentFloor() const { return m_floor; }
    bool floor3TrapActive() const { return m_floor3TrapActive; }
    void clearFloor3Trap() { m_floor3TrapActive = false; }
    bool floor3PrisonStoryPending() const { return m_floor3PrisonStoryPending; }
    bool princessDollPassageUnlocked() const { return m_princessDollRescued; }
    bool floor20VampireTriggered() const { return m_floor20VampireTriggered; }
    bool floor20VampireStoryShown() const { return m_floor20VampireStoryShown; }
    void markFloor20VampireStoryShown() { m_floor20VampireStoryShown = true; }
    bool floor33TrapTriggered() const { return m_floor33TrapTriggered; }
    bool floor14RedKeyRewardGranted() const { return m_floor14RedKeyRewardGranted; }
    bool floor32KnightStoryPending() const { return m_floor32KnightStoryPending; }
    // 32层按原版在骑士队长剧情触发前隐藏怪物图像；实体仍保留用于碰撞/战斗判定。
    bool floor32MonstersHidden() const { return m_floor == 32 && !m_floor32KnightTriggered; }
    bool floor42KnightStoryPending() const { return m_floor42KnightStoryPending; }
    // 全局一次性剧情标记，随存档保存，避免换楼层或读档后重复播放。
    bool storyShown(const std::string& key) const { return m_storyOnceKeys.count(key) != 0; }
    void markStoryShown(const std::string& key) { m_storyOnceKeys.insert(key); }
    // 38层机关剧情与43层藤都子SP退场事件。
    bool floor38FlowerTriggered() const { return m_floor38FlowerTriggered; }
    bool floor43MiyakoRetreated() const { return m_floor43MiyakoRetreated; }
    // 29层剧情结束后，将米歇尔送回二层右下角 (12,12)，等待玩家救出。
    void returnMichelleToFloor2Cage();
    // 二层牢笼中的米歇尔只有在中级守卫被击败后才能救出。
    bool canReleaseMichelleFromCage() const { return m_michelleGuardDefeated; }
    void sendMichelleToFloor29();
    void activateFloor35Michelle();
    void completeFloor35MichelleStory();
    // 50层最终对话揭开米歇尔的伪装，显现长崎素世本体。
    void revealFloor50MichelleIdentity();
    void resolveFloor3PrisonStory();
    // 26层假公主剧情完成后，开启24层通往50层的隐藏楼梯。
    void unlockPrincessDollPassage();

    enum MoveResult { Move_Ok, Move_Block, Move_Pickup, Move_Encounter,
                      Move_NPC, Move_StairsUp, Move_StairsDown, Move_PlayerDead,
                      Move_DoorLocked, Move_Shop };

    int tileAt(int x, int y) const;
    void setTile(int x, int y, int tile);
    MoveResult tryMovePlayer(int nx, int ny);
    // UI 鼠标移动使用的连通性检查，不执行拾取、战斗或开门副作用。
    bool isTeleportReachable(int targetX, int targetY) const;
    // 规划鼠标瞬移路径；只记录路径，不改变玩家坐标，也不触发目标格交互。
    bool beginTeleportPlayerTo(int targetX, int targetY);
    // 在表现层逐格动画结束后提交此前规划的路径和目标格交互。
    MoveResult completeTeleportPlayerTo();
    // 将玩家移动到鼠标选中的可达格，并执行该格应有的交互。
    MoveResult teleportPlayerTo(int targetX, int targetY);
    // 最近一次鼠标移动所经过的逐格路径，供表现层播放连续移动动画。
    std::vector<std::pair<int, int>> takeLastTeleportPath();
    bool lastTeleportNeedsAnimation() const { return m_lastTeleportNeedsAnimation; }
    // 管理员调试传送：绕过可达性、钥匙和剧情触发，只校验楼层/坐标边界。
    bool debugTeleport(int floor, int x, int y);
    // 爱音手机（飞行魔杖）规则：只能从已连通楼梯的楼层前往已访问楼层。
    bool hasVisitedFloor(int floor) const { return m_visitedFloors.count(floor) != 0; }
    bool canUsePhone() const;
    bool phoneTeleportToFloor(int targetFloor);
    std::vector<MonsterMovementAnimation> takeFloor10AmbushMovementAnimations();
    std::vector<MonsterMovementAnimation> takeScriptedMonsterMovementAnimations();
    std::vector<MonsterMovementAnimation> takeFloor32KnightMovementAnimations();
    // 确认32层骑士队长的剧情对白，并执行其先攻。
    int resolveFloor32KnightStory();
    // 42层首次抵达时播放骑士队长被捕剧情，结束后仍停留在42层。
    void resolveFloor42KnightStory();

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
    // 使用大黄门钥匙：开启当前楼层全部黄门，返回开启数量。
    int useMagicKey();
    // 检查上楼器/下楼器的落点是否为纯地板；失败时不应消耗道具。
    bool canTeleportByStairItem(bool up, int srcX, int srcY) const;
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
    // 39层左上房间的两扇黄门开启后生成对称飞行器。
    void resolveFloor39SymmetryFlyer();
    std::vector<std::pair<int, int>> findTeleportPath(int targetX, int targetY) const;
    bool isTeleportStoryPoint(int x, int y) const;
    // 经典魔塔的魔法领域/夹击伤害，在玩家每经过一个格子时结算。
    int applyApproachHazardsAt(int x, int y);

    // 10 层中央 Boss 区的骷髅士兵包围事件。
    void triggerFloor3PrisonStoryIfNeeded();
    void prepareFloor3PrisonCell();
    void triggerFloor10AmbushIfNeeded();
    void triggerFloor33TrapIfNeeded();
    void resolveFloor33TrapIfCleared();
    void triggerFloor14RewardIfCleared();
    void triggerFloor20VampireIfNeeded();
    void triggerFloor32KnightIfNeeded();
    // 32层骑士队长只沿可行走地板移动；返回的路径包含起点和终点。
    std::vector<std::pair<int, int>> floor32KnightRoute() const;
    void triggerFloor42KnightIfNeeded();
    void resolveFloor34RewardIfCleared();
    void revealFloor35RewardsIfDragonDefeated();
    void spawnFloor40DeferredRewards();
    void resolveFloor10AmbushIfCleared();
    bool m_princessDollRescued = false;
    bool m_floor20VampireTriggered = false;
    bool m_floor20VampireStoryShown = false;
    bool m_floor14RedKeyRewardGranted = false;
    bool m_floor32KnightTriggered = false;
    bool m_floor32KnightStoryPending = false;
    bool m_floor42KnightStoryTriggered = false;
    bool m_floor42KnightStoryPending = false;
    bool m_floor34RewardGranted = false;
    bool m_floor35RewardsHidden = false;
    bool m_floor40BossDefeated = false;
    bool m_floor40RewardsGranted = false;
    std::unordered_map<int, std::pair<std::string, int>> m_floor35HiddenItems;
    bool m_floor33TrapTriggered = false;
    bool m_michelleRescued = false;
    bool m_michelleGuardDefeated = false;
    bool m_floor38FlowerTriggered = false;
    bool m_floor43MiyakoRetreated = false;
    std::unordered_set<std::string> m_storyOnceKeys;
    bool m_floor3PrisonTriggered = false;
    bool m_floor3PrisonStoryPending = false;
    bool m_floor3TrapActive = false;
    bool m_floor10AmbushTriggered = false;
    std::unordered_set<int> m_visitedFloors;
    std::unordered_set<int> m_floor10AmbushMonsterKeys;
    std::unordered_set<int> m_floor10AmbushDoorKeys;
    std::vector<MonsterMovementAnimation> m_floor10AmbushMovements;
    std::vector<MonsterMovementAnimation> m_scriptedMonsterMovements;
    std::vector<MonsterMovementAnimation> m_floor32KnightMovements;
    std::vector<std::pair<int, int>> m_lastTeleportPath;
    std::vector<std::pair<int, int>> m_pendingTeleportPath;
    int m_pendingTeleportTargetX = -1;
    int m_pendingTeleportTargetY = -1;
    bool m_lastTeleportNeedsAnimation = false;
};
