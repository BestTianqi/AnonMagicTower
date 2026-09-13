#include "Game.h"
#include "Entities/MonsterDB.h"
#include <QString>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <algorithm>
#include <fstream>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace {

bool hasClassicMonsterId(const FloorData& floor, int id)
{
    for (const auto& entry : floor.monsters) {
        if (MonsterDB::indexOf(entry.second.GetName()) == id - 1)
            return true;
    }
    return false;
}

bool mechanismDoorReadyAt(int floorNumber, const FloorData& floor, int doorX, int doorY)
{
    // 10 层花门由进入中央 Boss 区的剧情事件开启，而不是清怪开启。
    if (floorNumber == 10) return false;
    // 原版 48 层圣剑房花门是损坏的机关，清怪后仍不会自动开启。
    if (floorNumber == 48) return false;

    if (floor.map[doorY * MAP_SIZE + doorX] == Tile_DoorMagic) {
        if (floorNumber == 49) {
            // 49层上下两扇花门各自只绑定门正下方横向三格（左、中、右）。
            // 两扇门互不共享守卫，其他位置的同类怪物不参与判定。
            bool hasBelowGuard = false;
            for (int dx = -1; dx <= 1; ++dx) {
                const int key = (doorY + 1) * MAP_SIZE + (doorX + dx);
                if (floor.monsters.find(key) != floor.monsters.end()) {
                    hasBelowGuard = true;
                    break;
                }
            }
            if (!hasBelowGuard) return true;
            for (int dx = -1; dx <= 1; ++dx) {
                const int key = (doorY + 1) * MAP_SIZE + (doorX + dx);
                if (floor.monsters.find(key) != floor.monsters.end()) return false;
            }
            return true;
        }

        // 花门只绑定自身周围八格的怪物：周围有一只或两只就只清理这一小组，
        // 同编号但位于楼层其他区域的怪物不参与本门判定。
        bool hasAdjacentMonster = false;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                const int x = doorX + dx;
                const int y = doorY + dy;
                const int key = y * MAP_SIZE + x;
                if (floor.monsters.find(key) != floor.monsters.end()) {
                    hasAdjacentMonster = true;
                    break;
                }
            }
            if (hasAdjacentMonster) break;
        }
        if (hasAdjacentMonster) {
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    const int key = (doorY + dy) * MAP_SIZE + (doorX + dx);
                    if (floor.monsters.find(key) != floor.monsters.end()) return false;
                }
            }
            return true;
        }

        // 花门没有邻近守卫时直接开放；全楼层指定怪物组只用于铁门等原版特殊门。
        return true;
    }

    // 某些原版门（例如二层六扇铁门）周围没有怪物，仍绑定原版指定守卫
    // ID（1-based）。未列出的自定义楼层使用“清空本层”通用规则。
    static const std::unordered_map<int, std::vector<int>> guardGroups = {
        {2,  {21}},                         // 六扇铁门：两名中级卫兵
        {8,  {1, 2, 3, 4, 5, 6, 7}},       // 1–7号怪物
        {11, {3, 9, 10, 11, 12}},           // 初级法师、大史莱姆/大蝙蝠/高级法师/兽人
        {15, {3, 9, 10, 11, 12, 13}},
        {17, {7, 9, 10, 11, 12, 13}},
        {20, {3, 10, 11, 14}},
        {30, {1, 2, 9}},
        {32, {18, 19, 20, 21, 24}},
        {35, {23}},
        {38, {12, 18, 19, 20, 21, 22, 24}},
        {44, {32}},
        {45, {26, 27, 28, 29, 30, 31}},
        {49, {27, 30}},
    };
    static const std::unordered_set<int> classicDoorFloors = {
        2, 8, 10, 11, 15, 17, 20, 30, 32, 35, 38, 44, 45, 48, 49
    };
    const auto it = guardGroups.find(floorNumber);
    if (it == guardGroups.end()) {
        // 经典塔有机关门但未配置守卫组时禁止自动开门，避免误用编辑器楼层的
        // “清空本层”兜底规则；非经典自定义楼层仍保留该兜底行为。
        if (classicDoorFloors.count(floorNumber) != 0) return false;
        return floor.monsters.empty();
    }
    for (int id : it->second)
        if (hasClassicMonsterId(floor, id)) return false;
    return true;
}

bool mechanismDoorReady(int floorNumber, const FloorData& floor)
{
    // 仅供需要楼层级判断的旧调用；实际开门路径会逐门调用上面的函数。
    for (int y = 0; y < MAP_SIZE; ++y) {
        for (int x = 0; x < MAP_SIZE; ++x) {
            const int key = y * MAP_SIZE + x;
            if (floor.map[key] != Tile_DoorMagic && floor.map[key] != Tile_DoorIron) continue;
            if (mechanismDoorReadyAt(floorNumber, floor, x, y)) return true;
        }
    }
    return false;
}

} // namespace

Game::Game()
    : m_width(MAP_SIZE), m_height(MAP_SIZE)
{
    initFloor(1);
}

