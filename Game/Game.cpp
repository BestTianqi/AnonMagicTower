#include "Game.h"
#include "Entities/MonsterDB.h"
#include <QString>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <array>
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
    // 48层圣剑房花门由左上角藤都子SP的专属事件开启，
    // 不参与通用机关门清怪判定。
    if (floorNumber == 48) return false;
    if (floorNumber == 38 && doorX == 3 && doorY == 6) {
        // 38层事件花门是剧情锁，不允许用钥匙、镐或普通清怪逻辑打开。
        return false;
    }
    if (floorNumber == 20 && ((doorX == 7 && doorY == 10) || (doorX == 7 && doorY == 4))) {
        // 20层剧情触发后，两扇花门都绑定中央吸血鬼。
        return floor.monsters.find(7 * MAP_SIZE + 7) == floor.monsters.end();
    }
    if (floorNumber == 33 && doorX == 11 && (doorY == 5 || doorY == 9)) {
        // 33层两扇花门共用四个斜角守卫；四只全部击败后同时开启。
        static const int guards[] = {
            6 * MAP_SIZE + 10, 6 * MAP_SIZE + 12,
            8 * MAP_SIZE + 10, 8 * MAP_SIZE + 12
        };
        for (const int key : guards)
            if (floor.monsters.find(key) != floor.monsters.end()) return false;
        return true;
    }

    if (floorNumber == 15 && doorX == 7 && doorY == 4) {
        // 15层中央花门绑定大章鱼（原版15号怪物）；
        // 只有击败它后才允许打开，不能被其他清怪组提前解锁。
        return !hasClassicMonsterId(floor, 15);
    }

    if (floorNumber == 30 && floor.map[doorY * MAP_SIZE + doorX] == Tile_DoorMagic) {
        // 30层花门绑定整层怪物，不能按花门周围八格或旧编号组提前开启。
        return floor.monsters.empty();
    }

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
    m_lastTeleportPath.clear();
    m_pendingTeleportPath.clear();
    m_pendingTeleportTargetX = -1;
    m_pendingTeleportTargetY = -1;
    m_visitedFloors.clear();
    m_visitedFloors.insert(1);
    m_floor = 1;
    m_floor3PrisonTriggered = false;
    m_floor3PrisonStoryPending = false;
    m_floor3TrapActive = false;
    m_princessDollRescued = false;
    m_floor20VampireTriggered = false;
    m_floor20VampireStoryShown = false;
    m_floor14RedKeyRewardGranted = false;
    m_floor32KnightTriggered = false;
    m_floor32KnightStoryPending = false;
    m_floor42KnightStoryTriggered = false;
    m_floor42KnightStoryPending = false;
    m_floor34RewardGranted = false;
    m_floor35RewardsHidden = false;
    m_floor40BossDefeated = false;
    m_floor40RewardsGranted = false;
    m_floor32KnightMovements.clear();
    m_floor33TrapTriggered = false;
    m_michelleRescued = false;
    m_michelleGuardDefeated = false;
    m_floor38FlowerTriggered = false;
    m_floor43MiyakoRetreated = false;
    m_storyOnceKeys.clear();
    m_floor10AmbushTriggered = false;
    m_floor10AmbushMonsterKeys.clear();
    m_floor10AmbushDoorKeys.clear();
    m_floor10AmbushMovements.clear();
    // 原版 0 层不是普通塔层：使用 1 层取得的下楼器后抵达此处，
    // 房间中央放置幸运金币。保留完整墙圈，便于正常移动和管理员调试。
    FloorData luckyCoinFloor;
    luckyCoinFloor.map.assign(m_width * m_height, Tile_Wall);
    for (int y = 2; y <= 12; ++y)
        for (int x = 2; x <= 12; ++x)
            luckyCoinFloor.map[y * m_width + x] = Tile_Floor;
    const int luckyCoinKey = 7 * m_width + 7;
    luckyCoinFloor.map[2 * m_width + 7] = Tile_StairsUp;
    luckyCoinFloor.items[luckyCoinKey] = std::make_unique<LuckyCoin>();
    luckyCoinFloor.map[luckyCoinKey] = Tile_Item;
    m_floors.emplace(0, std::move(luckyCoinFloor));

    for (int floor = 1; floor <= 50; ++floor) {
        FloorData fd;
        fd.map.assign(m_width * m_height, Tile_Wall);
        for (int y = 2; y <= 12; ++y)
            for (int x = 2; x <= 12; ++x)
                fd.map[y * m_width + x] = Tile_Floor;
        m_floors.emplace(floor, std::move(fd));
    }

    // 即使经典地图资源暂时不可用（例如无 Qt 资源的逻辑测试目标），
    // 41层下楼器事件仍保持可验证的最小布局。
    const auto ensureFloor41DownstairsLayout = [this]() {
        FloorData& fd = m_floors[41];
        // 原版41层的右上对称暗墙位于(11,3)，墙内藏着第二只高级巫师。
        const int darkWallKey = posKey(11, 3);
        const int wizardKey = posKey(3, 3);
        // 无论静态资源原先是墙还是地板，都强制恢复右上暗墙；
        // 左上(3,3)保留可见的首只高级巫师。
        if (fd.monsters.find(darkWallKey) == fd.monsters.end())
            fd.map[darkWallKey] = Tile_DarkWall;
        if (fd.monsters.find(wizardKey) == fd.monsters.end()) {
            fd.monsters.emplace(wizardKey, MonsterDB::getByIndex(26));
            fd.map[wizardKey] = Tile_Monster;
        }
    };

    // 逻辑测试和无 Qt 资源运行时仍需保留经典塔的关键事件布局。
    // 正式资源加载完成后下面的完整脚本会再次校正这些格子。
    const auto ensureClassicEventLayouts = [this]() {
        auto& floor19 = m_floors[19];
        const int crossKey = posKey(7, 4);
        if (floor19.items.find(crossKey) == floor19.items.end()) {
            floor19.items[crossKey] = std::make_unique<Cross>();
            floor19.map[crossKey] = Tile_Item;
        }
        auto& floor20 = m_floors[20];
        const int vampireKey = posKey(7, 7);
        // 吸血鬼由(7,9)剧情点显现，初始地图不放置该Boss。
        floor20.monsters.erase(vampireKey);
        if (floor20.map[vampireKey] == Tile_Monster) floor20.map[vampireKey] = Tile_Floor;
        floor20.map[posKey(7, 10)] = Tile_DoorRed;
        // 20层顶部黄色上楼梯是击败吸血鬼后的奖励，底部紫色小楼梯始终保留。
        floor20.map[posKey(7, 2)] = Tile_Floor;
        floor20.map[posKey(7, 12)] = Tile_StairsDown;

        auto& floor15 = m_floors[15];
        floor15.map[posKey(9, 2)] = Tile_DarkWall;
        floor15.map[posKey(7, 4)] = Tile_DoorMagic;
        const int octopusKey = posKey(7, 6);
        floor15.monsters[octopusKey] = MonsterDB::getByIndex(14);
        floor15.map[octopusKey] = Tile_Monster;
        m_floors[35].map[posKey(7, 4)] = Tile_DarkWall;
        auto& floor35 = m_floors[35];
        // 35层薇欧拉SP·魔龙固定站在(7,5)；清理旧版资源可能遗留的(7,7)事件格。
        const int dragonKey = posKey(7, 5);
        floor35.monsters.erase(posKey(7, 7));
        floor35.monsters[dragonKey] = MonsterDB::get("薇欧拉SP·魔龙");
        floor35.map[posKey(7, 7)] = floor35.items.count(posKey(7, 7)) ? Tile_Item : Tile_Floor;
        floor35.map[dragonKey] = Tile_Monster;
        auto& floor28 = m_floors[28];
        const int merchantKey = posKey(9, 5);
        floor28.map[merchantKey] = Tile_Shop;
        floor28.shops[merchantKey] = ShopData{};
        floor28.shops[merchantKey].classicNpcId = 24;
        floor28.shops[merchantKey].classicShopFloor = 28;

        // 29层米歇尔脚下的原版暗墙，完成对话后才会消失。
        m_floors[29].map[posKey(7, 4)] = Tile_Wall;

        // 无 Qt 地图资源的逻辑测试/后备地图也保留二层两名中级守卫，
        // 以便牢笼救出前置条件与正式地图一致。
        for (const auto& guard : std::array<std::pair<int, int>, 2>{{{7, 3}, {9, 3}}}) {
            const int key = posKey(guard.first, guard.second);
            m_floors[2].monsters[key] = MonsterDB::getByIndex(20);
            m_floors[2].map[key] = Tile_Monster;
        }

        // 38层(3,7)为事件触发格；花门只有剧情创建后才出现。
        m_floors[38].map[posKey(3, 7)] = Tile_Floor;
        m_floors[38].map[posKey(3, 6)] = Tile_Floor;

        // 43层藤都子SP的初始站位（资源缺失时也保持事件可测试）。
        const int miyakoKey = posKey(10, 2);
        m_floors[43].monsters[miyakoKey] = MonsterDB::get("藤都子SP·魔法警卫");
        m_floors[43].map[miyakoKey] = Tile_Monster;

        // 无资源测试与正式地图必须共享同一组关键地形校正。
        const int floor14RewardKey = posKey(2, 4);
        m_floors[14].items.erase(floor14RewardKey);
        m_floors[14].map[floor14RewardKey] = Tile_Wall;
        m_floors[32].map[posKey(6, 12)] = Tile_StairsDown;
        m_floors[32].map[posKey(12, 2)] = Tile_StairsUp;
        for (const auto& wall : std::array<std::pair<int, int>, 2>{{{10, 11}, {12, 11}}}) {
            const int key = posKey(wall.first, wall.second);
            m_floors[32].monsters.erase(key);
            m_floors[32].items.erase(key);
            m_floors[32].npcs.erase(key);
            m_floors[32].shops.erase(key);
            m_floors[32].map[key] = Tile_Wall;
        }
        for (const auto& wall : std::array<std::pair<int, int>, 12>{{
                 {3, 6}, {4, 6}, {5, 6}, {9, 6}, {10, 6}, {11, 6},
                 {3, 8}, {4, 8}, {5, 8}, {9, 8}, {10, 8}, {11, 8}}})
            m_floors[36].map[posKey(wall.first, wall.second)] = Tile_DarkWall;
    };

    QFile source(":/data/classic50_map.txt");
    if (!source.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ensureClassicEventLayouts();
        ensureFloor41DownstairsLayout();
        m_currentFloor = &m_floors[m_floor];
        m_visitedFloors.insert(m_floor);
        m_player = Player();
        m_player.x = 7; m_player.y = 12;
        m_player.hp = 1000; m_player.atk = 100; m_player.def = 100;
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
        ensureClassicEventLayouts();
        ensureFloor41DownstairsLayout();
        m_currentFloor = &m_floors[m_floor];
        m_player = Player();
        m_player.x = 7; m_player.y = 12;
        m_player.hp = 1000; m_player.atk = 100; m_player.def = 100;
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
            // 40层 Boss 区入口原本标为花门，按当前流程改为红门，
            // 由红钥匙开启，不再参与花门的清怪判定。
            // 40层上方原先的第二扇红门已移除；id=3 的下方红门保留。
            else if (id == 30 && level == 40) tile = Tile_Floor;
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
                                  id == 38 || id == 41 || id == 43 || id == 44 || id == 24;
            if (merchant) {
                fd.map[key] = Tile_Shop;
                ShopData shop{20 + level * 10, 20 + level * 10, 20 + level * 10,
                              100 + level * 5, 2 + level / 10, 2 + level / 10};
                shop.classicNpcId = id;
                shop.classicShopFloor = id == 15 ? 4 : id == 16 ? 12 : id == 24 ? 28 : id == 28 ? 32 : id == 43 ? 46 : 0;
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
                case 37:
                    dialog = (level == 39)
                        ? std::vector<std::string>{"打开左上房间12点和3点方向的两扇黄门，镜面舞台票就会出现。"}
                        : std::vector<std::string>{"翡翠剑的房间需要用镐破墙进入。"};
                    break;
                case 39: dialog = {"通往异界的入口就在不远处。"}; break;
                case 40: dialog = {"44层被藏在异界，只有通过秘宝才能到达。"}; break;
                case 42: dialog = {"神圣盾能免疫魔法攻击，但它被藏在异界内。"}; break;
                case 45: dialog = {"要打败魔龙必须准备神圣剑、神圣盾或屠龙匕首。"}; break;
                default: break;
                }
                std::unique_ptr<Item> reward;
                if (id == 3) reward = std::make_unique<MonsterBook>();
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

    ensureFloor41DownstairsLayout();

    auto spawnEventMonster = [this](int level, int sourceX, int sourceY, int monsterId) {
        FloorData& fd = m_floors[level];
        const int x = sourceX + 7;
        const int y = 7 - sourceY;
        const int key = y * m_width + x;
        fd.monsters[key] = MonsterDB::getByIndex(monsterId - 1);
        fd.map[key] = Tile_Monster;
    };

    // 原版脚本动态生成、因而不在静态 mapinfo 中的关键战斗。
    // 20层吸血鬼只有走到(7,9)剧情点后才出现，初始不能在地图上看到。
    // 20层吸血鬼房入口初始为红门，开门后才转换为花门。
    m_floors[20].map[(7 - (-3)) * m_width + (0 + 7)] = Tile_DoorRed;
    // 顶部黄色上楼梯由击败吸血鬼后生成，底部紫色小楼梯不参与奖励替换。
    m_floors[20].map[posKey(7, 2)] = Tile_Floor;
    m_floors[20].map[posKey(7, 12)] = Tile_StairsDown;
    // 20层上方的第二扇花门，和下方入口门各自绑定同一场吸血鬼事件。
    m_floors[20].map[posKey(7, 4)] = Tile_DoorMagic;
    // 19层十字架与35层魔龙是经典流程必备节点；资源缺失时补回默认位置。
    const int crossKey = posKey(7, 4);
    if (m_floors[19].items.find(crossKey) == m_floors[19].items.end()) {
        m_floors[19].items[crossKey] = std::make_unique<Cross>();
        m_floors[19].map[crossKey] = Tile_Item;
    }
    const int dragonKey = posKey(7, 5);
    m_floors[35].monsters.erase(posKey(7, 7));
    m_floors[35].monsters[dragonKey] = MonsterDB::get("薇欧拉SP·魔龙");
    m_floors[35].map[posKey(7, 7)] = m_floors[35].items.count(posKey(7, 7)) ? Tile_Item : Tile_Floor;
    m_floors[35].map[dragonKey] = Tile_Monster;

    // 15层米歇尔左侧的原版暗墙。静态资源把这里标成普通墙，
    // 导致对话后的“打开左边墙”逻辑找不到目标格。
    m_floors[15].map[posKey(9, 2)] = Tile_DarkWall;
    // 35层米歇尔剧情对应的是暗墙，不是花门；完成对话后才变为地板。
    m_floors[35].map[posKey(7, 4)] = Tile_DarkWall;
    // 35层魔龙附近的地面奖励在魔龙被击败前不可见；保留名称和值，击败后恢复原位。
    m_floor35HiddenItems.clear();
    for (auto it = m_floors[35].items.begin(); it != m_floors[35].items.end();) {
        const int x = it->first % m_width;
        const int y = it->first / m_width;
        if (std::abs(x - 7) <= 2 && std::abs(y - 5) <= 2) {
            m_floor35HiddenItems[it->first] = {it->second->GetName(), it->second->GetValue()};
            m_floors[35].map[it->first] = Tile_Floor;
            it = m_floors[35].items.erase(it);
        } else ++it;
    }
    m_floor35RewardsHidden = !m_floor35HiddenItems.empty();
    // 35层米歇尔在救出二楼牢笼中的米歇尔后由 activateFloor35Michelle() 生成；
    // 龙房间地上的奖励则在击败魔龙后才显现。
    // 14层左上奖励格在三只指定怪物被击败前是一面普通墙。
    const int floor14RewardKey = posKey(2, 4);
    m_floors[14].items.erase(floor14RewardKey);
    m_floors[14].map[floor14RewardKey] = Tile_Wall;

    // 32层保留原版两个楼梯：底部(6,12)为紫色下楼梯，右上(12,2)
    // 为黄色上楼梯。剧情触发点两侧的(10,11)、(12,11)固定为墙。
    m_floors[32].map[posKey(6, 12)] = Tile_StairsDown;
    m_floors[32].map[posKey(12, 2)] = Tile_StairsUp;
    for (const auto& wall : std::array<std::pair<int, int>, 2>{{{10, 11}, {12, 11}}}) {
        const int key = posKey(wall.first, wall.second);
        m_floors[32].monsters.erase(key);
        m_floors[32].items.erase(key);
        m_floors[32].npcs.erase(key);
        m_floors[32].shops.erase(key);
        m_floors[32].map[key] = Tile_Wall;
    }

    // 原版36层的四条对称暗道：左右两侧、上下各一条，每条三格。
    // 导入资源把这些格子误标为普通墙，因此在脚本层恢复为暗墙。
    const std::array<std::pair<int, int>, 12> floor36DarkWalls{{
        {3, 6}, {4, 6}, {5, 6}, {9, 6}, {10, 6}, {11, 6},
        {3, 8}, {4, 8}, {5, 8}, {9, 8}, {10, 8}, {11, 8}
    }};
    for (const auto& wall : floor36DarkWalls)
        m_floors[36].map[posKey(wall.first, wall.second)] = Tile_DarkWall;
    // 48层左上角的藤都子SP是开启圣剑房花门的专属守卫。
    spawnEventMonster(48, -5, 5, 31);
    // 38层剧情触发格与43层藤都子SP退场事件，即使地图资源已有同名格也以脚本为准。
    m_floors[38].map[posKey(3, 7)] = Tile_Floor;
    m_floors[38].map[posKey(3, 6)] = Tile_Floor;
    const int miyako43Key = posKey(10, 2);
    m_floors[43].monsters[miyako43Key] = MonsterDB::get("藤都子SP·魔法警卫");
    m_floors[43].map[miyako43Key] = Tile_Monster;
    // 49层假魔王一开始就出现，但处于未封印的满属性状态；
    // 只有击败其上下左右四名魔法警卫后，封印才会生效并削弱魔王。
    spawnEventMonster(49, 0, 3, 33);
    const int guardPositions[][2] = {
        {-1, 4}, {0, 4}, {1, 4}, {-1, 3}, {1, 3}, {-1, 2}, {0, 2}, {1, 2}
    };
    for (const auto& point : guardPositions)
        spawnEventMonster(49, point[0], point[1], 31);

    prepareFloor3PrisonCell();
    m_currentFloor = &m_floors[m_floor];
    m_visitedFloors.insert(m_floor);
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
        if (floor == 0) {
            const int luckyCoinKey = 7 * m_width + 7;
            fd.items[luckyCoinKey] = std::make_unique<LuckyCoin>();
            fd.map[luckyCoinKey] = Tile_Item;
            fd.map[2 * m_width + 7] = Tile_StairsUp;
        }
        m_floors[floor] = std::move(fd);
    }
    m_currentFloor = &m_floors[floor];
    prepareFloor3PrisonCell();
}