void Game::generateClassicTower()
{
    m_floors.clear();
    m_floor = 1;
    m_floor3PrisonTriggered = false;
    m_floor3PrisonStoryPending = false;
    m_floor3TrapActive = false;
    m_floor10AmbushTriggered = false;
    m_floor10AmbushMonsterKeys.clear();
    m_floor10AmbushDoorKeys.clear();
    m_floor10AmbushMovements.clear();
    for (int floor = 1; floor <= 50; ++floor) {
        FloorData fd;
        fd.map.assign(m_width * m_height, Tile_Wall);
        for (int y = 2; y <= 12; ++y)
            for (int x = 2; x <= 12; ++x)
                fd.map[y * m_width + x] = Tile_Floor;
        m_floors.emplace(floor, std::move(fd));
    }

    QFile source(":/data/classic50_map.txt");
    if (!source.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_currentFloor = &m_floors[m_floor];
        return;
    }

    auto makeClassicItem = [](int floor, int id) -> std::unique_ptr<Item> {
        const ClassicItemTier tier = classicItemTierForFloor(floor);
        switch (id) {
        case 1:  return std::make_unique<Key>(KeyType::Green); // 内部第三钥匙槽即黄钥匙
        case 2:  return std::make_unique<Key>(KeyType::Blue);
        case 3:  return std::make_unique<Key>(KeyType::Red);
        case 4:  return std::make_unique<MagicKey>();
        case 5:  return std::make_unique<SmallPotion>(tier.smallPotionHp);
        case 6:  return std::make_unique<LargePotion>(tier.largePotionHp);
        case 7:  return std::make_unique<RubyGem>(tier.rubyAttack);
        case 8:  return std::make_unique<SapphireGem>(tier.sapphireDefense);
        case 9:  return std::make_unique<FlyingWand>();
        case 11: return std::make_unique<NoteBook>();
        case 10: return std::make_unique<AnonGlasses>();
        case 15: return std::make_unique<FreezeMagic>();
        case 16: return std::make_unique<Bomb>();
        case 19: return std::make_unique<Cross>();
        case 13: return std::make_unique<Pickaxe>();
        case 17: return std::make_unique<HolyWater>();
        case 18: return std::make_unique<LuckyCoin>();
        case 21: return std::make_unique<StairUpper>();
        case 22: return std::make_unique<StairLower>();
        case 24: return std::make_unique<Weapon>(10, "爱音拨片");
        case 25: return std::make_unique<Armor>(10, "素世谱架");
        case 26: return std::make_unique<Weapon>(20, "立希鼓棒");
        case 27: return std::make_unique<Armor>(20, "海铃节拍器");
        case 28: return std::make_unique<Weapon>(40, "乐奈猫爪");
        case 29: return std::make_unique<Armor>(40, "初华舞台耳返");
        case 30: return std::make_unique<Weapon>(50, "灯的麦克风");
        case 31: return std::make_unique<HolyShield>(50, "祥子黑色乐谱");
        case 32: return std::make_unique<Weapon>(100, "睦的贝斯");
        case 33: return std::make_unique<DivineShield>(100, "Mujica终幕面具");
        default: return std::make_unique<Item>("ClassicArtifact", id);
        }
    };

    QTextStream stream(&source);
    const QString header = stream.readLine().trimmed();
    if (header != "CLASSIC50_MAP_V1") {
        m_currentFloor = &m_floors[m_floor];
        return;
    }
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        QTextStream fields(const_cast<QString*>(&line), QIODevice::ReadOnly);
        int level = 0, type = 0, id = 0, sourceX = 0, sourceY = 0;
        fields >> level >> type >> id >> sourceX >> sourceY;
        if (fields.status() != QTextStream::Ok || level < 1 || level > 50) continue;

        const int x = sourceX + 7;
        const int y = 7 - sourceY;
        if (x < 2 || x > 12 || y < 2 || y > 12) continue;
        FloorData& fd = m_floors[level];
        int key = y * m_width + x;

        if (type == 0) {
            int tile = Tile_Floor;
            if (id == 1) tile = Tile_DoorGreen;       // 黄门（沿用旧存档枚举值）
            else if (id == 2) tile = Tile_DoorBlue;
            else if (id == 3) tile = Tile_DoorRed;
            else if (id == 6) tile = Tile_Wall;
            else if (id == 15) tile = Tile_Lava;
            else if (id == 38) tile = Tile_StarRiver;
            else if (id == 7) tile = Tile_StairsUp;
            else if (id == 8) tile = Tile_StairsDown;
            else if (id == 12 || id == 16 || id == 19 || id == 25 ||
                     id == 27 || id == 29 || id == 32) tile = Tile_DarkWall;
            else if (id == 4 || id == 5 || id == 18 || id == 20 ||
                     id == 22 || id == 26 || id == 34) tile = Tile_DoorMagic;
            else if (id == 9 || id == 11) tile = Tile_DoorIron;
            fd.map[key] = tile;
        } else if (type == 1) {
            fd.items[key] = makeClassicItem(level, id);
            fd.map[key] = Tile_Item;
        } else if (type == 2) {
            if (id == 1) continue; // 原版玩家出生标记
            if (id == 46) { // 50 层小偷剧情位置会显现魔王本体
                fd.monsters[key] = MonsterDB::getByIndex(33);
                fd.map[key] = Tile_Monster;
                continue;
            }
            const bool merchant = id == 6 || id == 7 || id == 8 || id == 9 || id == 11 ||
                                  id == 15 || id == 16 || id == 27 || id == 28 || id == 36 ||
                                  id == 38 || id == 41 || id == 43 || id == 44;
            if (merchant) {
                fd.map[key] = Tile_Shop;
                ShopData shop{20 + level * 10, 20 + level * 10, 20 + level * 10,
                              100 + level * 5, 2 + level / 10, 2 + level / 10};
                shop.classicNpcId = id;
                shop.classicShopFloor = id == 15 ? 4 : id == 16 ? 12 : id == 28 ? 32 : id == 43 ? 46 : 0;
                if (shop.classicShopFloor > 0) {
                    const auto offer = classicShopOfferForFloor(shop.classicShopFloor, 0);
                    shop.potionPrice = shop.weaponPrice = shop.armorPrice = offer.price;
                    shop.potionValue = offer.hp;
                    shop.weaponValue = offer.atk;
                    shop.armorValue = offer.def;
                }
                fd.shops[key] = shop;
            } else {
                fd.map[key] = Tile_NPC;
                // 二层小偷使用原版牢房坐标 (4,8)；其左侧 (3,8) 是待开启暗墙。
                if (level == 2 && id == 13) {
                    fd.map[posKey(3, 8)] = Tile_DarkWall;
                }
                const char* name = (id == 22) ? "公主" : ((id == 12 || id == 13 || id == 14 || id == 25 || id == 30 || id == 31 || id == 46) ? "小偷" : "老头");
                std::vector<std::string> dialog{"这是第 " + std::to_string(level) + " 层。继续探索吧。"};
                switch (id) {
                case 2: dialog = {"欢迎来到魔塔，先收集钥匙和装备吧。"}; break;
                case 3: dialog = {"这本怪物手册交给你。", "它能查看本层怪物的能力。"}; break;
                case 4: dialog = {"购买物品后再与商人对话，他会告诉你重要信息。"}; break;
                case 5: dialog = {"魔塔里隐藏着许多秘密通道。"}; break;
                case 12: case 13: dialog = {"喂！别挡路，我要先走一步。"}; break;
                case 14: case 25: case 31: dialog = {"我挖了一条暗道，已经替你打开了。"}; break;
                case 17: dialog = {"听说塔内有两把隐藏的红钥匙。"}; break;
                case 19: dialog = {"注意那些颜色与众不同的墙。"}; break;
                case 20: dialog = {"大法师在25层，他是魔塔的主人。"}; break;
                case 21: dialog = {"25层以后会遇到更强的敌人，准备好再前进。"}; break;
                case 23: dialog = {"27层的道路被封锁了，寻找隐藏的道路吧。"}; break;
                case 26: dialog = {"魔塔有50层，但你不能直接到达顶层。"}; break;
                case 29: dialog = {"有些墙壁只是伪装，靠近它们试试看。"}; break;
                case 30: dialog = {"救救我！这里的牢门似乎能被打开。"}; break;
                case 34: dialog = {"前方的道路需要更高级的装备。"}; break;
                case 35: dialog = {"秘宝被藏在更高的楼层。"}; break;
                case 37: dialog = {"翡翠剑的房间需要用镐破墙进入。"}; break;
                case 39: dialog = {"通往异界的入口就在不远处。"}; break;
                case 40: dialog = {"44层被藏在异界，只有通过秘宝才能到达。"}; break;
                case 42: dialog = {"神圣盾能免疫魔法攻击，但它被藏在异界内。"}; break;
                case 45: dialog = {"要打败魔龙必须准备神圣剑、神圣盾或屠龙匕首。"}; break;
                default: break;
                }
                std::unique_ptr<Item> reward;
                if (id == 3) reward = std::make_unique<NoteBook>();
                else if (id == 18) reward = std::make_unique<HolyWater>();
                else if (id == 32) reward = std::make_unique<Treasure>(1000);
                NPC npc(name, dialog, std::move(reward), false, 0, nullptr, id);
                fd.npcs.emplace(key, std::move(npc));
            }
        } else if (type == 3 && id >= 1 && id <= 34) {
            fd.monsters[key] = MonsterDB::getByIndex(id - 1);
            fd.map[key] = Tile_Monster;
        }
    }

    auto spawnEventMonster = [this](int level, int sourceX, int sourceY, int monsterId) {
        FloorData& fd = m_floors[level];
        const int x = sourceX + 7;
        const int y = 7 - sourceY;
        const int key = y * m_width + x;
        fd.monsters[key] = MonsterDB::getByIndex(monsterId - 1);
        fd.map[key] = Tile_Monster;
    };

    // 原版脚本动态生成、因而不在静态 mapinfo 中的关键战斗。
    spawnEventMonster(20, 0, 0, 16);
    m_floors[20].map[(7 - (-3)) * m_width + (0 + 7)] = Tile_DoorMagic;
    spawnEventMonster(49, 0, 3, 33);
    const int guardPositions[][2] = {
        {-1, 4}, {0, 4}, {1, 4}, {-1, 3}, {1, 3}, {-1, 2}, {0, 2}, {1, 2}
    };
    for (const auto& point : guardPositions)
        spawnEventMonster(49, point[0], point[1], 31);

    prepareFloor3PrisonCell();
    m_currentFloor = &m_floors[m_floor];
    m_player = Player();
    m_player.x = 7;
    m_player.y = 12;
    m_player.hp = 1000;
    // 经典塔新游戏的序章初始攻防均为 100，三层陷阱后固定降至 10。
    m_player.atk = 100;
    m_player.def = 100;
}

void Game::initFloor(int floor)
{
    if (m_floors.find(floor) == m_floors.end()) {
        FloorData fd;
        fd.map.assign(m_width * m_height, Tile_Floor);
        // 两圈墙壁 (rows 0,1,13,14 and cols 0,1,13,14)
        for (int x = 0; x < m_width; ++x) {
            fd.map[0 * m_width + x]                = Tile_Wall;
            fd.map[1 * m_width + x]                = Tile_Wall;
            fd.map[(m_height-2)*m_width + x]       = Tile_Wall;
            fd.map[(m_height-1)*m_width + x]       = Tile_Wall;
        }
        for (int y = 0; y < m_height; ++y) {
            fd.map[y*m_width + 0]       = Tile_Wall;
            fd.map[y*m_width + 1]       = Tile_Wall;
            fd.map[y*m_width + (m_width-2)] = Tile_Wall;
            fd.map[y*m_width + (m_width-1)] = Tile_Wall;
        }
        m_floors[floor] = std::move(fd);
    }
    m_currentFloor = &m_floors[floor];
    prepareFloor3PrisonCell();
}

bool Game::loadDefaultMap()
{
    m_floor3PrisonTriggered = false;
    m_floor3PrisonStoryPending = false;
    m_floor3TrapActive = false;
    m_floor10AmbushTriggered = false;
    m_floor10AmbushMonsterKeys.clear();
    m_floor10AmbushDoorKeys.clear();
    m_floor10AmbushMovements.clear();
    // 从嵌入资源加载默认地图
    QFile res(":/map.txt");
    if (res.open(QIODevice::ReadOnly)) {
        QString tmpPath = QDir::tempPath() + "/mota_default_map.txt";
        QFile tmp(tmpPath);
        if (tmp.open(QIODevice::WriteOnly)) {
            tmp.write(res.readAll());
            tmp.close();
        }
        res.close();
        if (loadFromFile(tmpPath.toStdString())) {
            prepareFloor3PrisonCell();
            return true;
        }
    }

    // 后备：空地图
    m_floor = 1;
    initFloor(1);
    m_player.x   = 2;
    m_player.y   = 3;
    m_player.hp  = 100;
    m_player.atk = 10;
    m_player.def = 5;

    return true;
}

int Game::tileAt(int x, int y) const
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height) return Tile_Empty;
    return m_currentFloor->map[y * m_width + x];
}

void Game::setTile(int x, int y, int tile)
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height) return;
    m_currentFloor->map[y * m_width + x] = tile;
}

void Game::spawnMonster(int x, int y, const Monster& m)
{
    int key = posKey(x, y);
    m_currentFloor->monsters.emplace(key, m);
}

bool Game::hasMonsterAt(int x, int y) const
{
    int key = posKey(x, y);
    return m_currentFloor->monsters.find(key) != m_currentFloor->monsters.end();
}

Monster* Game::monsterAt(int x, int y)
{
    int key = posKey(x, y);
    auto it = m_currentFloor->monsters.find(key);
    if (it == m_currentFloor->monsters.end()) return nullptr;
    return &it->second;
}

void Game::addItemAt(int x, int y, std::unique_ptr<Item> item)
{
    int key = posKey(x, y);
    m_currentFloor->items[key] = std::move(item);
}