bool Game::loadDefaultMap()
{
    m_lastTeleportPath.clear();
    m_pendingTeleportPath.clear();
    m_pendingTeleportTargetX = -1;
    m_pendingTeleportTargetY = -1;
    m_visitedFloors.clear();
    m_visitedFloors.insert(1);
    m_floor3PrisonTriggered = false;
    m_floor3PrisonStoryPending = false;
    m_floor3TrapActive = false;
    m_princessDollRescued = false;
    m_floor20VampireTriggered = false;
    m_floor20VampireStoryShown = false;
    m_floor14RedKeyRewardGranted = false;
    m_floor32KnightTriggered = false;
    m_floor32KnightStoryPending = false;
    m_floor42KnightStoryTriggered = false;
    m_floor42KnightStoryPending = false;
    m_floor34RewardGranted = false;
    m_floor35RewardsHidden = false;
    m_floor40BossDefeated = false;
    m_floor40RewardsGranted = false;
    m_floor32KnightMovements.clear();
    m_floor33TrapTriggered = false;
    m_michelleRescued = false;
    m_michelleGuardDefeated = false;
    m_floor38FlowerTriggered = false;
    m_floor43MiyakoRetreated = false;
    m_storyOnceKeys.clear();
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

std::vector<std::pair<int, int>> Game::findTeleportPath(int targetX, int targetY) const
{
    std::vector<std::pair<int, int>> noPath;
    if (targetX < 0 || targetY < 0 || targetX >= m_width || targetY >= m_height)
        return noPath;
    if (targetX == m_player.x && targetY == m_player.y)
        return {{m_player.x, m_player.y}};

    const auto walkable = [](int tile) {
        return tile == Tile_Floor || tile == Tile_Item ||
               tile == Tile_StairsUp || tile == Tile_StairsDown;
    };
    const int targetTile = tileAt(targetX, targetY);
    const bool targetInteractable = targetTile == Tile_NPC || targetTile == Tile_Shop ||
                                    (targetTile == Tile_Monster && hasMonsterAt(targetX, targetY));
    const bool princessPassageDoorLocked =
        (m_floor == 24 && targetX == 7 && targetY == 9 && !m_princessDollRescued);
    const bool targetDoorWithKey =
        (!princessPassageDoorLocked && targetTile == Tile_DoorRed && m_player.HasKey(KeyType::Red)) ||
        (targetTile == Tile_DoorBlue && m_player.HasKey(KeyType::Blue)) ||
        (targetTile == Tile_DoorGreen && (m_player.HasKey(KeyType::Green) || m_player.magicKeyUses > 0));
    const bool targetMechanismDoor =
        (targetTile == Tile_DoorMagic || targetTile == Tile_DoorIron) &&
        ((m_floor == 48 && m_player.wallBreakerUsed) ||
         (m_currentFloor && mechanismDoorReadyAt(m_floor, *m_currentFloor, targetX, targetY)));
    if (!walkable(targetTile) && !targetInteractable && !targetDoorWithKey && !targetMechanismDoor)
        return noPath;
    const int currentTile = tileAt(m_player.x, m_player.y);
    const bool currentInteractable =
        currentTile == Tile_NPC || currentTile == Tile_Shop ||
        (currentTile == Tile_Monster && hasMonsterAt(m_player.x, m_player.y));
    // 玩家可能正站在刚瞬移到的 NPC/商店格，仍应以该格为 BFS 起点。
    if (!walkable(currentTile) && !currentInteractable) return noPath;

    std::vector<unsigned char> visited(static_cast<size_t>(m_width * m_height), 0);
    std::queue<std::pair<int, int>> pending;
    std::vector<std::pair<int, int>> parent(static_cast<size_t>(m_width * m_height), {-1, -1});
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
            if (nx == targetX && ny == targetY) {
                parent[index(nx, ny)] = {x, y};
                std::vector<std::pair<int, int>> path;
                std::pair<int, int> cursor{targetX, targetY};
                while (cursor.first >= 0) {
                    path.push_back(cursor);
                    if (cursor.first == m_player.x && cursor.second == m_player.y) break;
                    cursor = parent[index(cursor.first, cursor.second)];
                }
                if (path.back() != std::make_pair(m_player.x, m_player.y)) return noPath;
                std::reverse(path.begin(), path.end());
                return path;
            }
            const int cell = index(nx, ny);
            if (visited[cell] || !walkable(tileAt(nx, ny))) continue;
            visited[cell] = 1;
            parent[cell] = {x, y};
            pending.emplace(nx, ny);
        }
    }
    return noPath;
}