const Item* Game::itemAt(int x, int y) const
{
    int key = posKey(x, y);
    auto it = m_currentFloor->items.find(key);
    if (it == m_currentFloor->items.end()) return nullptr;
    return it->second.get();
}

std::unique_ptr<Item> Game::takeItemAt(int x, int y)
{
    int key = posKey(x, y);
    auto it = m_currentFloor->items.find(key);
    if (it == m_currentFloor->items.end()) return nullptr;
    auto item = std::move(it->second);
    m_currentFloor->items.erase(it);
    return item;
}

void Game::addNPCAt(int x, int y, NPC npc)
{
    int key = posKey(x, y);
    m_currentFloor->npcs.emplace(key, std::move(npc));
}

NPC* Game::npcAt(int x, int y)
{
    int key = posKey(x, y);
    auto it = m_currentFloor->npcs.find(key);
    if (it == m_currentFloor->npcs.end()) return nullptr;
    return &it->second;
}

void Game::addShopAt(int x, int y, const ShopData& s)
{
    int key = posKey(x, y);
    m_currentFloor->shops.emplace(key, s);
}

const ShopData* Game::shopAt(int x, int y) const
{
    int key = posKey(x, y);
    auto it = m_currentFloor->shops.find(key);
    if (it == m_currentFloor->shops.end()) return nullptr;
    return &it->second;
}

bool Game::isTeleportReachable(int targetX, int targetY) const
{
    if (targetX < 0 || targetY < 0 || targetX >= m_width || targetY >= m_height)
        return false;
    if (targetX == m_player.x && targetY == m_player.y)
        return true;

    const auto walkable = [](int tile) {
        return tile == Tile_Floor || tile == Tile_Item ||
               tile == Tile_StairsUp || tile == Tile_StairsDown;
    };
    const int targetTile = tileAt(targetX, targetY);
    const bool targetInteractable = targetTile == Tile_NPC || targetTile == Tile_Shop ||
                                    (targetTile == Tile_Monster && hasMonsterAt(targetX, targetY));
    const bool targetDoorWithKey =
        (targetTile == Tile_DoorRed && (m_player.HasKey(KeyType::Red) || m_player.magicKeyUses > 0)) ||
        (targetTile == Tile_DoorBlue && (m_player.HasKey(KeyType::Blue) || m_player.magicKeyUses > 0)) ||
        (targetTile == Tile_DoorGreen && (m_player.HasKey(KeyType::Green) || m_player.magicKeyUses > 0));
    const bool targetMechanismDoor =
        (targetTile == Tile_DoorMagic || targetTile == Tile_DoorIron) &&
        ((m_floor == 48 && m_player.wallBreakerUsed) ||
         (m_currentFloor && mechanismDoorReadyAt(m_floor, *m_currentFloor, targetX, targetY)));
    if (!walkable(targetTile) && !targetInteractable && !targetDoorWithKey && !targetMechanismDoor)
        return false;
    const int currentTile = tileAt(m_player.x, m_player.y);
    const bool currentInteractable =
        currentTile == Tile_NPC || currentTile == Tile_Shop ||
        (currentTile == Tile_Monster && hasMonsterAt(m_player.x, m_player.y));
    // 玩家可能正站在刚瞬移到的 NPC/商店格，仍应以该格为 BFS 起点。
    if (!walkable(currentTile) && !currentInteractable) return false;

    std::vector<unsigned char> visited(static_cast<size_t>(m_width * m_height), 0);
    std::queue<std::pair<int, int>> pending;
    const auto index = [this](int x, int y) { return y * m_width + x; };
    pending.emplace(m_player.x, m_player.y);
    visited[index(m_player.x, m_player.y)] = 1;
    static constexpr int directions[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

    while (!pending.empty()) {
        const auto [x, y] = pending.front();
        pending.pop();
        for (const auto& direction : directions) {
            const int nx = x + direction[0];
            const int ny = y + direction[1];
            if (nx < 0 || ny < 0 || nx >= m_width || ny >= m_height) continue;
            // 交互对象本身不可作为路径中间节点，但可以作为终点点击。
            if (nx == targetX && ny == targetY) return true;
            const int cell = index(nx, ny);
            if (visited[cell] || !walkable(tileAt(nx, ny))) continue;
            visited[cell] = 1;
            pending.emplace(nx, ny);
        }
    }
    return false;
}

Game::MoveResult Game::teleportPlayerTo(int targetX, int targetY)
{
    if (m_floor3PrisonStoryPending) return Move_Block;
    if (!isTeleportReachable(targetX, targetY)) return Move_Block;

    const int tile = tileAt(targetX, targetY);
    m_player.x = targetX;
    m_player.y = targetY;
    triggerFloor10AmbushIfNeeded();

    switch (tile) {
    case Tile_Item: {
        auto item = takeItemAt(targetX, targetY);
        if (!item) {
            setTile(targetX, targetY, Tile_Floor);
            return Move_Ok;
        }
        if (item->IsUseItem()) {
            m_player.AddItem(std::move(item));
        } else if (item->IsPassiveEffect()) {
            item->Apply(m_player);
            m_player.AddItem(std::move(item));
        } else {
            item->Apply(m_player);
        }
        setTile(targetX, targetY, Tile_Floor);
        return Move_Pickup;
    }
    case Tile_NPC:
        return Move_NPC;
    case Tile_Shop:
        return Move_Shop;
    case Tile_Monster:
        return hasMonsterAt(targetX, targetY) ? Move_Encounter : Move_Ok;
    case Tile_DoorRed:
    case Tile_DoorBlue:
    case Tile_DoorGreen:
        // 复用普通移动的开门逻辑，确保钥匙/万能钥匙只消耗一次。
        return tryMovePlayer(targetX, targetY);
    case Tile_DoorMagic:
    case Tile_DoorIron:
        return tryMovePlayer(targetX, targetY);
    case Tile_StairsUp:
        return Move_StairsUp;
    case Tile_StairsDown:
        return Move_StairsDown;
    default:
        return Move_Ok;
    }
}

bool Game::debugTeleport(int floor, int x, int y)
{
    if (floor < 1 || floor > 50 || x < 0 || y < 0 || x >= m_width || y >= m_height)
        return false;
    if (m_floors.find(floor) == m_floors.end())
        initFloor(floor);
    m_floor = floor;
    m_currentFloor = &m_floors[floor];
    m_player.x = x;
    m_player.y = y;
    // 调试传送应能离开等待中的剧情现场，避免被事件锁死。
    m_floor3PrisonStoryPending = false;
    return true;
}

ShopData* Game::shopAt(int x, int y)
{
    int key = posKey(x, y);
    auto it = m_currentFloor->shops.find(key);
    if (it == m_currentFloor->shops.end()) return nullptr;
    return &it->second;
}

bool Game::breakWall(int x, int y)
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height) return false;
    int idx = y * m_width + x;
    if (m_currentFloor->map[idx] == Tile_Wall || m_currentFloor->map[idx] == Tile_DarkWall) {
        m_currentFloor->map[idx] = m_currentFloor->items.count(idx) ? Tile_Item : Tile_Floor;
        return true;
    }
    return false;
}

int Game::useBomb()
{
    int defeated = 0;
    static const int directions[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (const auto& direction : directions) {
        const int x = m_player.x + direction[0];
        const int y = m_player.y + direction[1];
        const int key = posKey(x, y);
        auto it = m_currentFloor->monsters.find(key);
        if (it == m_currentFloor->monsters.end()) continue;
        const std::string name = it->second.GetName();
        // 原版炸弹不能伤害四类头目。
        if (name.find("魔王") != std::string::npos || name.find("长崎素世") != std::string::npos ||
            name.find("魔龙") != std::string::npos ||
            name.find("大法师") != std::string::npos) continue;
        m_player.gold += it->second.GetGold();
        m_currentFloor->monsters.erase(it);
        setTile(x, y, m_currentFloor->items.count(key) ? Tile_Item : Tile_Floor);
        ++defeated;
    }
    openMechanismDoorsIfReady();
    resolveFloor10AmbushIfCleared();
    return defeated;
}

void Game::openMechanismDoorsIfReady()
{
    if (!m_currentFloor) return;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            const int tile = tileAt(x, y);
            if ((tile == Tile_DoorMagic || tile == Tile_DoorIron) &&
                mechanismDoorReadyAt(m_floor, *m_currentFloor, x, y))
                setTile(x, y, Tile_Floor);
        }
    }
}

void Game::prepareFloor3PrisonCell()
{
    if (m_floor3PrisonTriggered || m_floor3PrisonStoryPending) return;
    auto floorIt = m_floors.find(3);
    if (floorIt == m_floors.end()) return;
    FloorData& floor3 = floorIt->second;
    const int key = posKey(6, 7);
    // 剧情触发前，素世所在格是不可直接进入的暗墙；不保留静态地图上
    // 可能落在该格的普通怪物，触发时再由事件显现素世与四名警卫。
    floor3.monsters.erase(key);
    floor3.map[key] = Tile_DarkWall;
}

void Game::triggerFloor3PrisonStoryIfNeeded()
{
    if (m_floor != 3 || m_floor3PrisonTriggered || m_floor3PrisonStoryPending || !m_currentFloor)
        return;
    // 三层入口附近的 (6,9) 是原版“走入包围圈”的剧情点。
    if (m_player.x != 6 || m_player.y != 9)
        return;

    m_floor3PrisonTriggered = true;
    m_floor3PrisonStoryPending = true;
    m_floor3TrapActive = true;
    // 将素世所在的暗墙格替换为事件怪物，再显现四名魔法警卫。
    // 先在三层留下事件现场：长崎素世位于主角上方，四名魔法警卫
    // 围住触发格的四个方向。战斗结束后仍可返回三层查看现场。
    const auto placeTrapMonster = [this](int x, int y, const Monster& monster) {
        const int key = posKey(x, y);
        m_currentFloor->monsters[key] = monster;
        setTile(x, y, Tile_Monster);
    };
    placeTrapMonster(6, 7, MonsterDB::get("长崎素世·幻影"));
    const int guardPositions[][2] = {{5, 9}, {7, 9}, {6, 8}, {6, 10}};
    for (const auto& position : guardPositions)
        placeTrapMonster(position[0], position[1], MonsterDB::get("藤都子SP·魔法警卫"));
}

void Game::resolveFloor3PrisonStory()
{
    if (!m_floor3PrisonStoryPending || m_floor != 3 || !m_currentFloor)
        return;

    // 原版序章的围攻伤害与虚弱效果：确认剧情后才结算。
    m_player.hp = std::max(1, m_player.hp - 600);
    m_player.atk = 10;
    m_player.def = 10;
    goDownFloor(3, 12, false);
    // 小偷位于二层 (4,8)，主角被扔回其下方的 (4,9)。
    m_player.x = 4;
    m_player.y = 9;

    // 传送完成后清理三层现场的五个临时怪物，不影响三层原有敌人。
    FloorData& floor3 = m_floors[3];
    const int trapPositions[][2] = {{6, 7}, {5, 9}, {7, 9}, {6, 8}, {6, 10}};
    for (const auto& position : trapPositions) {
        const int key = posKey(position[0], position[1]);
        floor3.monsters.erase(key);
        floor3.map[key] = floor3.items.count(key) ? Tile_Item : Tile_Floor;
    }
    m_floor3PrisonStoryPending = false;
}

void Game::triggerFloor10AmbushIfNeeded()
{
    triggerFloor3PrisonStoryIfNeeded();
    if (m_floor != 10 || m_floor10AmbushTriggered || !m_currentFloor)
        return;

    // 原塔第 10 层从红门向上走，在八幡海铃正下方这一格触发剧情。
    if (m_player.x != 7 || m_player.y != 6)
        return;

    bool hasFlowerDoor = false;
    for (int y = 0; y < m_height && !hasFlowerDoor; ++y)
        for (int x = 0; x < m_width; ++x)
            if (tileAt(x, y) == Tile_DoorMagic) {
                hasFlowerDoor = true;
                break;
            }
    // 存档/编辑器若已把花门打开，不重复执行一次性包围事件。
    if (!hasFlowerDoor) {
        m_floor10AmbushTriggered = true;
        return;
    }

    m_floor10AmbushTriggered = true;

    // 八幡海铃退到中央通道最上方；侧翼花门在进入陷阱时取消，
    // 主角上下两格花门堵住，必须清完原地的侧翼怪才能通过。
    const int oldCaptainKey = posKey(7, 5);
    int captainKey = oldCaptainKey;
    auto captainIt = m_currentFloor->monsters.find(captainKey);
    if (captainIt == m_currentFloor->monsters.end() ||
        MonsterDB::indexOf(captainIt->second.GetName()) + 1 != 8) {
        captainIt = std::find_if(m_currentFloor->monsters.begin(), m_currentFloor->monsters.end(),
            [](const auto& entry) {
                return MonsterDB::indexOf(entry.second.GetName()) + 1 == 8;
            });
        if (captainIt != m_currentFloor->monsters.end()) captainKey = captainIt->first;
    }
    if (captainIt != m_currentFloor->monsters.end()) {
        const Monster captain = captainIt->second;
        m_currentFloor->monsters.erase(captainIt);
        setTile(captainKey % m_width, captainKey / m_width, Tile_Floor);
        const int topKey = posKey(7, 2);
        m_currentFloor->monsters[topKey] = captain;
        setTile(7, 2, Tile_Monster);
    }
    setTile(5, 5, Tile_Floor);
    setTile(9, 5, Tile_Floor);
    setTile(7, 5, Tile_DoorMagic);
    setTile(7, 7, Tile_DoorMagic);
    m_floor10AmbushDoorKeys.clear();
    m_floor10AmbushDoorKeys.insert(posKey(7, 5));
    m_floor10AmbushDoorKeys.insert(posKey(7, 7));
    m_floor10AmbushMovements.clear();
    m_floor10AmbushMonsterKeys.clear();

    // 十层陷阱固定绑定地图内侧第三、第四排（坐标行 4、5）的八只侧翼怪物。
    // 不按距离筛选，避免把楼层其他位置的怪物纳入条件。
    // 按当前十层规则，怪物保持原地图位置，不再移动或播放移动动画。
    std::vector<std::pair<int, Monster>> candidates;
    for (const auto& entry : m_currentFloor->monsters) {
        const int row = entry.first / m_width;
        if (row == 4 || row == 5)
            candidates.emplace_back(entry.first, entry.second);
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
    if (candidates.size() > 8) candidates.resize(8);

    for (const auto& candidate : candidates) {
        m_floor10AmbushMonsterKeys.insert(candidate.first);
    }
    m_floor10AmbushMovements.clear();
    resolveFloor10AmbushIfCleared();
}

std::vector<Game::MonsterMovementAnimation> Game::takeFloor10AmbushMovementAnimations()
{
    auto movements = std::move(m_floor10AmbushMovements);
    m_floor10AmbushMovements.clear();
    return movements;
}

void Game::resolveFloor10AmbushIfCleared()
{
    if (m_floor != 10 || !m_floor10AmbushTriggered || !m_currentFloor)
        return;

    bool guardsRemain = false;
    if (!m_floor10AmbushMonsterKeys.empty()) {
        for (const int key : m_floor10AmbushMonsterKeys) {
            if (m_currentFloor->monsters.count(key) != 0) {
                guardsRemain = true;
                break;
            }
        }
    } else {
        // 读档时不保存可变集合，按地图内侧第三、第四排重建十层侧翼守卫集合。
        for (const auto& entry : m_currentFloor->monsters) {
            const int row = entry.first / m_width;
            if (row == 4 || row == 5) {
                guardsRemain = true;
                break;
            }
        }
    }
    if (guardsRemain) return;

    if (m_floor10AmbushDoorKeys.empty()) {
        // 旧存档未保存门集合时，按十层事件的固定上下两门恢复。
        m_floor10AmbushDoorKeys.insert(posKey(7, 5));
        m_floor10AmbushDoorKeys.insert(posKey(7, 7));
    }
    for (const int key : m_floor10AmbushDoorKeys) {
        const int x = key % m_width;
        const int y = key / m_width;
        if (tileAt(x, y) == Tile_DoorMagic || tileAt(x, y) == Tile_DoorIron)
            setTile(x, y, Tile_Floor);
    }
    m_floor10AmbushDoorKeys.clear();
    m_floor10AmbushMonsterKeys.clear();
}

int Game::useEarthquakeScroll()
{
    int cleared = 0;
    for (int y = 2; y <= m_height - 3; ++y) {
        for (int x = 2; x <= m_width - 3; ++x) {
            const int tile = tileAt(x, y);
            if (tile == Tile_Wall || tile == Tile_DarkWall) {
                setTile(x, y, m_currentFloor->items.count(posKey(x, y)) ? Tile_Item : Tile_Floor);
                ++cleared;
            }
        }
    }
    return cleared;
}

int Game::useFreezeMagic()
{
    int frozen = 0;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            if (tileAt(x, y) == Tile_Lava) {
                setTile(x, y, Tile_Floor);
                ++frozen;
            }
        }
    }
    m_player.freezeMagicUsed = false;
    return frozen;
}

void Game::goUpFloor(int srcX, int srcY, bool findStairs)
{
    m_floor++;
    if (m_floors.find(m_floor) == m_floors.end()) {
        initFloor(m_floor);
    }
    m_currentFloor = &m_floors[m_floor];
    prepareFloor3PrisonCell();

    if (!findStairs) {
        // 上楼器：传送到当前位置
        m_player.x = std::clamp(srcX, 2, m_width - 3);
        m_player.y = std::clamp(srcY, 2, m_height - 3);
        return;
    }

    // 在新楼层找到距离源位置最近的对应楼梯
    int bestDist = 9999, bestX = -1, bestY = -1;
    for (int y = 0; y < m_height; ++y)
        for (int x = 0; x < m_width; ++x)
            if (m_currentFloor->map[y * m_width + x] == Tile_StairsDown) {
                int dist = (x - srcX) * (x - srcX) + (y - srcY) * (y - srcY);
                if (dist < bestDist) { bestDist = dist; bestX = x; bestY = y; }
            }
    if (bestX >= 0) {
        m_player.x = bestX;
        m_player.y = bestY;
        return;
    }
    m_player.x = m_width / 2;
    m_player.y = m_height / 2;
}

void Game::goDownFloor(int srcX, int srcY, bool findStairs)
{
    if (m_floor <= 1) return;
    m_floor--;
    m_currentFloor = &m_floors[m_floor];
    prepareFloor3PrisonCell();

    if (!findStairs) {
        // 下楼器：传送到当前位置
        m_player.x = std::clamp(srcX, 2, m_width - 3);
        m_player.y = std::clamp(srcY, 2, m_height - 3);
        return;
    }

    // 在前楼层找到距离源位置最近的对应楼梯
    int bestDist = 9999, bestX = -1, bestY = -1;
    for (int y = 0; y < m_height; ++y)
        for (int x = 0; x < m_width; ++x)
            if (m_currentFloor->map[y * m_width + x] == Tile_StairsUp) {
                int dist = (x - srcX) * (x - srcX) + (y - srcY) * (y - srcY);
                if (dist < bestDist) { bestDist = dist; bestX = x; bestY = y; }
            }
    if (bestX >= 0) {
        m_player.x = bestX;
        m_player.y = bestY;
        return;
    }
    m_player.x = m_width / 2;
    m_player.y = m_height / 2;
}