bool Game::isTeleportReachable(int targetX, int targetY) const
{
    return !findTeleportPath(targetX, targetY).empty();
}

bool Game::canUsePhone() const
{
    if (!m_currentFloor || m_floor <= 0 || m_floor == 44 || m_floor == 50)
        return false;
    const auto walkable = [this](int x, int y) {
        const int tile = tileAt(x, y);
        if (tile == Tile_Floor || tile == Tile_Item ||
            tile == Tile_StairsUp || tile == Tile_StairsDown)
            return true;
        if (tile == Tile_DoorRed) return m_player.HasKey(KeyType::Red);
        if (tile == Tile_DoorBlue) return m_player.HasKey(KeyType::Blue);
        if (tile == Tile_DoorGreen)
            return m_player.HasKey(KeyType::Green) || m_player.magicKeyUses > 0;
        if (tile == Tile_DoorMagic || tile == Tile_DoorIron)
            return mechanismDoorReadyAt(m_floor, *m_currentFloor, x, y);
        return false;
    };
    if (!walkable(m_player.x, m_player.y)) return false;
    std::vector<unsigned char> seen(static_cast<size_t>(m_width * m_height), 0);
    std::queue<std::pair<int, int>> pending;
    const auto index = [this](int x, int y) { return y * m_width + x; };
    pending.emplace(m_player.x, m_player.y);
    seen[index(m_player.x, m_player.y)] = 1;
    static constexpr int directions[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!pending.empty()) {
        const auto [x, y] = pending.front();
        pending.pop();
        if (tileAt(x, y) == Tile_StairsUp || tileAt(x, y) == Tile_StairsDown)
            return true;
        for (const auto& direction : directions) {
            const int nx = x + direction[0], ny = y + direction[1];
            if (nx < 0 || ny < 0 || nx >= m_width || ny >= m_height) continue;
            const int cell = index(nx, ny);
            if (seen[cell] || !walkable(nx, ny)) continue;
            seen[cell] = 1;
            pending.emplace(nx, ny);
        }
    }
    return false;
}

bool Game::phoneTeleportToFloor(int targetFloor)
{
    bool hasPhone = false;
    for (int i = 0; i < m_player.InventoryCount(); ++i)
        if (dynamic_cast<const FlyingWand*>(m_player.GetItem(i)) != nullptr) {
            hasPhone = true;
            break;
        }
    if (!canUsePhone() || targetFloor <= 0 || targetFloor > 50 ||
        targetFloor == 44 || targetFloor == 50 || !hasVisitedFloor(targetFloor) || !hasPhone)
        return false;
    if (m_floors.find(targetFloor) == m_floors.end()) initFloor(targetFloor);
    m_floor = targetFloor;
    m_currentFloor = &m_floors[m_floor];
    m_visitedFloors.insert(m_floor);
    prepareFloor3PrisonCell();

    // 爱音手机固定落在目标层紫色下楼梯；若该层没有紫色楼梯则退回黄色上楼梯。
    int stairX = -1, stairY = -1;
    for (int y = 0; y < m_height && stairX < 0; ++y) {
        for (int x = 0; x < m_width; ++x) {
            if (tileAt(x, y) == Tile_StairsDown) {
                stairX = x; stairY = y; break;
            }
        }
    }
    if (stairX < 0) {
        for (int y = 0; y < m_height && stairX < 0; ++y)
            for (int x = 0; x < m_width; ++x)
                if (tileAt(x, y) == Tile_StairsUp) { stairX = x; stairY = y; break; }
    }
    if (stairX < 0) return false;
    m_player.x = stairX;
    m_player.y = stairY;
    return true;
}

bool Game::isTeleportStoryPoint(int x, int y) const
{
    if (!m_currentFloor) return false;
    const int tile = tileAt(x, y);
    if (tile == Tile_NPC) return true;
    return (m_floor == 3 && !m_floor3PrisonTriggered && x == 6 && y == 9) ||
           (m_floor == 10 && !m_floor10AmbushTriggered && x == 7 && y == 6) ||
           (m_floor == 38 && !m_floor38FlowerTriggered && x == 3 && y == 7);
}