Game::MoveResult Game::tryMovePlayer(int nx, int ny)
{
    // 夹击现场显现后必须由鼠标点击确认剧情，期间不允许继续移动。
    if (m_floor3PrisonStoryPending) return Move_Block;
    if (nx < 0 || ny < 0 || nx >= m_width || ny >= m_height) return Move_Block;
    int tile = tileAt(nx, ny);

    switch (tile) {
    case Tile_Wall:
        // 检查破墙锤
        if (m_player.wallBreakerUsed) {
            if (breakWall(nx, ny)) {
                m_player.wallBreakerUsed = false;
                m_player.x = nx; m_player.y = ny;
                triggerFloor10AmbushIfNeeded();
                return Move_Ok;
            }
        }
        return Move_Block;

    case Tile_Lava:
        if (m_player.freezeMagicUsed) {
            m_player.freezeMagicUsed = false;
            setTile(nx, ny, Tile_Floor);
            m_player.x = nx; m_player.y = ny;
            triggerFloor10AmbushIfNeeded();
            return Move_Ok;
        }
        return Move_Block;
    case Tile_StarRiver:
        return Move_Block;

    case Tile_DoorMagic:
    case Tile_DoorIron:
        // 机关门不消耗钥匙，击败本楼层指定守卫后自动打开。
        // 48 层圣剑房为原版损坏花门，只能用镐破坏。
        if (m_floor == 48 && m_player.wallBreakerUsed) {
            m_player.wallBreakerUsed = false;
            setTile(nx, ny, Tile_Floor);
            m_player.x = nx; m_player.y = ny;
            triggerFloor10AmbushIfNeeded();
            return Move_Ok;
        }
        if (!mechanismDoorReadyAt(m_floor, *m_currentFloor, nx, ny)) return Move_DoorLocked;
        setTile(nx, ny, Tile_Floor);
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return Move_Ok;

    case Tile_DarkWall:
        // 暗墙是可撞开的机关墙：第一次碰撞只打开墙体，下一次移动才进入。
        // 破墙道具仍可立即打开并进入，普通墙不会走这条分支。
        if (m_player.wallBreakerUsed && breakWall(nx, ny)) {
            m_player.wallBreakerUsed = false;
            m_player.x = nx; m_player.y = ny;
            triggerFloor10AmbushIfNeeded();
            return Move_Ok;
        }
        setTile(nx, ny, m_currentFloor->items.count(posKey(nx, ny)) ? Tile_Item : Tile_Floor);
        return Move_Block;

    case Tile_Floor:
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return Move_Ok;

    case Tile_DoorRed:
        if (m_player.HasKey(KeyType::Red)) {
            m_player.UseKey(KeyType::Red);
        } else if (m_player.magicKeyUses > 0) {
            m_player.magicKeyUses--;
        } else {
            return Move_DoorLocked;
        }
        setTile(nx, ny, Tile_Floor);
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return Move_Ok;

    case Tile_DoorBlue:
        if (m_player.HasKey(KeyType::Blue)) {
            m_player.UseKey(KeyType::Blue);
        } else if (m_player.magicKeyUses > 0) {
            m_player.magicKeyUses--;
        } else {
            return Move_DoorLocked;
        }
        setTile(nx, ny, Tile_Floor);
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return Move_Ok;

    case Tile_DoorGreen:
        if (m_player.HasKey(KeyType::Green)) {
            m_player.UseKey(KeyType::Green);
        } else if (m_player.magicKeyUses > 0) {
            m_player.magicKeyUses--;
        } else {
            return Move_DoorLocked;
        }
        setTile(nx, ny, Tile_Floor);
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return Move_Ok;

    case Tile_Monster:
        if (hasMonsterAt(nx, ny))
            return Move_Encounter;
        else {
            m_player.x = nx; m_player.y = ny;
            triggerFloor10AmbushIfNeeded();
            return Move_Ok;
        }

    case Tile_Item: {
        auto item = takeItemAt(nx, ny);
        if (item) {
            if (item->IsUseItem()) {
                m_player.AddItem(std::move(item));
            } else if (item->IsPassiveEffect()) {
                item->Apply(m_player);
                m_player.AddItem(std::move(item));
            } else {
                item->Apply(m_player);
            }
            setTile(nx, ny, Tile_Floor);
            m_player.x = nx; m_player.y = ny;
            triggerFloor10AmbushIfNeeded();
            return Move_Pickup;
        }
        setTile(nx, ny, Tile_Floor);
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return Move_Ok;
    }

    case Tile_NPC:
        return Move_NPC;

    case Tile_Shop:
        return Move_Shop;

    case Tile_StairsUp:
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return Move_StairsUp;

    case Tile_StairsDown:
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return Move_StairsDown;

    default:
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return Move_Ok;
    }
}

Game::FightResult Game::fightAt(int x, int y, std::vector<std::string>& outLog)
{
    Monster* m = monsterAt(x, y);
    if (!m) {
        outLog.push_back(std::string("没有怪物。"));
        return Fight_PlayerWin;
    }

    std::string bossName = m->GetName();
    if (m_floor == 10 && bossName == "八幡海铃·骷髅队长" && !m_floor10AmbushTriggered) {
        outLog.push_back("八幡海铃挡在花门前，先触发包围事件才能挑战她。");
        return Fight_Stalemate;
    }
    bool hasShield = (m_player.tempShieldCharges > 0);
    int  shieldBonus = hasShield ? 50 : 0;

    while (true) {
        // 玩家攻击：atk - 怪物def (至少为0)
        int attackPower = m_player.atk;
        const bool vampireOrOrc = bossName.find("吸血") != std::string::npos ||
                                  bossName.find("兽人") != std::string::npos;
        const bool dragon = bossName.find("魔龙") != std::string::npos ||
                            bossName.find("龙") != std::string::npos;
        if (m_player.hasCross && vampireOrOrc) attackPower *= 2;
        if (m_player.hasDragonSlayer && dragon) attackPower *= 2;
        int dmgToMonster = attackPower - m->GetDEF();
        if (dmgToMonster < 0) dmgToMonster = 0;

        // 怪物攻击：atk - 玩家def - 护盾 (至少为0)
        int dmgToPlayer = m->Attack() - m_player.def - shieldBonus;
        if (dmgToPlayer < 0) dmgToPlayer = 0;
        const bool magicAttacker = bossName.find("法师") != std::string::npos ||
                                   bossName.find("巫师") != std::string::npos ||
                                   bossName.find("大法师") != std::string::npos ||
                                   bossName.find("魔法") != std::string::npos;
        if (m_player.hasHolyShield && magicAttacker) dmgToPlayer = 0;
        if (m_player.hasPenguinDoll &&
            (bossName.find("高松灯") != std::string::npos || bossName.find("企鹅") != std::string::npos))
            dmgToPlayer /= 2;
        if (m_player.hasMatchaParfait &&
            (bossName.find("要乐奈") != std::string::npos || bossName.find("小猫") != std::string::npos))
            dmgToPlayer /= 2;

        // 双方都无法造成伤害 → 僵局
        if (dmgToMonster <= 0 && dmgToPlayer <= 0) {
            outLog.push_back(std::string("双方防御均高于对方攻击，无法互相造成伤害，战斗结束。"));
            if (hasShield) m_player.tempShieldCharges--;
            return Fight_Stalemate;
        }

        // 不能破防则无法战斗
        if (dmgToMonster <= 0) {
            outLog.push_back(std::string("攻击力不足，无法对 ") + bossName + " 造成伤害！");
            if (hasShield) m_player.tempShieldCharges--;
            return Fight_PlayerDead;
        }

        m->TakeDamageRaw(dmgToMonster);
        {
            std::ostringstream ss;
            ss << "你对 " << bossName << " 造成 " << dmgToMonster << " 点伤害。 剩余HP=" << m->GetHP();
            outLog.push_back(ss.str());
        }
        if (m->IsDead()) {
            int gold = m->GetGold();
            if (m_player.hasLuckyCoin) gold *= 2;
            if (gold > 0) m_player.gold += gold;

            int key = posKey(x, y);
            m_currentFloor->monsters.erase(key);
            setTile(x, y, m_currentFloor->items.count(key) ? Tile_Item : Tile_Floor);
            openMechanismDoorsIfReady();
            if (m_floor == 10 && m_floor10AmbushTriggered &&
                bossName != "八幡海铃·骷髅队长")
                m_floor10AmbushMonsterKeys.erase(key);
            const bool floor10GuardsCleared =
                m_floor == 10 && m_floor10AmbushTriggered &&
                bossName != "八幡海铃·骷髅队长" &&
                m_floor10AmbushMonsterKeys.empty();
            if (floor10GuardsCleared)
                resolveFloor10AmbushIfCleared();

            if (m_floor == 49 && bossName == "藤都子SP·魔法警卫") {
                auto eventKey = [this](int sourceX, int sourceY) {
                    return (7 - sourceY) * m_width + (sourceX + 7);
                };
                const bool sealComplete =
                    !m_currentFloor->monsters.count(eventKey(0, 4)) &&
                    !m_currentFloor->monsters.count(eventKey(-1, 3)) &&
                    !m_currentFloor->monsters.count(eventKey(1, 3)) &&
                    !m_currentFloor->monsters.count(eventKey(0, 2));
                if (sealComplete) {
                    const int bossKey = eventKey(0, 3);
                    m_currentFloor->monsters[bossKey] =
                        Monster("长崎素世·幻影", 800, 500, 100, 500);
                    outLog.push_back("四名魔法警卫形成的封印生效，长崎素世·幻影的属性降为原来的十分之一！");
                }
            }
            if (m_floor == 49 && bossName == "长崎素世·幻影") {
                m_currentFloor->monsters.clear();
                for (int& tile : m_currentFloor->map)
                    if (tile == Tile_Monster) tile = Tile_Floor;
            }

            std::ostringstream ss;
            ss << "你击败了 " << bossName << " 并获得 " << gold << " 金币。";
            outLog.push_back(ss.str());
            // 原版阶段 Boss 击败后会留下奖励并开启通往上一层的出口。
            // 奖励放在 Boss 原位置，避免覆盖地图上的其他静态物件。
            std::unique_ptr<Item> bossReward;
            std::string rewardName;
            int rewardStairY = m_height - 3;
            if (m_floor == 10 && bossName == "八幡海铃·骷髅队长") {
                const auto tier = classicItemTierForFloor(m_floor);
                bossReward = std::make_unique<RubyGem>(tier.rubyAttack, "舞台红宝石");
                rewardName = "舞台红宝石";
            } else if (m_floor == 20 && bossName == "凑友希那·吸血鬼") {
                const auto tier = classicItemTierForFloor(m_floor);
                bossReward = std::make_unique<SapphireGem>(tier.sapphireDefense, "舞台蓝宝石");
                rewardName = "舞台蓝宝石";
            } else if (m_floor == 40 && bossName == "幼年长崎素世·骑士队长") {
                bossReward = std::make_unique<HolyWater>();
                rewardName = "立希水壶";
            }
            if (bossReward) {
                addItemAt(x, y, std::move(bossReward));
                setTile(x, y, Tile_Item);
                setTile(m_width / 2, rewardStairY, Tile_StairsUp);
                outLog.push_back("Boss奖励：" + rewardName + "；地图正中间下方出现向上楼梯！");
            }
            if (hasShield) m_player.tempShieldCharges--;
            if (bossName == "长崎素世·本体")
                return Fight_GameWin;
            return Fight_PlayerWin;
        }

        m_player.hp -= dmgToPlayer;
        {
            std::ostringstream ss;
            ss << bossName << " 对你造成 " << dmgToPlayer << " 点伤害。 你的剩余HP=" << m_player.hp;
            if (hasShield) ss << " [护盾]";
            outLog.push_back(ss.str());
        }
        if (m_player.hp <= 0) {
            outLog.push_back(std::string("你被击败了。\n"));
            if (hasShield) m_player.tempShieldCharges--;
            return Fight_PlayerDead;
        }
    }
}

bool Game::saveToFile(const std::string& path) const
{
    std::ofstream ofs(path);
    if (!ofs) return false;

    const auto serializedItemName = [](const Item* item) {
        if (const auto* unknown = dynamic_cast<const UnknownItem*>(item))
            if (!unknown->SourceName().empty()) return unknown->SourceName();
        return item ? item->GetName() : std::string("-");
    };

    ofs << "MOTA2\n";
    ofs << m_width << " " << m_height << "\n";
    ofs << m_floor << "\n";
    ofs << m_floors.size() << "\n";

    for (auto& fp : m_floors) {
        int fnum = fp.first;
        const FloorData& fd = fp.second;
        ofs << fnum << "\n";

        // 地图
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                ofs << fd.map[y*m_width + x] << (x+1 == m_width ? '\n' : ' ');
            }
        }

        // 物品
        ofs << fd.items.size() << "\n";
        for (auto& ikv : fd.items) {
            int key = ikv.first;
            const Item* item = ikv.second.get();
            int x = key % m_width;
            int y = key / m_width;
            ofs << x << " " << y << " " << serializedItemName(item) << " " << item->GetValue() << "\n";
        }

        // 怪物
        ofs << fd.monsters.size() << "\n";
        for (auto& kv : fd.monsters) {
            int key = kv.first;
            const Monster& m = kv.second;
            ofs << key << " " << m.GetName() << " "
                << m.GetHP() << " " << m.GetATK() << " "
                << m.GetDEF() << " " << m.GetGold() << "\n";
        }

        // NPC
        ofs << fd.npcs.size() << "\n";
        for (auto& nkv : fd.npcs) {
            int key = nkv.first;
            const NPC& n = nkv.second;
            int x = key % m_width;
            int y = key / m_width;
            const Item* reward = n.GetReward();
            const Item* tradeReward = n.GetTradeReward();
            ofs << x << " " << y << " " << n.GetName() << " "
                << n.HasGivenReward() << " "
                << n.Dialog().size() << " "
                << serializedItemName(reward) << " "
                << (reward ? reward->GetValue() : 0) << " "
                << n.IsTrader() << " " << n.GetTradeGoldCost() << " "
                << serializedItemName(tradeReward) << " "
                << (tradeReward ? tradeReward->GetValue() : 0) << " "
                << n.IsTradeDone() << " " << n.ClassicId() << "\n";
            for (auto& d : n.Dialog())
                ofs << d << "\n";
        }

    }

    // 商店 (SHOP block)
    size_t totalShops = 0;
    for (auto& fp : m_floors)
        totalShops += fp.second.shops.size();
    ofs << "SHOP\n" << totalShops << "\n";
    for (auto& fp : m_floors) {
        int fnum = fp.first;
        for (auto& skv : fp.second.shops) {
            int x = skv.first % m_width;
            int y = skv.first / m_width;
            const ShopData& s = skv.second;
            ofs << fnum << " " << x << " " << y << " "
                << s.potionPrice << " " << s.weaponPrice << " "
                << s.armorPrice << " "
                << s.potionValue << " " << s.weaponValue << " "
                << s.armorValue << " " << s.classicNpcId << " "
                << s.classicShopFloor << " " << s.classicPurchaseCount << "\n";
        }
    }

    // 玩家
    ofs << m_player.x << " " << m_player.y << " "
        << m_player.hp << " " << m_player.atk << " "
        << m_player.def << " " << m_player.gold << "\n";

    ofs << m_player.KeyCount(KeyType::Red) << " "
        << m_player.KeyCount(KeyType::Blue) << " "
        << m_player.KeyCount(KeyType::Green) << "\n";

    // 特殊物品标记
    ofs << m_player.hasGlasses << " " << m_player.hasPenguinDoll << " "
        << m_player.hasMatchaParfait << "\n";

    // 扩展标记 (v2)
    ofs << "EXTRA " << m_player.hasLuckyCoin << " " << m_player.shopUseCount << " "
        << m_player.wallBreakerUsed << " " << m_player.stairUpUsed << " "
        << m_player.stairDownUsed << " "
        << m_player.magicKeyUses << " " << m_player.tempShieldCharges << " "
        << m_player.hasCross << " " << m_player.hasDragonSlayer << " "
        << m_player.hasHolyShield << " " << m_player.freezeMagicUsed << " "
        << m_player.flyWandUses << " " << m_player.symmetryFlyerUses << " "
        << m_floor10AmbushTriggered << " " << m_floor3PrisonTriggered << " "
        << m_floor3TrapActive << " " << m_floor3PrisonStoryPending << "\n";

    // 背包物品
    ofs << m_player.InventoryCount() << "\n";
    for (int i = 0; i < m_player.InventoryCount(); ++i) {
        auto* item = m_player.GetItem(i);
        if (item) {
            ofs << serializedItemName(item) << " " << item->GetValue() << "\n";
        }
    }

    return true;
}