int Game::applyApproachHazardsAt(int x, int y)
{
    if (!m_currentFloor) return 0;
    // 33层(9,11)是伪装地板：玩家走到相邻格时才显现为墙。
    if (m_floor == 33 && tileAt(9, 11) == Tile_Floor &&
        std::abs(x - 9) + std::abs(y - 11) == 1)
        setTile(9, 11, Tile_Wall);
    if (m_player.hasHolyShield || m_floor3PrisonStoryPending) return 0;

    int mageDamage = 0;
    static constexpr int directions[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (const auto& direction : directions) {
        const Monster* nearby = monsterAt(x + direction[0], y + direction[1]);
        if (!nearby) continue;
        const std::string& name = nearby->GetName();
        if (name.find("高级巫师") != std::string::npos)
            mageDamage += 200;
        else if (name.find("初级巫师") != std::string::npos)
            mageDamage += 100;
    }

    const auto isMagicGuard = [this](int gx, int gy) {
        const Monster* guard = monsterAt(gx, gy);
        return guard && guard->GetName().find("魔法警卫") != std::string::npos;
    };
    const bool horizontalPair = isMagicGuard(x - 1, y) && isMagicGuard(x + 1, y);
    const bool verticalPair = isMagicGuard(x, y - 1) && isMagicGuard(x, y + 1);
    const int beforeDamage = m_player.hp;
    if (mageDamage > 0) m_player.hp = std::max(0, m_player.hp - mageDamage);
    const int beforeHalving = m_player.hp;
    if (horizontalPair || verticalPair) m_player.hp /= 2;
    const int lost = (beforeDamage - m_player.hp);
    return lost;
}

std::vector<std::pair<int, int>> Game::takeLastTeleportPath()
{
    auto path = std::move(m_lastTeleportPath);
    m_lastTeleportPath.clear();
    m_lastTeleportNeedsAnimation = false;
    return path;
}

bool Game::beginTeleportPlayerTo(int targetX, int targetY)
{
    m_lastTeleportPath.clear();
    m_lastTeleportNeedsAnimation = false;
    if (!m_pendingTeleportPath.empty()) return false;
    if (m_floor3PrisonStoryPending) return false;
    m_lastTeleportPath = findTeleportPath(targetX, targetY);
    if (m_lastTeleportPath.empty()) return false;

    m_pendingTeleportPath = m_lastTeleportPath;
    m_pendingTeleportTargetX = targetX;
    m_pendingTeleportTargetY = targetY;

    // 三层夹击和十层花门是“走到指定格后停下、显现机关”的剧情点。
    // 规划阶段就截断到该格，避免动画继续走到原点击目标而剧情却在中途停住。
    for (size_t i = 1; i + 1 < m_pendingTeleportPath.size(); ++i) {
        const auto [pathX, pathY] = m_pendingTeleportPath[i];
        const bool stopsAtStoryPoint =
            (m_floor == 3 && !m_floor3PrisonTriggered && pathX == 6 && pathY == 9) ||
            (m_floor == 10 && !m_floor10AmbushTriggered && pathX == 7 && pathY == 6);
        if (stopsAtStoryPoint) {
            m_pendingTeleportPath.resize(i + 1);
            m_lastTeleportPath = m_pendingTeleportPath;
            m_pendingTeleportTargetX = pathX;
            m_pendingTeleportTargetY = pathY;
            break;
        }
    }

    // 只有会产生交互或沿途事件的瞬移才需要播放逐格动画；普通空地仍保持
    // 原有的即时传送体验。副作用全部留到 completeTeleportPlayerTo()。
    const int targetTile = tileAt(targetX, targetY);
    const bool finalInteraction = targetTile != Tile_Floor;
    const auto hasApproachHazard = [this](int x, int y) {
        if (m_player.hasHolyShield || m_floor3PrisonStoryPending) return false;
        static constexpr int directions[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        bool hasMage = false;
        for (const auto& direction : directions) {
            const Monster* nearby = monsterAt(x + direction[0], y + direction[1]);
            if (!nearby) continue;
            const std::string& name = nearby->GetName();
            if (name.find("高级巫师") != std::string::npos ||
                name.find("初级巫师") != std::string::npos) {
                hasMage = true;
                break;
            }
        }
        const auto isMagicGuard = [this](int gx, int gy) {
            const Monster* guard = monsterAt(gx, gy);
            return guard && guard->GetName().find("魔法警卫") != std::string::npos;
        };
        return hasMage ||
               (isMagicGuard(x - 1, y) && isMagicGuard(x + 1, y)) ||
               (isMagicGuard(x, y - 1) && isMagicGuard(x, y + 1));
    };
    bool pathEvent = finalInteraction;
    for (const auto& [pathX, pathY] : m_lastTeleportPath) {
        if (isTeleportStoryPoint(pathX, pathY) || hasApproachHazard(pathX, pathY)) {
            pathEvent = true;
            break;
        }
    }
    m_lastTeleportNeedsAnimation = m_lastTeleportPath.size() > 1 && pathEvent;
    return true;
}

Game::MoveResult Game::completeTeleportPlayerTo()
{
    if (m_pendingTeleportPath.empty()) return Move_Block;
    m_lastTeleportPath = std::move(m_pendingTeleportPath);
    const int targetX = m_pendingTeleportTargetX;
    const int targetY = m_pendingTeleportTargetY;
    m_pendingTeleportTargetX = -1;
    m_pendingTeleportTargetY = -1;

    const int tile = tileAt(targetX, targetY);
    bool finalHazardApplied = false;
    for (size_t i = 1; i < m_lastTeleportPath.size(); ++i) {
        const auto [pathX, pathY] = m_lastTeleportPath[i];
        const bool finalMonster = i + 1 == m_lastTeleportPath.size() && tile == Tile_Monster;
        if (finalMonster) {
            // 43层藤都子SP的首次点击是剧情退场，不应直接进入战斗。
            if (m_floor == 43 && pathX == 10 && pathY == 2 && !m_floor43MiyakoRetreated) {
                m_player.x = pathX;
                m_player.y = pathY;
                const auto result = tryMovePlayer(pathX, pathY);
                m_lastTeleportNeedsAnimation = true;
                return result;
            }
            break;
        }
        const bool storyPoint = isTeleportStoryPoint(pathX, pathY);
        if (storyPoint && i + 1 < m_lastTeleportPath.size())
            m_lastTeleportNeedsAnimation = true;
        m_player.x = pathX;
        m_player.y = pathY;
        if (m_floor == 38 && pathX == 3 && pathY == 7 && !m_floor38FlowerTriggered) {
            m_floor38FlowerTriggered = true;
            setTile(3, 6, Tile_DoorMagic);
        }
        if (storyPoint) {
            triggerFloor10AmbushIfNeeded();
            if (m_floor3PrisonStoryPending ||
                (m_floor == 10 && m_floor10AmbushTriggered && pathX == 7 && pathY == 6)) {
                m_lastTeleportNeedsAnimation = true;
                m_lastTeleportPath.resize(i + 1);
                return Move_Ok;
            }
        }
        const int hpBefore = m_player.hp;
        applyApproachHazardsAt(pathX, pathY);
        if (m_player.hp != hpBefore && i + 1 > 1)
            m_lastTeleportNeedsAnimation = true;
        if (i + 1 == m_lastTeleportPath.size()) finalHazardApplied = true;
        if (m_player.hp <= 0) {
            m_lastTeleportPath.resize(i + 1);
            return Move_PlayerDead;
        }
    }
    if (m_lastTeleportPath.size() > 1 && isTeleportStoryPoint(targetX, targetY))
        m_lastTeleportNeedsAnimation = true;
    m_player.x = targetX;
    m_player.y = targetY;
    if (m_floor == 38 && targetX == 3 && targetY == 7 && !m_floor38FlowerTriggered) {
        m_floor38FlowerTriggered = true;
        setTile(3, 6, Tile_DoorMagic);
    }
    triggerFloor10AmbushIfNeeded();

    switch (tile) {
    case Tile_Item: {
        auto item = takeItemAt(targetX, targetY);
        if (!item) {
            setTile(targetX, targetY, Tile_Floor);
            return Move_Ok;
        }
        if (dynamic_cast<MagicKey*>(item.get()) != nullptr) {
            // 大黄门钥匙沿用原版“一次使用、开启本层全部黄门”的效果。
            for (int y = 0; y < m_height; ++y) {
                for (int x = 0; x < m_width; ++x) {
                    if (tileAt(x, y) == Tile_DoorGreen)
                        setTile(x, y, Tile_Floor);
                }
            }
        } else if (item->IsUseItem()) {
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
        if (!finalHazardApplied) applyApproachHazardsAt(targetX, targetY);
        if (m_player.hp <= 0) return Move_PlayerDead;
        return Move_NPC;
    case Tile_Shop:
        if (!finalHazardApplied) applyApproachHazardsAt(targetX, targetY);
        if (m_player.hp <= 0) return Move_PlayerDead;
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
        if (!finalHazardApplied) applyApproachHazardsAt(targetX, targetY);
        if (m_player.hp <= 0) return Move_PlayerDead;
        return Move_StairsUp;
    case Tile_StairsDown:
        if (!finalHazardApplied) applyApproachHazardsAt(targetX, targetY);
        if (m_player.hp <= 0) return Move_PlayerDead;
        return Move_StairsDown;
    default:
        if (!finalHazardApplied) applyApproachHazardsAt(targetX, targetY);
        if (m_player.hp <= 0) return Move_PlayerDead;
        return Move_Ok;
    }
}

Game::MoveResult Game::teleportPlayerTo(int targetX, int targetY)
{
    if (!beginTeleportPlayerTo(targetX, targetY)) return Move_Block;
    return completeTeleportPlayerTo();
}

bool Game::debugTeleport(int floor, int x, int y)
{
    if (floor < 0 || floor > 50 || x < 0 || y < 0 || x >= m_width || y >= m_height)
        return false;
    if (m_floors.find(floor) == m_floors.end())
        initFloor(floor);
    m_floor = floor;
    m_currentFloor = &m_floors[floor];
    m_lastTeleportPath.clear();
    m_pendingTeleportPath.clear();
    m_pendingTeleportTargetX = -1;
    m_pendingTeleportTargetY = -1;
    m_lastTeleportNeedsAnimation = false;
    m_visitedFloors.insert(m_floor);
    m_player.x = x;
    m_player.y = y;
    // 调试传送应能离开等待中的剧情现场，避免被事件锁死。
    m_floor3PrisonStoryPending = false;
    m_floor32KnightStoryPending = false;
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

void Game::resolveFloor39SymmetryFlyer()
{
    if (m_floor != 39 || !m_currentFloor) return;

    // 原版提示中的“12点”和“3点”分别对应左上房间的上方中门
    // (5,3) 与右侧中门 (7,5)。两扇黄门都打开后，中心格 (5,5)
    // 才生成对称飞行器；重复检查不会重复生成。
    const int twelveOClock = posKey(5, 3);
    const int threeOClock = posKey(7, 5);
    const int rewardKey = posKey(5, 5);
    if (m_currentFloor->map[twelveOClock] != Tile_Floor ||
        m_currentFloor->map[threeOClock] != Tile_Floor ||
        m_currentFloor->items.find(rewardKey) != m_currentFloor->items.end()) {
        return;
    }
    m_currentFloor->items.emplace(rewardKey, std::make_unique<SymmetryFlyer>());
    setTile(5, 5, Tile_Item);
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

void Game::unlockPrincessDollPassage()
{
    auto floorIt = m_floors.find(24);
    if (floorIt == m_floors.end()) {
        initFloor(24);
        floorIt = m_floors.find(24);
    }
    if (floorIt == m_floors.end()) return;

    // 红门上方的隐藏格是原版完成26层假公主对话后显现的直通楼梯。
    const int key = posKey(7, 8);
    FloorData& floor24 = floorIt->second;
    floor24.monsters.erase(key);
    floor24.items.erase(key);
    floor24.map[key] = Tile_StairsUp;
    m_princessDollRescued = true;
}

void Game::activateFloor35Michelle()
{
    m_michelleRescued = true;
    if (m_floors.find(35) == m_floors.end()) initFloor(35);
    FloorData& floor35 = m_floors[35];
    // 薇欧拉固定占据(7,5)，米歇尔改在其下方等待，避免覆盖Boss格。
    const int key = posKey(7, 6);
    for (auto it = floor35.npcs.begin(); it != floor35.npcs.end();) {
        if (it->second.GetName() == "米歇尔" || it->second.ClassicId() == 26) {
            floor35.map[it->first] = floor35.items.count(it->first) ? Tile_Item : Tile_Floor;
            it = floor35.npcs.erase(it);
        } else {
            ++it;
        }
    }
    floor35.npcs.emplace(key, NPC("米歇尔", {"我会去35层帮你打开魔龙房间的暗道。"}, nullptr,
                                  false, 0, nullptr, 26));
    floor35.map[key] = Tile_NPC;
}

void Game::returnMichelleToFloor2Cage()
{
    // 29层剧情结束时，米歇尔回到二层右下角；此时尚未获救，
    // 35层的米歇尔不会提前出现。
    m_michelleRescued = false;
    if (m_floors.find(2) == m_floors.end()) initFloor(2);
    FloorData& floor2 = m_floors[2];
    for (auto it = floor2.npcs.begin(); it != floor2.npcs.end();) {
        if (it->second.ClassicId() == 13 || it->second.GetName() == "米歇尔") {
            floor2.map[it->first] = floor2.items.count(it->first) ? Tile_Item : Tile_Floor;
            it = floor2.npcs.erase(it);
        } else {
            ++it;
        }
    }
    // 29层剧情后的返场位置固定为二层右下角 (12,12)，与开局三层陷阱
    // 使用的 (4,8) 小偷牢笼分开，避免两段剧情互相覆盖。
    floor2.map[posKey(3, 8)] = Tile_DarkWall;
    const int returnKey = posKey(12, 12);
    floor2.npcs.emplace(returnKey,
                        NPC("米歇尔", {"大家到35层集合吧，我会在那里打开魔龙房间的暗道。"},
                            nullptr, false, 0, nullptr, 13));
    floor2.map[returnKey] = Tile_NPC;
}

void Game::sendMichelleToFloor29()
{
    if (m_floors.find(29) == m_floors.end()) initFloor(29);
    FloorData& floor29 = m_floors[29];
    for (auto it = floor29.npcs.begin(); it != floor29.npcs.end();) {
        if (it->second.GetName() == "米歇尔" || it->second.ClassicId() == 25) {
            floor29.map[it->first] = floor29.items.count(it->first) ? Tile_Item : Tile_Floor;
            it = floor29.npcs.erase(it);
        } else ++it;
    }
    // 原版29层米歇尔位于中央通道 (7,3)，其下方 (7,4) 是对话后消失的暗墙。
    const int key = posKey(7, 3);
    floor29.npcs.emplace(key, NPC("米歇尔", {"二楼牢笼已经脱险了，接下来去35层打开魔龙房间的暗道吧。"},
                                  nullptr, false, 0, nullptr, 25));
    floor29.map[key] = Tile_NPC;
}

void Game::triggerFloor20VampireIfNeeded()
{
    if (m_floor != 20 || m_floor20VampireTriggered || !m_currentFloor) return;
    if (m_player.x != 7 || m_player.y != 9) return;
    m_floor20VampireTriggered = true;
    // 事件触发前中央九格只有普通怪物；进入(7,9)后清空并显现中心吸血鬼。
    for (int y = 6; y <= 8; ++y) {
        for (int x = 6; x <= 8; ++x) {
            const int key = posKey(x, y);
            m_currentFloor->monsters.erase(key);
            if (tileAt(x, y) == Tile_Monster)
                setTile(x, y, m_currentFloor->items.count(key) ? Tile_Item : Tile_Floor);
        }
    }
    const int center = posKey(7, 7);
    m_currentFloor->monsters[center] = MonsterDB::get("凑友希那·吸血鬼");
    setTile(7, 7, Tile_Monster);
    // 原红门位置与顶端新增门都改为花门，必须击败中心吸血鬼才能开启。
    setTile(7, 10, Tile_DoorMagic);
    setTile(7, 4, Tile_DoorMagic);
}

std::vector<Game::MonsterMovementAnimation> Game::takeFloor32KnightMovementAnimations()
{
    auto result = std::move(m_floor32KnightMovements);
    m_floor32KnightMovements.clear();
    return result;
}

std::vector<std::pair<int, int>> Game::floor32KnightRoute() const
{
    if (!m_currentFloor || m_floor != 32) return {};
    const std::pair<int, int> source{12, 2};
    const std::pair<int, int> target{7, 10};
    const int sourceKey = posKey(source.first, source.second);
    const int targetKey = posKey(target.first, target.second);
    std::queue<int> pending;
    std::unordered_map<int, int> parent;
    pending.push(sourceKey);
    parent[sourceKey] = sourceKey;
    constexpr int directions[4][2] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};
    while (!pending.empty() && parent.count(targetKey) == 0) {
        const int key = pending.front();
        pending.pop();
        const int x = key % m_width;
        const int y = key / m_width;
        for (const auto& direction : directions) {
            const int nx = x + direction[0];
            const int ny = y + direction[1];
            if (nx < 2 || nx > m_width - 3 || ny < 2 || ny > m_height - 3)
                continue;
            const int nextKey = posKey(nx, ny);
            if (parent.count(nextKey) != 0) continue;
            const int tile = m_currentFloor->map[nextKey];
            const bool walkable = tile == Tile_Floor || tile == Tile_StairsUp ||
                                  tile == Tile_StairsDown || nextKey == targetKey;
            if (!walkable) continue;
            parent[nextKey] = key;
            pending.push(nextKey);
        }
    }
    if (parent.count(targetKey) == 0) return {};
    std::vector<std::pair<int, int>> route;
    for (int key = targetKey;; key = parent[key]) {
        route.push_back({key % m_width, key / m_width});
        if (key == sourceKey) break;
    }
    std::reverse(route.begin(), route.end());
    return route;
}

void Game::triggerFloor32KnightIfNeeded()
{
    if (m_floor != 32 || m_floor32KnightTriggered || !m_currentFloor) return;
    if (m_player.x != 7 || m_player.y != 11) return;
    const auto route = floor32KnightRoute();
    if (route.size() < 2) return;
    m_floor32KnightTriggered = true;
    m_floor32KnightStoryPending = true;
    // 骑士队长从右上黄色楼梯出现，但在行走动画结束前不写入终点格，
    // 避免角色瞬间出现在主角面前。路径由地板寻路生成，不再穿墙。
    const int sourceX = 12;
    const int sourceY = 2;
    const int sourceKey = posKey(sourceX, sourceY);
    const int knightKey = posKey(7, 10);
    m_currentFloor->monsters.erase(sourceKey);
    m_currentFloor->monsters.erase(knightKey);
    setTile(sourceX, sourceY, Tile_StairsUp);
    setTile(7, 10, Tile_Floor);
    const Monster knight = MonsterDB::get("幼年长崎素世·骑士队长");
    for (std::size_t i = 1; i < route.size(); ++i) {
        m_floor32KnightMovements.push_back({knight,
            route[i - 1].first, route[i - 1].second,
            route[i].first, route[i].second});
    }
    // 最后一格真正撞向站在(7,11)的爱音，动画完成后再把战斗实体
    // 放回相邻的(7,10)，由界面自动结算先攻与战斗。
    m_floor32KnightMovements.push_back({knight, 7, 10, 7, 11});
}

int Game::resolveFloor32KnightStory()
{
    if (!m_floor32KnightStoryPending) return 0;
    m_floor32KnightStoryPending = false;
    const int knightKey = posKey(7, 10);
    if (!monsterAt(7, 10)) {
        m_currentFloor->monsters[knightKey] = MonsterDB::get("幼年长崎素世·骑士队长");
        setTile(7, 10, Tile_Monster);
    }
    Monster* knight = monsterAt(7, 10);
    if (!knight) return 0;
    const int damage = std::max(0, knight->Attack() - m_player.def);
    m_player.hp = std::max(0, m_player.hp - damage);
    return damage;
}

void Game::triggerFloor42KnightIfNeeded()
{
    if (m_floor != 42 || m_floor42KnightStoryTriggered || !m_currentFloor)
        return;
    m_floor42KnightStoryTriggered = true;
    m_floor42KnightStoryPending = true;

    // 首次抵达42层时，骑士队长逃跑并被魔王抓住；
    // 由界面播放魔王与四名魔法警卫夹击的剧情，结束后仍留在42层。
    const int knightKey = posKey(7, 11);
    auto it = m_currentFloor->monsters.find(knightKey);
    if (it != m_currentFloor->monsters.end() &&
        it->second.GetName() == "幼年长崎素世·骑士队长") {
        m_currentFloor->monsters.erase(it);
        m_currentFloor->map[knightKey] =
            m_currentFloor->items.count(knightKey) ? Tile_Item : Tile_Floor;
    }
}

void Game::resolveFloor42KnightStory()
{
    if (!m_floor42KnightStoryPending || m_floor != 42)
        return;
    m_floor42KnightStoryPending = false;
    // 剧情结束后不改变楼层，玩家继续探索42层。
}

void Game::triggerFloor14RewardIfCleared()
{
    if (m_floor != 14 || m_floor14RedKeyRewardGranted || !m_currentFloor) return;
    // 只检测左上角这三只怪；(8,3)与红钥匙机关无关。
    const std::pair<int, int> targets[] = {{2, 2}, {4, 2}, {3, 3}};
    for (const auto& target : targets)
        if (hasMonsterAt(target.first, target.second)) return;
    m_floor14RedKeyRewardGranted = true;
    const int rewardKey = posKey(2, 4);
    m_currentFloor->monsters.erase(rewardKey);
    m_currentFloor->items[rewardKey] = std::make_unique<Key>(KeyType::Red);
    setTile(2, 4, Tile_Item);
}

void Game::resolveFloor34RewardIfCleared()
{
    if (m_floor != 34 || m_floor34RewardGranted || !m_currentFloor) return;
    for (const auto& entry : m_currentFloor->monsters) {
        const int y = entry.first / m_width;
        if (y == 5 || y == 9) return;
    }
    m_floor34RewardGranted = true;
    std::vector<std::unique_ptr<Item>> rewards;
    rewards.emplace_back(std::make_unique<Key>(KeyType::Red));
    for (int i = 0; i < 4; ++i) rewards.emplace_back(std::make_unique<Key>(KeyType::Green));
    // 原版34层奖励固定围绕(3,7)摆放；中心格放红钥匙，四个正交邻格放黄钥匙。
    const std::pair<int, int> fixedSpots[] = {{3, 7}, {2, 7}, {4, 7}, {3, 6}, {3, 8}};
    size_t placed = 0;
    for (const auto& spot : fixedSpots) {
        if (placed >= rewards.size()) break;
        const int key = posKey(spot.first, spot.second);
        if (tileAt(spot.first, spot.second) != Tile_Floor || m_currentFloor->items.count(key) != 0)
            continue;
        addItemAt(spot.first, spot.second, std::move(rewards[placed++]));
        setTile(spot.first, spot.second, Tile_Item);
    }
}

void Game::revealFloor35RewardsIfDragonDefeated()
{
    if (m_floor != 35 || !m_currentFloor || !m_floor35RewardsHidden) return;
    for (const auto& entry : m_floor35HiddenItems) {
        const int key = entry.first;
        if (m_currentFloor->items.count(key) != 0) continue;
        auto item = createItemByName(entry.second.first, entry.second.second);
        if (!item) continue;
        m_currentFloor->items.emplace(key, std::move(item));
        m_currentFloor->map[key] = Tile_Item;
    }
    m_floor35HiddenItems.clear();
    m_floor35RewardsHidden = false;
}

void Game::spawnFloor40DeferredRewards()
{
    if (m_floor != 40 || !m_currentFloor || !m_floor40BossDefeated ||
        m_floor40RewardsGranted)
        return;
    // 40层奖励的触发条件是红门上方（y<9）的怪物全部清空。
    for (const auto& entry : m_currentFloor->monsters)
        if (entry.first / m_width < 9)
            return;

    const ClassicItemTier tier = classicItemTierForFloor(40);
    std::vector<std::unique_ptr<Item>> rewards;
    for (int i = 0; i < 3; ++i) {
        rewards.emplace_back(std::make_unique<RubyGem>(tier.rubyAttack, "舞台红宝石"));
        rewards.emplace_back(std::make_unique<SapphireGem>(tier.sapphireDefense, "舞台蓝宝石"));
        rewards.emplace_back(std::make_unique<Key>(KeyType::Green));
        rewards.emplace_back(std::make_unique<LargePotion>(tier.largePotionHp));
    }
    size_t placed = 0;
    for (int radius = 0; radius <= m_width + m_height && placed < rewards.size(); ++radius) {
        for (int y = 2; y <= m_height - 3 && placed < rewards.size(); ++y) {
            for (int x = 2; x <= m_width - 3 && placed < rewards.size(); ++x) {
                if (y >= 9 || std::abs(x - 7) + std::abs(y - 9) != radius) continue;
                const int key = posKey(x, y);
                if (tileAt(x, y) != Tile_Floor || m_currentFloor->items.count(key) ||
                    m_currentFloor->monsters.count(key)) continue;
                addItemAt(x, y, std::move(rewards[placed++]));
                setTile(x, y, Tile_Item);
            }
        }
    }
    if (placed == rewards.size()) {
        setTile(m_width / 2, 2, Tile_StairsUp);
        m_floor40RewardsGranted = true;
    }
}

void Game::completeFloor35MichelleStory()
{
    if (!m_michelleRescued) activateFloor35Michelle();
    FloorData& floor35 = m_floors[35];
    for (auto it = floor35.npcs.begin(); it != floor35.npcs.end();) {
        if (it->second.GetName() == "米歇尔" || it->second.ClassicId() == 26) {
            floor35.map[it->first] = floor35.items.count(it->first) ? Tile_Item : Tile_Floor;
            it = floor35.npcs.erase(it);
        } else {
            ++it;
        }
    }
    // 米歇尔打开的是本层全部暗墙；花门和铁门仍保留各自的机关条件。
    for (int& tile : floor35.map)
        if (tile == Tile_DarkWall)
            tile = Tile_Floor;

    // 35层暗道打开后，米歇尔不回二楼，而是前往终幕的50层。
    // 清理二楼牢笼中的旧NPC，避免玩家误以为她仍在那里等待对话。
    if (m_floors.find(2) == m_floors.end()) initFloor(2);
    FloorData& floor2 = m_floors[2];
    for (auto it = floor2.npcs.begin(); it != floor2.npcs.end();) {
        if (it->second.ClassicId() == 13 || it->second.GetName() == "米歇尔") {
            floor2.map[it->first] = floor2.items.count(it->first) ? Tile_Item : Tile_Floor;
            it = floor2.npcs.erase(it);
        } else {
            ++it;
        }
    }
    if (m_floors.find(50) == m_floors.end()) initFloor(50);
    FloorData& floor50 = m_floors[50];
    const int revealKey = posKey(7, 5);
    floor50.npcs.erase(revealKey);
    floor50.npcs.emplace(revealKey, NPC("米歇尔",
        {"终于到达终幕了……是时候让你看见我的真面目。"}, nullptr,
        false, 0, nullptr, 47));
    floor50.map[revealKey] = Tile_NPC;
    // 在最终对话前隐藏王座处的素世本体，避免玩家跳过伪装揭示直接开战。
    const int hiddenBossKey = posKey(7, 6);
    floor50.monsters.erase(hiddenBossKey);
    floor50.map[hiddenBossKey] = floor50.items.count(hiddenBossKey) ? Tile_Item : Tile_Floor;
    // 救出后的35层剧情已经完成；保留该状态，避免重复生成35层NPC。
    m_michelleRescued = true;
}

void Game::revealFloor50MichelleIdentity()
{
    if (m_floors.find(50) == m_floors.end()) initFloor(50);
    FloorData& floor50 = m_floors[50];
    const int revealKey = posKey(7, 5);
    floor50.npcs.erase(revealKey);
    floor50.map[revealKey] = floor50.items.count(revealKey) ? Tile_Item : Tile_Floor;

    // 50层王座处显现长崎素世本体；若旧地图缺少Boss则补回原版终幕位置。
    const int bossKey = posKey(7, 6);
    floor50.monsters[bossKey] = MonsterDB::get("长崎素世·本体");
    floor50.map[bossKey] = Tile_Monster;
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
    triggerFloor33TrapIfNeeded();
    triggerFloor3PrisonStoryIfNeeded();
    triggerFloor20VampireIfNeeded();
    triggerFloor32KnightIfNeeded();
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
    m_scriptedMonsterMovements.clear();
    resolveFloor10AmbushIfCleared();
}

void Game::triggerFloor33TrapIfNeeded()
{
    if (m_floor != 33 || m_floor33TrapTriggered || !m_currentFloor) return;
    if (m_player.x != 11 || m_player.y != 7) return;
    m_floor33TrapTriggered = true;
    setTile(11, 5, Tile_DoorMagic);
    setTile(11, 9, Tile_DoorMagic);
    // 资源缺失时补回四个固定位置的原版守卫，保证机关可测试。
    const std::pair<int, int> positions[] = {{10, 6}, {12, 6}, {10, 8}, {12, 8}};
    const int monsterIds[] = {18, 18, 19, 19};
    for (int i = 0; i < 4; ++i) {
        const int key = posKey(positions[i].first, positions[i].second);
        if (!hasMonsterAt(positions[i].first, positions[i].second)) {
            m_currentFloor->monsters[key] = MonsterDB::getByIndex(monsterIds[i] - 1);
            setTile(positions[i].first, positions[i].second, Tile_Monster);
        }
    }
}

void Game::resolveFloor33TrapIfCleared()
{
    if (m_floor != 33 || !m_floor33TrapTriggered || !m_currentFloor) return;
    const int guards[] = {posKey(10, 6), posKey(12, 6), posKey(10, 8), posKey(12, 8)};
    for (const int key : guards)
        if (m_currentFloor->monsters.find(key) != m_currentFloor->monsters.end()) return;
    setTile(11, 5, Tile_Floor);
    setTile(11, 9, Tile_Floor);
}

std::vector<Game::MonsterMovementAnimation> Game::takeFloor10AmbushMovementAnimations()
{
    auto movements = std::move(m_floor10AmbushMovements);
    m_floor10AmbushMovements.clear();
    m_scriptedMonsterMovements.clear();
    return movements;
}

std::vector<Game::MonsterMovementAnimation> Game::takeScriptedMonsterMovementAnimations()
{
    auto movements = std::move(m_scriptedMonsterMovements);
    m_scriptedMonsterMovements.clear();
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

int Game::useMagicKey()
{
    if (!m_currentFloor) return 0;
    int opened = 0;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            if (tileAt(x, y) == Tile_DoorGreen) {
                setTile(x, y, Tile_Floor);
                ++opened;
            }
        }
    }
    return opened;
}

bool Game::canTeleportByStairItem(bool up, int srcX, int srcY) const
{
    const int targetFloor = up ? m_floor + 1 : m_floor - 1;
    if (targetFloor < 0 || targetFloor > 50) return false;
    const auto it = m_floors.find(targetFloor);
    if (it == m_floors.end()) return false;

    int targetX = std::clamp(srcX, 2, m_width - 3);
    int targetY = std::clamp(srcY, 2, m_height - 3);
    // 50层终幕入口由楼层切换逻辑固定为王座入口(7,8)。
    if (up && targetFloor == 50) {
        targetX = 7;
        targetY = 8;
    }
    if (targetX < 0 || targetY < 0 || targetX >= m_width || targetY >= m_height)
        return false;
    const auto& targetMap = it->second.map;
    if (static_cast<int>(targetMap.size()) != m_width * m_height) return false;
    return targetMap[targetY * m_width + targetX] == Tile_Floor;
}

void Game::goUpFloor(int srcX, int srcY, bool findStairs)
{
    if (m_floor == 24 && m_princessDollRescued && findStairs && srcX == 7 && srcY == 8) {
        // 24层红门后的隐藏楼梯直通50层最终舞台。
        m_floor = 50;
        if (m_floors.find(m_floor) == m_floors.end())
            initFloor(m_floor);
        m_currentFloor = &m_floors[m_floor];
        m_visitedFloors.insert(m_floor);
        // 50层终幕固定从王座入口(7,8)开始，避免沿用来源楼层坐标落在错误区域。
        m_player.x = 7;
        m_player.y = 8;
        return;
    }
    // 44层是独立异空间，原版楼梯路线从43层直接连到45层；
    // 只有上楼器（findStairs=false）才能把目标设为44层。
    m_floor = (findStairs && m_floor == 43) ? 45 : m_floor + 1;
    if (m_floors.find(m_floor) == m_floors.end()) {
        initFloor(m_floor);
    }
    m_currentFloor = &m_floors[m_floor];
    m_visitedFloors.insert(m_floor);
    prepareFloor3PrisonCell();
    if (m_floor == 42)
        triggerFloor42KnightIfNeeded();

    if (m_floor == 50) {
        // 无论是楼梯还是楼层传送器，进入终幕都从固定入口(7,8)开始。
        m_player.x = 7;
        m_player.y = 8;
        return;
    }

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
    // 原版0层只能由1层使用下楼器/传送器抵达，普通下楼梯不通往0层。
    if (m_floor <= 0 || (m_floor == 1 && findStairs)) return;
    // 从45层沿普通楼梯返回时同样跳过44层；下楼器可显式落到44层。
    m_floor = (findStairs && m_floor == 45) ? 43 : m_floor - 1;
    if (m_floors.find(m_floor) == m_floors.end())
        initFloor(m_floor);
    m_currentFloor = &m_floors[m_floor];
    m_visitedFloors.insert(m_floor);
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
    if (m_floor3PrisonStoryPending || m_floor32KnightStoryPending) return Move_Block;
    if (nx < 0 || ny < 0 || nx >= m_width || ny >= m_height) return Move_Block;
    int tile = tileAt(nx, ny);
    const auto finishMove = [this](MoveResult result) {
        applyApproachHazardsAt(m_player.x, m_player.y);
        return m_player.hp <= 0 ? Move_PlayerDead : result;
    };

    switch (tile) {
    case Tile_Wall:
        // 检查破墙锤
        if (m_player.wallBreakerUsed) {
            if (breakWall(nx, ny)) {
                m_player.wallBreakerUsed = false;
                m_player.x = nx; m_player.y = ny;
                triggerFloor10AmbushIfNeeded();
                return finishMove(Move_Ok);
            }
        }
        return Move_Block;

    case Tile_Lava:
        if (m_player.freezeMagicUsed) {
            m_player.freezeMagicUsed = false;
            setTile(nx, ny, Tile_Floor);
            m_player.x = nx; m_player.y = ny;
            triggerFloor10AmbushIfNeeded();
            return finishMove(Move_Ok);
        }
        return Move_Block;
    case Tile_StarRiver:
        return Move_Block;

    case Tile_DoorMagic:
    case Tile_DoorIron:
        // 机关门不消耗钥匙，击败本楼层指定守卫后自动打开。
        // 48层圣剑房花门未触发专属事件前仍只能用镐破坏；
        // 左上角藤都子SP被击败后由事件直接解锁。
        if (m_floor == 48 && m_player.wallBreakerUsed) {
            m_player.wallBreakerUsed = false;
            setTile(nx, ny, Tile_Floor);
            m_player.x = nx; m_player.y = ny;
            triggerFloor10AmbushIfNeeded();
            return finishMove(Move_Ok);
        }
        if (!mechanismDoorReadyAt(m_floor, *m_currentFloor, nx, ny)) return Move_DoorLocked;
        setTile(nx, ny, Tile_Floor);
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return finishMove(Move_Ok);

    case Tile_DarkWall:
        if (m_floor == 35 && nx == 7 && ny == 4) {
            // 魔龙房暗墙由35层米歇尔剧情开启，不能提前撞墙或用破墙锤绕过。
            return Move_Block;
        }
        // 41层左上高级巫师被击败后，撞击右上对称暗墙(11,3)会显现隐藏的
        // 第二名高级巫师；在此之前暗墙不可被普通撞击或破墙道具绕过。
        if (m_floor == 41 && nx == 11 && ny == 3) {
            if (hasMonsterAt(3, 3)) return Move_Block;
            if (!hasMonsterAt(nx, ny)) {
                setTile(nx, ny, Tile_Monster);
                spawnMonster(nx, ny, MonsterDB::getByIndex(26)); // 峰月律SP·高级巫师
                return Move_Block;
            }
        }
        // 暗墙是可撞开的机关墙：第一次碰撞只打开墙体，下一次移动才进入。
        // 破墙道具仍可立即打开并进入，普通墙不会走这条分支。
        if (m_player.wallBreakerUsed && breakWall(nx, ny)) {
            m_player.wallBreakerUsed = false;
            m_player.x = nx; m_player.y = ny;
            triggerFloor10AmbushIfNeeded();
            return finishMove(Move_Ok);
        }
        setTile(nx, ny, m_currentFloor->items.count(posKey(nx, ny)) ? Tile_Item : Tile_Floor);
        return Move_Block;

    case Tile_Floor:
        m_player.x = nx; m_player.y = ny;
        if (m_floor == 38 && nx == 3 && ny == 7 && !m_floor38FlowerTriggered) {
            // 到达38层(3,7)后才显现上方的剧情花门；该门始终由剧情控制。
            m_floor38FlowerTriggered = true;
            setTile(3, 6, Tile_DoorMagic);
        }
        triggerFloor10AmbushIfNeeded();
        return finishMove(Move_Ok);

    case Tile_DoorRed:
        if (m_floor == 24 && nx == 7 && ny == 9 && !m_princessDollRescued)
            return Move_DoorLocked;
        if (m_player.HasKey(KeyType::Red)) {
            m_player.UseKey(KeyType::Red);
        } else {
            return Move_DoorLocked;
        }
        setTile(nx, ny, Tile_Floor);
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return finishMove(Move_Ok);

    case Tile_DoorBlue:
        if (m_player.HasKey(KeyType::Blue)) {
            m_player.UseKey(KeyType::Blue);
        } else {
            return Move_DoorLocked;
        }
        setTile(nx, ny, Tile_Floor);
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return finishMove(Move_Ok);

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
        resolveFloor39SymmetryFlyer();
        triggerFloor10AmbushIfNeeded();
        return finishMove(Move_Ok);

    case Tile_Monster:
        if (m_floor == 43 && nx == 10 && ny == 2 && !m_floor43MiyakoRetreated) {
            // 藤都子SP第一次交互是退场事件，不进入战斗；战斗位置改到右侧，
            // 中间格同时被剧情墙封住，后续再次交互才会正常战斗。
            const int fromKey = posKey(10, 2);
            const int toKey = posKey(12, 2);
            if (const auto it = m_currentFloor->monsters.find(fromKey);
                it != m_currentFloor->monsters.end()) {
                m_scriptedMonsterMovements.push_back({it->second, 10, 2, 12, 2});
            }
            m_currentFloor->monsters.erase(fromKey);
            m_currentFloor->monsters.erase(toKey);
            m_currentFloor->monsters.emplace(toKey, MonsterDB::get("藤都子SP·魔法警卫"));
            setTile(10, 2, m_currentFloor->items.count(fromKey) ? Tile_Item : Tile_Floor);
            setTile(11, 2, Tile_Wall);
            setTile(12, 2, Tile_Monster);
            m_floor43MiyakoRetreated = true;
            return Move_Ok;
        }
        if (hasMonsterAt(nx, ny))
            return Move_Encounter;
        else {
            m_player.x = nx; m_player.y = ny;
            triggerFloor10AmbushIfNeeded();
            return finishMove(Move_Ok);
        }

    case Tile_Item: {
        auto item = takeItemAt(nx, ny);
        if (item) {
            if (dynamic_cast<MagicKey*>(item.get()) != nullptr) {
                // 大黄门钥匙必须进入背包，改由背包中的“使用”动作开启本层全部黄门。
                m_player.AddItem(std::move(item));
            } else if (item->IsUseItem()) {
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
            return finishMove(Move_Pickup);
        }
        setTile(nx, ny, Tile_Floor);
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return finishMove(Move_Ok);
    }

    case Tile_NPC:
        return Move_NPC;

    case Tile_Shop:
        return Move_Shop;

    case Tile_StairsUp:
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return finishMove(Move_StairsUp);

    case Tile_StairsDown:
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return finishMove(Move_StairsDown);

    default:
        m_player.x = nx; m_player.y = ny;
        triggerFloor10AmbushIfNeeded();
        return finishMove(Move_Ok);
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
    if (m_floor == 49 && x == 7 && y == 4 && bossName == "长崎素世·幻影" &&
        m->GetHP() >= 8000) {
        outLog.push_back("长崎素世·幻影尚未解除封印，先击败她上下左右的四名魔法警卫！");
        return Fight_Stalemate;
    }
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
            if (m_floor == 2 && MonsterDB::indexOf(bossName) == 20)
                m_michelleGuardDefeated = true;
            triggerFloor14RewardIfCleared();
            resolveFloor34RewardIfCleared();
            const bool hadHiddenFloor35Rewards =
                m_floor == 35 && bossName == "薇欧拉SP·魔龙" && !m_floor35HiddenItems.empty();
            if (hadHiddenFloor35Rewards)
                revealFloor35RewardsIfDragonDefeated();
            if (m_floor == 32 && bossName == "幼年长崎素世·骑士队长") {
                // 逆序播放来时的地板路径，确保撤退也不穿墙。
                const auto route = floor32KnightRoute();
                const Monster knight = MonsterDB::get("幼年长崎素世·骑士队长");
                m_floor32KnightMovements.push_back({knight, 7, 11, 7, 10});
                for (std::size_t i = route.size(); i > 1; --i) {
                    m_floor32KnightMovements.push_back({knight,
                        route[i - 1].first, route[i - 1].second,
                        route[i - 2].first, route[i - 2].second});
                }
                m_currentFloor->map[posKey(6, 12)] = Tile_StairsDown;
                m_currentFloor->map[posKey(12, 2)] = Tile_StairsUp;
            }
            openMechanismDoorsIfReady();
            resolveFloor33TrapIfCleared();
            if (m_floor == 48 && x == 2 && y == 2 &&
                bossName == "藤都子SP·魔法警卫") {
                // 48层圣剑房花门只由这只左上角藤都子SP解锁，
                // 不受其他魔法门的通用清怪规则影响。
                if (tileAt(9, 9) == Tile_DoorMagic) {
                    setTile(9, 9, Tile_Floor);
                    outLog.push_back("藤都子SP被击败，48层圣剑房花门已解锁！");
                }
            }
            if (m_floor == 41 && x == 11 && y == 3 &&
                bossName == "峰月律SP·高级巫师") {
                bool advancedWizardsRemain = false;
                for (const auto& entry : m_currentFloor->monsters) {
                    if (entry.second.GetName().find("高级巫师") != std::string::npos) {
                        advancedWizardsRemain = true;
                        break;
                    }
                }
                if (advancedWizardsRemain) {
                    outLog.push_back("41层仍有高级巫师，击败全部高级巫师后才能生成下楼器。");
                } else {
                    // 原版第二名高级巫师被击败后，魔法金币在(7,6)显现，
                    // 并以三墙夹一通道的方式重排周围地形。
                    const int rewardKey = posKey(7, 6);
                    if (m_currentFloor->items.count(rewardKey) == 0) {
                        setTile(7, 6, Tile_Floor);
                        addItemAt(7, 6, std::make_unique<StairLower>());
                        setTile(7, 6, Tile_Item);
                        setTile(6, 7, Tile_Wall);
                        setTile(7, 7, Tile_Wall);
                        setTile(8, 7, Tile_Wall);
                        setTile(6, 8, Tile_Floor);
                        setTile(8, 8, Tile_Floor);
                        outLog.push_back("隐藏的高级巫师被击败，(7,6)显现撤场通行卡（下楼器）；周围墙体随之重排！");
                    }
                }
            }
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
                    auto bossIt = m_currentFloor->monsters.find(bossKey);
                    if (bossIt != m_currentFloor->monsters.end() &&
                        bossIt->second.GetName() == "长崎素世·幻影" &&
                        bossIt->second.GetHP() >= 8000) {
                        bossIt->second = Monster("长崎素世·幻影", 800, 500, 100, 500);
                        outLog.push_back("四名魔法警卫被击败，魔王封印解除；长崎素世·幻影的属性降为原来的十分之一！");
                    }
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
            // 原版阶段 Boss 击败后会留下固定奖励并开启通往上一层的出口。
            // 奖励按原版掉落组合放在 Boss 周围，避免覆盖地图上的其他静态物件。
            std::vector<std::unique_ptr<Item>> bossRewards;
            std::string rewardSummary;
            bool spawnBossStair = false;
            const int rewardStairX = m_width / 2;
            const int rewardStairY = (m_floor == 20 || m_floor == 40) ? 2 : m_height - 3;
            const auto appendClassicBossRewards = [&](const ClassicItemTier& tier) {
                for (int i = 0; i < 3; ++i) {
                    bossRewards.emplace_back(std::make_unique<RubyGem>(tier.rubyAttack, "舞台红宝石"));
                    bossRewards.emplace_back(std::make_unique<SapphireGem>(tier.sapphireDefense, "舞台蓝宝石"));
                    bossRewards.emplace_back(std::make_unique<Key>(KeyType::Green));
                    bossRewards.emplace_back(std::make_unique<LargePotion>(tier.largePotionHp));
                }
                rewardSummary = "舞台红宝石×3、舞台蓝宝石×3、黄色Live票×3、爱音能量饮×3";
            };
            if (m_floor == 10 && bossName == "八幡海铃·骷髅队长") {
                const auto tier = classicItemTierForFloor(m_floor);
                appendClassicBossRewards(tier);
                spawnBossStair = true;
            } else if (m_floor == 20 && bossName == "凑友希那·吸血鬼") {
                const auto tier = classicItemTierForFloor(m_floor);
                appendClassicBossRewards(tier);
                spawnBossStair = true;
            } else if (m_floor == 40 && bossName == "幼年长崎素世·骑士队长") {
                m_floor40BossDefeated = true;
                bool upperMonstersRemain = false;
                for (const auto& entry : m_currentFloor->monsters) {
                    if (entry.first / m_width < 9) {
                        upperMonstersRemain = true;
                        break;
                    }
                }
                if (!upperMonstersRemain) {
                    const auto tier = classicItemTierForFloor(m_floor);
                    appendClassicBossRewards(tier);
                    spawnBossStair = true;
                }
            } else if (m_floor == 25 && bossName == "户山香澄·大法师") {
                for (int i = 0; i < 4; ++i)
                    bossRewards.emplace_back(std::make_unique<Key>(KeyType::Red));
                rewardSummary = "红色Live票×4";
            } else if (m_floor == 35 && bossName == "薇欧拉SP·魔龙") {
                rewardSummary = "海铃冷静指令×1、爱音能量饮×3";
                // 正式经典地图已预埋这四件奖励并在战斗前隐藏，揭示后不重复生成；
                // 无资源/测试地图没有预埋物品时才使用同样组合的后备掉落。
                if (!hadHiddenFloor35Rewards) {
                    bossRewards.emplace_back(std::make_unique<FreezeMagic>());
                    for (int i = 0; i < 3; ++i)
                        bossRewards.emplace_back(std::make_unique<LargePotion>(classicItemTierForFloor(m_floor).largePotionHp));
                }
            } else if (m_floor == 49 && bossName == "长崎素世·幻影") {
                const auto tier = classicItemTierForFloor(m_floor);
                for (int i = 0; i < 3; ++i) {
                    bossRewards.emplace_back(std::make_unique<RubyGem>(tier.rubyAttack, "舞台红宝石"));
                    bossRewards.emplace_back(std::make_unique<SapphireGem>(tier.sapphireDefense, "舞台蓝宝石"));
                    bossRewards.emplace_back(std::make_unique<LargePotion>(tier.largePotionHp));
                }
                bossRewards.emplace_back(std::make_unique<Key>(KeyType::Red));
                bossRewards.emplace_back(std::make_unique<DragonSlayer>());
                rewardSummary = "舞台红宝石×3、舞台蓝宝石×3、爱音能量饮×3、红色Live票×1、祥子指挥棒×1";
            }
            if (!bossRewards.empty()) {
                const std::array<std::pair<int, int>, 9> offsets = {{
                    {0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1},
                    {1, 1}, {-1, 1}, {1, -1}, {-1, -1}
                }};
                std::vector<std::pair<int, int>> candidates;
                std::unordered_set<int> seen;
                const auto appendCandidate = [&](int rx, int ry) {
                    if (rx < 2 || rx > m_width - 3 || ry < 2 || ry > m_height - 3) return;
                    if (seen.insert(posKey(rx, ry)).second) candidates.emplace_back(rx, ry);
                };
                for (const auto& offset : offsets) appendCandidate(x + offset.first, y + offset.second);
                for (int radius = 2; radius <= m_width + m_height; ++radius) {
                    for (int dy = -radius; dy <= radius; ++dy) {
                        const int dx = radius - std::abs(dy);
                        appendCandidate(x + dx, y + dy);
                        if (dx != 0) appendCandidate(x - dx, y + dy);
                    }
                }
                size_t placed = 0;
                for (const auto& candidate : candidates) {
                    if (placed >= bossRewards.size()) break;
                    const int rx = candidate.first;
                    const int ry = candidate.second;
                    const int key = posKey(rx, ry);
                    if ((rx == rewardStairX && ry == rewardStairY) ||
                        tileAt(rx, ry) != Tile_Floor ||
                        m_currentFloor->monsters.count(key) != 0 ||
                        m_currentFloor->items.count(key) != 0)
                        continue;
                    addItemAt(rx, ry, std::move(bossRewards[placed++]));
                    setTile(rx, ry, Tile_Item);
                }
                if (spawnBossStair) {
                    // 20层奖励生成顶部黄色上楼梯；底部原有紫色小楼梯保持不变。
                    setTile(rewardStairX, rewardStairY, Tile_StairsUp);
                    if (m_floor == 40) m_floor40RewardsGranted = true;
                    outLog.push_back("Boss奖励：" + rewardSummary + "；地图正中间出现向上楼梯！");
                } else {
                    outLog.push_back("Boss奖励：" + rewardSummary + "！");
                }
            }
            // 40层Boss先被击败但上方仍有怪物时，最后一只怪物倒下后补发奖励。
            spawnFloor40DeferredRewards();
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
        << m_floor3TrapActive << " " << m_floor3PrisonStoryPending << " "
        << m_princessDollRescued << " " << m_floor20VampireTriggered << " "
        << m_michelleRescued << " " << m_floor33TrapTriggered << " "
        << m_floor38FlowerTriggered << " " << m_floor43MiyakoRetreated << " "
        << m_floor14RedKeyRewardGranted << " " << m_floor32KnightTriggered << " "
        << m_floor32KnightStoryPending << " " << m_floor34RewardGranted << " "
        << m_floor35RewardsHidden << " " << m_floor40BossDefeated << " "
        << m_floor40RewardsGranted << " " << m_michelleGuardDefeated << " "
        << m_floor20VampireStoryShown << " " << m_floor42KnightStoryTriggered << " "
        << m_floor42KnightStoryPending << "\n";

    // 35层魔龙奖励在击败前从地图暂时隐藏；把原始道具一并写入存档，
    // 这样读档后仍能按原版流程在击败魔龙时显现奖励。
    ofs << "HIDDEN35 " << m_floor35HiddenItems.size();
    for (const auto& entry : m_floor35HiddenItems)
        ofs << " " << entry.first << " " << entry.second.first << " " << entry.second.second;
    ofs << "\n";

    // 已访问楼层用于限制爱音手机目标；独立一行以兼容旧版存档。
    ofs << "VISITED " << m_visitedFloors.size();
    for (const int visited : m_visitedFloors) ofs << " " << visited;
    ofs << "\n";

    // 全局一次性剧情标记，保证读档后不会再次播放已完成剧情。
    ofs << "STORIES " << m_storyOnceKeys.size();
    for (const auto& key : m_storyOnceKeys) ofs << " " << key;
    ofs << "\n";

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
        {"Cross", "MyGO和解徽章"}, {"十字架", "MyGO和解徽章"}, {"MyGO团结徽章", "MyGO和解徽章"}, {"Dragon Slayer", "祥子指挥棒"}, {"屠龙匕", "祥子指挥棒"}, {"屠龙匕首", "祥子指挥棒"},
        {"Freeze Magic", "海铃冷静指令"}, {"冰冻魔法", "海铃冷静指令"}, {"冰冻徽章", "海铃冷静指令"}, {"冷静雪花徽章", "海铃冷静指令"}, {"Flying Wand", "爱音手机"}, {"飞行魔杖", "爱音手机"},
        {"Floor Teleporter", "楼层传送器"},
        {"Symmetry Flyer", "Mujica镜面舞台票"}, {"对称飞行器", "Mujica镜面舞台票"},
        {"Monster Book", "怪物手册"}, {"怪物手册", "怪物手册"},
        {"Note Book", "高松灯的单词本"}, {"记事本", "高松灯的单词本"},
        {"灯的歌词本", "高松灯的单词本"}, {"灯的单词本", "高松灯的单词本"},
        {"高松灯的歌词本", "高松灯的单词本"}, {"高松灯的单词本", "高松灯的单词本"},
        {"Magic Key", "大黄门钥匙"}, {"万能钥匙", "大黄门钥匙"}, {"后台万能通行证", "大黄门钥匙"}, {"Holy Water", "立希水壶"}, {"圣水", "立希水壶"},
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
        QString::fromUtf8("屠龙匕"), QString::fromUtf8("屠龙匕首"), QString::fromUtf8("冰冻魔法"), QString::fromUtf8("冰冻徽章"), QString::fromUtf8("冷静雪花徽章"), QString::fromUtf8("飞行魔杖"),
        QString::fromUtf8("楼层传送器"),
        QString::fromUtf8("对称飞行器"), QString::fromUtf8("记事本"),
        QString::fromUtf8("怪物手册"), QString::fromUtf8("高松灯的单词本")
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
    if (iname == "Cross" || iname == "十字架" || iname == "MyGO和解徽章" || iname == "MyGO团结徽章") return std::make_unique<Cross>();
    if (iname == "Dragon Slayer" || iname == "屠龙匕" || iname == "屠龙匕首" || iname == "祥子指挥棒") return std::make_unique<DragonSlayer>();
    if (iname == "Freeze Magic" || iname == "冰冻魔法" || iname == "冰冻徽章" || iname == "冷静雪花徽章" || iname == "海铃冷静指令") return std::make_unique<FreezeMagic>();
    if (iname == "Flying Wand" || iname == "飞行魔杖" || iname == "爱音手机") return std::make_unique<FlyingWand>();
    if (iname == "Floor Teleporter" || iname == "楼层传送器") return std::make_unique<FloorTeleporter>();
    if (iname == "Symmetry Flyer" || iname == "对称飞行器" || iname == "Mujica镜面舞台票") return std::make_unique<SymmetryFlyer>();
    if (iname == "Monster Book" || iname == "怪物手册") return std::make_unique<MonsterBook>();
    if (iname == "Note Book" || iname == "记事本" || iname == "灯的歌词本" ||
        iname == "灯的单词本" || iname == "高松灯的歌词本" || iname == "高松灯的单词本")
        return std::make_unique<NoteBook>();
    if (iname == "Magic Key" || iname == "万能钥匙" || iname == "后台万能通行证" || iname == "大黄门钥匙")
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
    m_lastTeleportPath.clear();
    m_pendingTeleportPath.clear();
    m_pendingTeleportTargetX = -1;
    m_pendingTeleportTargetY = -1;
    m_lastTeleportNeedsAnimation = false;
    // 读档前清空运行时隐藏奖励，避免把上一个游戏实例的状态带入当前存档。
    m_floor35HiddenItems.clear();

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
    bool princessDollRescued = false;
    bool floor20VampireTriggered = false;
    bool floor20VampireStoryShown = false;
    bool michelleRescued = false;
    bool floor33TrapTriggered = false;
    bool floor38FlowerTriggered = false;
    bool floor43MiyakoRetreated = false;
    bool floor14RedKeyRewardGranted = false;
    bool floor32KnightTriggered = false;
    bool floor32KnightStoryPending = false;
    bool floor34RewardGranted = false;
    bool floor35RewardsHidden = false;
    bool floor40BossDefeated = false;
    bool floor40RewardsGranted = false;
    bool michelleGuardDefeated = false;
    bool floor42KnightStoryTriggered = false;
    bool floor42KnightStoryPending = false;
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
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> princessDollRescued;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor20VampireTriggered;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> michelleRescued;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor33TrapTriggered;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor38FlowerTriggered;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor43MiyakoRetreated;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor14RedKeyRewardGranted;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor32KnightTriggered;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor32KnightStoryPending;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor34RewardGranted;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor35RewardsHidden;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor40BossDefeated;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor40RewardsGranted;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> michelleGuardDefeated;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor20VampireStoryShown;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor42KnightStoryTriggered;
                if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                    ifs >> floor42KnightStoryPending;
            }
    }
    ifs >> invToken;
    }

    // HIDDEN35 是在 VISITED 之前加入的可选扩展块。旧存档没有该标记，
    // 此时 invToken 仍可能直接是 VISITED 或背包数量，保持向后兼容。
    if (invToken == "HIDDEN35") {
        size_t hiddenCount = 0;
        ifs >> hiddenCount;
        for (size_t i = 0; i < hiddenCount; ++i) {
            int key = -1;
            std::string itemName;
            int itemValue = 0;
            ifs >> key >> itemName >> itemValue;
            if (key >= 0 && key < m_width * m_height && !itemName.empty() && itemName != "-")
                m_floor35HiddenItems[key] = {itemName, itemValue};
        }
        ifs >> invToken;
    }

    m_visitedFloors.clear();
    if (invToken == "VISITED") {
        int visitedCount = 0;
        ifs >> visitedCount;
        for (int i = 0; i < visitedCount; ++i) {
            int visitedFloor = -1;
            ifs >> visitedFloor;
            if (visitedFloor >= 0 && visitedFloor <= 50)
                m_visitedFloors.insert(visitedFloor);
        }
        ifs >> invToken;
    }
    m_storyOnceKeys.clear();
    if (invToken == "STORIES") {
        int storyCount = 0;
        ifs >> storyCount;
        for (int i = 0; i < storyCount; ++i) {
            std::string key;
            ifs >> key;
            if (!key.empty()) m_storyOnceKeys.insert(key);
        }
        ifs >> invToken;
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
    m_princessDollRescued = princessDollRescued;
    m_floor20VampireTriggered = floor20VampireTriggered;
    m_floor20VampireStoryShown = floor20VampireStoryShown;
    m_michelleRescued = michelleRescued;
    m_floor33TrapTriggered = floor33TrapTriggered;
    m_floor38FlowerTriggered = floor38FlowerTriggered;
    m_floor43MiyakoRetreated = floor43MiyakoRetreated;
    m_floor14RedKeyRewardGranted = floor14RedKeyRewardGranted;
    m_floor32KnightTriggered = floor32KnightTriggered;
    m_floor32KnightStoryPending = floor32KnightStoryPending;
    m_floor34RewardGranted = floor34RewardGranted;
    m_floor35RewardsHidden = floor35RewardsHidden;
    m_floor40BossDefeated = floor40BossDefeated;
    m_floor40RewardsGranted = floor40RewardsGranted;
    m_michelleGuardDefeated = michelleGuardDefeated;
    m_floor42KnightStoryTriggered = floor42KnightStoryTriggered;
    m_floor42KnightStoryPending = floor42KnightStoryPending;
    // 兼容旧存档：若二层已经没有中级守卫，视为该前置战斗已完成。
    if (!m_michelleGuardDefeated) {
        const auto floor2It = m_floors.find(2);
        if (floor2It != m_floors.end()) {
            bool hasIntermediateGuard = false;
            for (const auto& entry : floor2It->second.monsters) {
                if (MonsterDB::indexOf(entry.second.GetName()) == 20) {
                    hasIntermediateGuard = true;
                    break;
                }
            }
            if (!hasIntermediateGuard) m_michelleGuardDefeated = true;
        }
    }
    // 兼容旧存档：20层入口已转为花门即代表吸血鬼事件已触发。
    auto floor20It = m_floors.find(20);
    if (floor20It != m_floors.end() &&
        floor20It->second.map[posKey(7, 10)] == Tile_DoorMagic)
        m_floor20VampireTriggered = true;
    // 兼容没有 EXTRA 扩展字段但已经保存隐藏楼梯的旧存档。
    auto floor24It = m_floors.find(24);
    if (floor24It != m_floors.end() &&
        floor24It->second.map[posKey(7, 8)] == Tile_StairsUp)
        m_princessDollRescued = true;
    m_floor10AmbushMonsterKeys.clear();
    m_floor10AmbushDoorKeys.clear();
    m_floor10AmbushMovements.clear();
    m_scriptedMonsterMovements.clear();

    for (int i = 0; i < invCount; ++i) {
        std::string iname; int ival;
        ifs >> iname >> ival;
        auto item = createItemByName(iname, ival);
        if (item) m_player.AddItem(std::move(item));
    }

    m_currentFloor = &m_floors[m_floor];
    if (m_visitedFloors.empty()) m_visitedFloors.insert(m_floor);
    return true;
}