std::string Game::canonicalItemName(const std::string& iname)
{
    if (iname == "Red Key" || iname == "红钥匙" || iname == "红色Live票") return "红色Live票";
    if (iname == "Blue Key" || iname == "蓝钥匙" || iname == "蓝色Live票") return "蓝色Live票";
    if (iname == "Green Key" || iname == "Yellow Key" || iname == "绿钥匙" || iname == "黄钥匙" || iname == "黄色Live票") return "黄色Live票";
    if (iname == "Potion" || iname == "生命药" || iname == "药水" || iname == "现场补给") return "现场补给";
    if (iname == "Small Potion" || iname == "小血瓶" || iname == "灯的热牛奶") return "灯的热牛奶";
    if (iname == "Large Potion" || iname == "大血瓶" || iname == "爱音能量饮") return "爱音能量饮";
    if (iname == "Ruby Gem" || iname == "红宝石" || iname == "MyGO应援红章") return "MyGO应援红章";
    if (iname == "Sapphire Gem" || iname == "蓝宝石" || iname == "Mujica应援蓝章") return "Mujica应援蓝章";
    if (iname == "Weapon" || iname == QString::fromUtf8("武器").toStdString()) return "Weapon";
    if (iname == "Armor" || iname == QString::fromUtf8("防具").toStdString()) return "Armor";
    if (iname == "Treasure" || iname == QString::fromUtf8("金币").toStdString()) return "Treasure";
    const std::pair<const char*, const char*> originalItems[] = {
        {"Iron Sword", "爱音拨片"}, {"铁剑", "爱音拨片"}, {"Silver Sword", "立希鼓棒"}, {"银剑", "立希鼓棒"},
        {"Knight Sword", "乐奈猫爪"}, {"骑士剑", "乐奈猫爪"}, {"Holy Sword", "灯的麦克风"}, {"圣剑", "灯的麦克风"},
        {"Divine Sword", "睦的贝斯"}, {"神圣剑", "睦的贝斯"},
        {"Iron Shield", "素世谱架"}, {"铁盾", "素世谱架"}, {"Silver Shield", "海铃节拍器"}, {"银盾", "海铃节拍器"},
        {"Knight Shield", "初华舞台耳返"}, {"骑士盾", "初华舞台耳返"}, {"Holy Shield", "祥子黑色乐谱"}, {"圣盾", "祥子黑色乐谱"},
        {"Divine Shield", "Mujica终幕面具"}, {"神圣盾", "Mujica终幕面具"},
        {"Pickaxe", "睦的镐子"}, {"镐", "睦的镐子"}, {"Bomb", "Mujica烟雾弹"}, {"炸弹", "Mujica烟雾弹"},
        {"Earthquake Scroll", "Mujica舞台震响卷"}, {"地震卷轴", "Mujica舞台震响卷"},
        {"Cross", "MyGO和解徽章"}, {"十字架", "MyGO和解徽章"}, {"Dragon Slayer", "祥子指挥棒"}, {"屠龙匕", "祥子指挥棒"},
        {"Freeze Magic", "海铃冷静指令"}, {"冰冻魔法", "海铃冷静指令"}, {"Flying Wand", "爱音手机"}, {"飞行魔杖", "爱音手机"},
        {"Floor Teleporter", "楼层传送器"},
        {"Symmetry Flyer", "Mujica镜面舞台票"}, {"对称飞行器", "Mujica镜面舞台票"}, {"Note Book", "灯的歌词本"}, {"记事本", "灯的歌词本"},
        {"Magic Key", "后台万能通行证"}, {"万能钥匙", "后台万能通行证"}, {"Holy Water", "立希水壶"}, {"圣水", "立希水壶"},
        {"Lucky Coin", "乐奈幸运硬币"}, {"幸运金币", "乐奈幸运硬币"}, {"Anon Glasses", "爱音自拍眼镜"}, {"匿名眼镜", "爱音自拍眼镜"},
        {"Wall Breaker", "破墙锤"}, {"破墙锤", "破墙锤"}, {"Up Flyer", "舞台升降卡"}, {"上楼器", "舞台升降卡"},
        {"Down Flyer", "撤场通行卡"}, {"下楼器", "撤场通行卡"}, {"临时护盾", "乐队护盾贴"},
        {"企鹅玩偶", "立希企鹅挂件"}, {"抹茶芭菲", "乐奈抹茶芭菲"}
    };
    for (const auto& pair : originalItems) {
        if (iname == pair.first || iname == pair.second) return pair.second;
    }
    const QString special = QString::fromStdString(iname);
    static const QStringList specials = {
        QString::fromUtf8("万能钥匙"), QString::fromUtf8("匿名眼镜"), QString::fromUtf8("破墙锤"),
        QString::fromUtf8("上楼器"), QString::fromUtf8("下楼器"), QString::fromUtf8("临时护盾"),
        QString::fromUtf8("企鹅玩偶"), QString::fromUtf8("抹茶芭菲"), QString::fromUtf8("幸运金币"),
        QString::fromUtf8("圣水"), QString::fromUtf8("铁剑"), QString::fromUtf8("银剑"),
        QString::fromUtf8("骑士剑"), QString::fromUtf8("圣剑"), QString::fromUtf8("神圣剑"),
        QString::fromUtf8("铁盾"), QString::fromUtf8("银盾"), QString::fromUtf8("骑士盾"),
        QString::fromUtf8("圣盾"), QString::fromUtf8("神圣盾"), QString::fromUtf8("镐"),
        QString::fromUtf8("炸弹"), QString::fromUtf8("地震卷轴"), QString::fromUtf8("十字架"),
        QString::fromUtf8("屠龙匕"), QString::fromUtf8("冰冻魔法"), QString::fromUtf8("飞行魔杖"),
        QString::fromUtf8("楼层传送器"),
        QString::fromUtf8("对称飞行器"), QString::fromUtf8("记事本")
    };
    for (const auto& name : specials)
        if (special == name) return name.toStdString();
    return {};
}

bool Game::isKnownItemName(const std::string& iname)
{
    return !canonicalItemName(iname).empty();
}

std::unique_ptr<Item> Game::createItemByName(const std::string& iname, int ival) {
    if (iname == "Red Key" || iname == "红钥匙" || iname == "红色Live票")
        return std::make_unique<Key>(KeyType::Red);
    if (iname == "Blue Key" || iname == "蓝钥匙" || iname == "蓝色Live票")
        return std::make_unique<Key>(KeyType::Blue);
    if (iname == "Green Key" || iname == QString::fromUtf8("绿钥匙").toStdString() ||
        iname == "Yellow Key" || iname == "黄钥匙" || iname == "黄色Live票")
        return std::make_unique<Key>(KeyType::Green);
    if (iname == "Potion" || iname == "生命药" || iname == "现场补给")
        return std::make_unique<Potion>(ival, "现场补给");
    if (iname == "Small Potion" || iname == "小血瓶" || iname == "灯的热牛奶")
        return std::make_unique<SmallPotion>(ival, "灯的热牛奶");
    if (iname == "Large Potion" || iname == "大血瓶" || iname == "爱音能量饮")
        return std::make_unique<LargePotion>(ival, "爱音能量饮");
    if (iname == "Ruby Gem" || iname == "红宝石" || iname == "MyGO应援红章")
        return std::make_unique<RubyGem>(ival, "MyGO应援红章");
    if (iname == "Sapphire Gem" || iname == "蓝宝石" || iname == "Mujica应援蓝章")
        return std::make_unique<SapphireGem>(ival, "Mujica应援蓝章");
    if (iname == "Weapon" || iname == QString::fromUtf8("武器").toStdString())
        return std::make_unique<Weapon>(ival);
    if (iname == "Armor" || iname == QString::fromUtf8("防具").toStdString())
        return std::make_unique<Armor>(ival);
    if (iname == "Treasure" || iname == QString::fromUtf8("金币").toStdString())
        return std::make_unique<Treasure>(ival);
    if (iname == "Iron Sword" || iname == "铁剑" || iname == "爱音拨片") return std::make_unique<Weapon>(ival, "爱音拨片");
    if (iname == "Silver Sword" || iname == "银剑" || iname == "立希鼓棒") return std::make_unique<Weapon>(ival, "立希鼓棒");
    if (iname == "Knight Sword" || iname == "骑士剑" || iname == "乐奈猫爪") return std::make_unique<Weapon>(ival, "乐奈猫爪");
    if (iname == "Holy Sword" || iname == "圣剑" || iname == "灯的麦克风") return std::make_unique<Weapon>(ival, "灯的麦克风");
    if (iname == "Divine Sword" || iname == "神圣剑" || iname == "睦的贝斯") return std::make_unique<Weapon>(ival, "睦的贝斯");
    if (iname == "Iron Shield" || iname == "铁盾" || iname == "素世谱架") return std::make_unique<Armor>(ival, "素世谱架");
    if (iname == "Silver Shield" || iname == "银盾" || iname == "海铃节拍器") return std::make_unique<Armor>(ival, "海铃节拍器");
    if (iname == "Knight Shield" || iname == "骑士盾" || iname == "初华舞台耳返") return std::make_unique<Armor>(ival, "初华舞台耳返");
    if (iname == "Holy Shield" || iname == "圣盾" || iname == "祥子黑色乐谱") return std::make_unique<HolyShield>(ival, "祥子黑色乐谱");
    if (iname == "Divine Shield" || iname == "神圣盾" || iname == "Mujica终幕面具") return std::make_unique<DivineShield>(ival, "Mujica终幕面具");
    if (iname == "Pickaxe" || iname == "镐" || iname == "睦的镐子") return std::make_unique<Pickaxe>();
    if (iname == "Bomb" || iname == "炸弹" || iname == "Mujica烟雾弹") return std::make_unique<Bomb>();
    if (iname == "Earthquake Scroll" || iname == "地震卷轴" || iname == "Mujica舞台震响卷") return std::make_unique<EarthquakeScroll>();
    if (iname == "Cross" || iname == "十字架" || iname == "MyGO和解徽章") return std::make_unique<Cross>();
    if (iname == "Dragon Slayer" || iname == "屠龙匕" || iname == "祥子指挥棒") return std::make_unique<DragonSlayer>();
    if (iname == "Freeze Magic" || iname == "冰冻魔法" || iname == "海铃冷静指令") return std::make_unique<FreezeMagic>();
    if (iname == "Flying Wand" || iname == "飞行魔杖" || iname == "爱音手机") return std::make_unique<FlyingWand>();
    if (iname == "Floor Teleporter" || iname == "楼层传送器") return std::make_unique<FloorTeleporter>();
    if (iname == "Symmetry Flyer" || iname == "对称飞行器" || iname == "Mujica镜面舞台票") return std::make_unique<SymmetryFlyer>();
    if (iname == "Note Book" || iname == "记事本" || iname == "灯的歌词本") return std::make_unique<NoteBook>();
    if (iname == "Magic Key" || iname == "万能钥匙" || iname == "后台万能通行证")
        return std::make_unique<MagicKey>();
    if (iname == "Anon Glasses" || iname == "匿名眼镜" || iname == "爱音自拍眼镜")
        return std::make_unique<AnonGlasses>();
    if (iname == "Wall Breaker" || iname == "破墙锤")
        return std::make_unique<WallBreaker>();
    if (iname == "Up Flyer" || iname == "上楼器" || iname == "舞台升降卡")
        return std::make_unique<StairUpper>();
    if (iname == "Down Flyer" || iname == "下楼器" || iname == "撤场通行卡")
        return std::make_unique<StairLower>();
    if (iname == "临时护盾" || iname == "乐队护盾贴")
        return std::make_unique<TempShield>();
    if (iname == "企鹅玩偶" || iname == "立希企鹅挂件")
        return std::make_unique<PenguinDoll>();
    if (iname == "抹茶芭菲" || iname == "乐奈抹茶芭菲")
        return std::make_unique<MatchaParfait>();
    if (iname == "Lucky Coin" || iname == QString::fromUtf8("幸运金币").toStdString())
        return std::make_unique<LuckyCoin>();
    if (iname == "Holy Water" || iname == QString::fromUtf8("圣水").toStdString())
        return std::make_unique<HolyWater>();
    // 未知名称不再静默丢弃，转为无效果占位道具，保证地图/存档数据可见。
    return std::make_unique<UnknownItem>(iname, ival);
}

bool Game::loadFromFile(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs) return false;

    m_floors.clear();

    // 检测版本标记
    std::string version;
    ifs >> version;
    bool isV2 = (version == "MOTA2");
    if (!isV2) {
        // 旧格式: version 就是 m_width
        m_width = std::stoi(version);
        ifs >> m_height;
    } else {
        ifs >> m_width >> m_height;
    }
    ifs >> m_floor;

    size_t floorCount;
    ifs >> floorCount;
    for (size_t fi = 0; fi < floorCount; ++fi) {
        int fnum;
        ifs >> fnum;
        FloorData fd;
        fd.map.assign(m_width * m_height, Tile_Floor);

        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                int v; ifs >> v;
                fd.map[y*m_width + x] = v;
            }
        }

        // 物品
        size_t icount = 0; ifs >> icount;
        for (size_t i = 0; i < icount; ++i) {
            int ix, iy; std::string iname; int ival;
            ifs >> ix >> iy >> iname >> ival;
            int key = iy * m_width + ix;
            auto item = createItemByName(iname, ival);
            if (item) fd.items[key] = std::move(item);
        }

        // 怪物
        size_t mcount = 0; ifs >> mcount;
        for (size_t i = 0; i < mcount; ++i) {
            int key; std::string name; int hp, atk, def, gold;
            ifs >> key >> name >> hp >> atk >> def >> gold;
            fd.monsters.emplace(key, Monster(name, hp, atk, def, gold));
        }

        // NPC
        size_t ncount = 0; ifs >> ncount;
        for (size_t i = 0; i < ncount; ++i) {
            int nx, ny; std::string nname; bool given; size_t dsize;
            std::string rewardName; int rewardValue;
            ifs >> nx >> ny >> nname >> given >> dsize >> rewardName >> rewardValue;
            // 交易字段 (v2 扩展, 可选)
            bool isTrader = false; int tradeGoldCost = 0;
            std::string tradeRewardName = "-"; int tradeRewardValue = 0;
            bool tradeDone = false;
            if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF) {
                ifs >> isTrader >> tradeGoldCost >> tradeRewardName >> tradeRewardValue;
                // tradeDone 是可选的第五个字段（旧格式没有）
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> tradeDone;
            }
            int classicId = 0;
            if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                ifs >> classicId;
            ifs.ignore();
            std::vector<std::string> dialog;
            for (size_t d = 0; d < dsize; ++d) {
                std::string line;
                std::getline(ifs, line);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                dialog.push_back(line);
            }
            int key = ny * m_width + nx;
            auto reward = (rewardName == "-") ? nullptr : createItemByName(rewardName, rewardValue);
            auto tradeReward = (tradeRewardName == "-") ? nullptr : createItemByName(tradeRewardName, tradeRewardValue);
            auto npc = NPC(nname, dialog, std::move(reward), isTrader, tradeGoldCost, std::move(tradeReward), classicId);
            npc.SetGiven(given);
            npc.SetTradeDone(tradeDone);
            fd.npcs.emplace(key, std::move(npc));
        }

        // 商店 (仅旧格式 per-floor)
        if (!isV2) {
            size_t scount = 0; ifs >> scount;
            for (size_t i = 0; i < scount; ++i) {
                int sx, sy, pp, wp, ap, pv = 200, wv = 5, av = 8;
                ifs >> sx >> sy >> pp >> wp >> ap;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> pv >> wv >> av;
                int key = sy * m_width + sx;
                ShopData shop{pp, wp, ap, pv, wv, av};
                fd.shops.emplace(key, shop);
            }
        }

        m_floors[fnum] = std::move(fd);
    }

    // 玩家
    int px = 0, py = 0, php = 100, patk = 10, pdef = 5, pgold = 0;

    // 商店 (SHOP 标记)
    std::string marker;
    ifs >> marker;
    if (marker == "SHOP") {
        size_t totalShops = 0; ifs >> totalShops;
        for (size_t i = 0; i < totalShops; ++i) {
            int fnum, sx, sy, pp, wp, ap, pv = 200, wv = 5, av = 8, classicNpcId = 0, classicShopFloor = 0, classicPurchaseCount = 0;
            ifs >> fnum >> sx >> sy >> pp >> wp >> ap;
            if (ifs.peek() != '\n' && ifs.peek() != EOF)
                ifs >> pv >> wv >> av;
            if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                ifs >> classicNpcId;
            if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                ifs >> classicShopFloor >> classicPurchaseCount;
            auto it = m_floors.find(fnum);
            if (it != m_floors.end()) {
                ShopData shop{pp, wp, ap, pv, wv, av};
                shop.classicNpcId = classicNpcId;
                shop.classicShopFloor = classicShopFloor;
                shop.classicPurchaseCount = classicPurchaseCount;
                it->second.shops.emplace(sy * m_width + sx, shop);
            }
        }
        // 读取玩家数据
        ifs >> px >> py >> php >> patk >> pdef >> pgold;
    } else {
        // 旧格式: marker 就是 player x
        px = std::stoi(marker);
        ifs >> py >> php >> patk >> pdef >> pgold;
    }

    int r, b, g; ifs >> r >> b >> g;
    bool hasGl, hasPen, hasMat;
    ifs >> hasGl >> hasPen >> hasMat;

    // 扩展标记 (v2, 可选)
    bool hasLc = false; int mkUses = 0, shopUse = 0, tsc = 0;
    bool wbUsed = false, suUsed = false, sdUsed = false;
    bool hasCross = false, hasDragonSlayer = false, hasHolyShield = false, freezeMagicUsed = false;
    int flyWandUses = 0, symmetryFlyerUses = 0;
    bool floor10AmbushTriggered = false;
    bool floor3PrisonTriggered = false;
    bool floor3TrapActive = false;
    bool floor3PrisonStoryPending = false;
    std::string invToken;
    ifs >> invToken;
    if (invToken == "EXTRA") {
        ifs >> hasLc >> shopUse >> wbUsed >> suUsed >> sdUsed;
        // v3 扩展: 万能钥匙 + 临时护盾次数
        if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF) {
            ifs >> mkUses >> tsc;
            if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF) {
                ifs >> hasCross >> hasDragonSlayer >> hasHolyShield >> freezeMagicUsed
                    >> flyWandUses >> symmetryFlyerUses;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor10AmbushTriggered;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor3PrisonTriggered;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor3TrapActive;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor3PrisonStoryPending;
            }
        }
        ifs >> invToken; // 下一个是背包数量
    }
    // invToken 现在是背包数量（或旧格式直接读取的数字）
    int invCount = std::stoi(invToken);

    // 重置并恢复玩家数据
    m_player = Player();
    m_player.x = px; m_player.y = py;
    m_player.hp = php; m_player.atk = patk;
    m_player.def = pdef; m_player.gold = pgold;
    for (int i = 0; i < r; ++i) m_player.AddKey(KeyType::Red, 1);
    for (int i = 0; i < b; ++i) m_player.AddKey(KeyType::Blue, 1);
    for (int i = 0; i < g; ++i) m_player.AddKey(KeyType::Green, 1);
    m_player.hasGlasses = hasGl;
    m_player.hasPenguinDoll = hasPen;
    m_player.hasMatchaParfait = hasMat;
    m_player.hasLuckyCoin = hasLc;
    m_player.shopUseCount = shopUse;
    m_player.wallBreakerUsed = wbUsed;
    m_player.stairUpUsed = suUsed;
    m_player.stairDownUsed = sdUsed;
    m_player.magicKeyUses = mkUses;
    m_player.tempShieldCharges = tsc;
    m_player.hasCross = hasCross;
    m_player.hasDragonSlayer = hasDragonSlayer;
    m_player.hasHolyShield = hasHolyShield;
    m_player.freezeMagicUsed = freezeMagicUsed;
    m_player.flyWandUses = flyWandUses;
    m_player.symmetryFlyerUses = symmetryFlyerUses;
    m_floor10AmbushTriggered = floor10AmbushTriggered;
    m_floor3PrisonTriggered = floor3PrisonTriggered;
    m_floor3PrisonStoryPending = floor3PrisonStoryPending;
    m_floor3TrapActive = floor3TrapActive;
    m_floor10AmbushMonsterKeys.clear();
    m_floor10AmbushDoorKeys.clear();
    m_floor10AmbushMovements.clear();

    for (int i = 0; i < invCount; ++i) {
        std::string iname; int ival;
        ifs >> iname >> ival;
        auto item = createItemByName(iname, ival);
        if (item) m_player.AddItem(std::move(item));
    }

    m_currentFloor = &m_floors[m_floor];
    return true;
}
