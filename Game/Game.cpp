#include "Game.h"
#include "Entities/MonsterDB.h"
#include <QString>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <algorithm>
#include <fstream>
#include <sstream>

Game::Game()
    : m_width(MAP_SIZE), m_height(MAP_SIZE)
{
    initFloor(1);
}

void Game::generateClassicTower()
{
    m_floors.clear();
    m_floor = 1;
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
        case 10: return std::make_unique<AnonGlasses>();
        case 13: return std::make_unique<WallBreaker>();
        case 17: return std::make_unique<HolyWater>();
        case 18: return std::make_unique<LuckyCoin>();
        case 21: return std::make_unique<StairUpper>();
        case 22: return std::make_unique<StairLower>();
        case 24: return std::make_unique<Weapon>(10);
        case 25: return std::make_unique<Armor>(10);
        case 26: return std::make_unique<Weapon>(20);
        case 27: return std::make_unique<Armor>(20);
        case 28: return std::make_unique<Weapon>(40);
        case 29: return std::make_unique<Armor>(40);
        case 30: return std::make_unique<Weapon>(50);
        case 31: return std::make_unique<Armor>(50);
        case 32: return std::make_unique<Weapon>(100);
        case 33: return std::make_unique<Armor>(100);
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
        const int key = y * m_width + x;

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
                fd.shops[key] = shop;
            } else {
                fd.map[key] = Tile_NPC;
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
                if (id == 3) reward = std::make_unique<AnonGlasses>();
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

    m_currentFloor = &m_floors[m_floor];
    m_player = Player();
    m_player.x = 7;
    m_player.y = 12;
    m_player.hp = 1000;
    m_player.atk = 10;
    m_player.def = 10;
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
}

bool Game::loadDefaultMap()
{
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
        if (loadFromFile(tmpPath.toStdString()))
            return true;
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

bool Game::breakWall(int x, int y)
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height) return false;
    int idx = y * m_width + x;
    if (m_currentFloor->map[idx] == Tile_Wall) {
        m_currentFloor->map[idx] = m_currentFloor->items.count(idx) ? Tile_Item : Tile_Floor;
        return true;
    }
    return false;
}

void Game::goUpFloor(int srcX, int srcY, bool findStairs)
{
    m_floor++;
    if (m_floors.find(m_floor) == m_floors.end()) {
        initFloor(m_floor);
    }
    m_currentFloor = &m_floors[m_floor];

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
    if (nx < 0 || ny < 0 || nx >= m_width || ny >= m_height) return Move_Block;
    int tile = tileAt(nx, ny);

    switch (tile) {
    case Tile_Wall:
        // 检查破墙锤
        if (m_player.wallBreakerUsed) {
            if (breakWall(nx, ny)) {
                m_player.wallBreakerUsed = false;
                m_player.x = nx; m_player.y = ny;
                return Move_Ok;
            }
        }
        return Move_Block;

    case Tile_Lava:
    case Tile_StarRiver:
        return Move_Block;

    case Tile_DoorMagic:
    case Tile_DoorIron:
        // 原作由楼层剧情开启；当前事件兼容层在本层敌人清空后放行。
        if (!m_currentFloor->monsters.empty()) return Move_DoorLocked;
        setTile(nx, ny, Tile_Floor);
        m_player.x = nx; m_player.y = ny;
        return Move_Ok;

    case Tile_DarkWall:
        setTile(nx, ny, m_currentFloor->items.count(posKey(nx, ny)) ? Tile_Item : Tile_Floor);
        m_player.x = nx; m_player.y = ny;
        return Move_Ok;

    case Tile_Floor:
        m_player.x = nx; m_player.y = ny;
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
        return Move_Ok;

    case Tile_Monster:
        if (hasMonsterAt(nx, ny))
            return Move_Encounter;
        else {
            m_player.x = nx; m_player.y = ny;
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
            return Move_Pickup;
        }
        setTile(nx, ny, Tile_Floor);
        m_player.x = nx; m_player.y = ny;
        return Move_Ok;
    }

    case Tile_NPC:
        return Move_NPC;

    case Tile_Shop:
        return Move_Shop;

    case Tile_StairsUp:
        m_player.x = nx; m_player.y = ny;
        return Move_StairsUp;

    case Tile_StairsDown:
        m_player.x = nx; m_player.y = ny;
        return Move_StairsDown;

    default:
        m_player.x = nx; m_player.y = ny;
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
    bool hasShield = (m_player.tempShieldCharges > 0);
    int  shieldBonus = hasShield ? 50 : 0;

    while (true) {
        // 玩家攻击：atk - 怪物def (至少为0)
        int dmgToMonster = m_player.atk - m->GetDEF();
        if (dmgToMonster < 0) dmgToMonster = 0;

        // 怪物攻击：atk - 玩家def - 护盾 (至少为0)
        int dmgToPlayer = m->Attack() - m_player.def - shieldBonus;
        if (dmgToPlayer < 0) dmgToPlayer = 0;
        if (m_player.hasPenguinDoll && (bossName == "高松灯" || bossName == "企鹅"))
            dmgToPlayer /= 2;
        if (m_player.hasMatchaParfait && (bossName == "要乐奈" || bossName == "小猫"))
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

            if (m_floor == 49 && bossName == "丰川祥子·魔法警卫") {
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
                        Monster("长崎素世·魔王幻影", 800, 500, 100, 500);
                    outLog.push_back("四名魔法警卫形成的封印生效，长崎素世·魔王幻影的属性降为原来的十分之一！");
                }
            }
            if (m_floor == 49 && bossName == "长崎素世·魔王幻影") {
                m_currentFloor->monsters.clear();
                for (int& tile : m_currentFloor->map)
                    if (tile == Tile_Monster) tile = Tile_Floor;
            }

            std::ostringstream ss;
            ss << "你击败了 " << bossName << " 并获得 " << gold << " 金币。";
            outLog.push_back(ss.str());
            if (hasShield) m_player.tempShieldCharges--;
            if (bossName == "长崎素世·魔王本体")
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
            ofs << x << " " << y << " " << item->GetName() << " " << item->GetValue() << "\n";
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
                << (reward ? reward->GetName() : "-") << " "
                << (reward ? reward->GetValue() : 0) << " "
                << n.IsTrader() << " " << n.GetTradeGoldCost() << " "
                << (tradeReward ? tradeReward->GetName() : "-") << " "
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
                << s.armorValue << " " << s.classicNpcId << "\n";
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
        << m_player.magicKeyUses << " " << m_player.tempShieldCharges << "\n";

    // 背包物品
    ofs << m_player.InventoryCount() << "\n";
    for (int i = 0; i < m_player.InventoryCount(); ++i) {
        auto* item = m_player.GetItem(i);
        if (item) {
            ofs << item->GetName() << " " << item->GetValue() << "\n";
        }
    }

    return true;
}

std::unique_ptr<Item> Game::createItemByName(const std::string& iname, int ival) {
    if (iname == "Red Key" || iname == QString::fromUtf8("红钥匙").toStdString())
        return std::make_unique<Key>(KeyType::Red);
    if (iname == "Blue Key" || iname == QString::fromUtf8("蓝钥匙").toStdString())
        return std::make_unique<Key>(KeyType::Blue);
    if (iname == "Green Key" || iname == QString::fromUtf8("绿钥匙").toStdString() ||
        iname == "Yellow Key" || iname == QString::fromUtf8("黄钥匙").toStdString())
        return std::make_unique<Key>(KeyType::Green);
    if (iname == "Potion" || iname == QString::fromUtf8("生命药").toStdString())
        return std::make_unique<Potion>(ival);
    if (iname == "Small Potion" || iname == QString::fromUtf8("小血瓶").toStdString())
        return std::make_unique<SmallPotion>(ival);
    if (iname == "Large Potion" || iname == QString::fromUtf8("大血瓶").toStdString())
        return std::make_unique<LargePotion>(ival);
    if (iname == "Ruby Gem" || iname == QString::fromUtf8("红宝石").toStdString())
        return std::make_unique<RubyGem>(ival);
    if (iname == "Sapphire Gem" || iname == QString::fromUtf8("蓝宝石").toStdString())
        return std::make_unique<SapphireGem>(ival);
    if (iname == "Weapon" || iname == QString::fromUtf8("武器").toStdString())
        return std::make_unique<Weapon>(ival);
    if (iname == "Armor" || iname == QString::fromUtf8("防具").toStdString())
        return std::make_unique<Armor>(ival);
    if (iname == "Treasure" || iname == QString::fromUtf8("金币").toStdString())
        return std::make_unique<Treasure>(ival);
    if (iname == QString::fromUtf8("万能钥匙").toStdString())
        return std::make_unique<MagicKey>();
    if (iname == QString::fromUtf8("匿名眼镜").toStdString())
        return std::make_unique<AnonGlasses>();
    if (iname == QString::fromUtf8("破墙锤").toStdString())
        return std::make_unique<WallBreaker>();
    if (iname == QString::fromUtf8("上楼器").toStdString())
        return std::make_unique<StairUpper>();
    if (iname == QString::fromUtf8("下楼器").toStdString())
        return std::make_unique<StairLower>();
    if (iname == QString::fromUtf8("临时护盾").toStdString())
        return std::make_unique<TempShield>();
    if (iname == QString::fromUtf8("企鹅玩偶").toStdString())
        return std::make_unique<PenguinDoll>();
    if (iname == QString::fromUtf8("抹茶芭菲").toStdString())
        return std::make_unique<MatchaParfait>();
    if (iname == QString::fromUtf8("幸运金币").toStdString())
        return std::make_unique<LuckyCoin>();
    if (iname == QString::fromUtf8("圣水").toStdString())
        return std::make_unique<HolyWater>();
    return nullptr;
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
            int fnum, sx, sy, pp, wp, ap, pv = 200, wv = 5, av = 8, classicNpcId = 0;
            ifs >> fnum >> sx >> sy >> pp >> wp >> ap;
            if (ifs.peek() != '\n' && ifs.peek() != EOF)
                ifs >> pv >> wv >> av;
            if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF)
                ifs >> classicNpcId;
            auto it = m_floors.find(fnum);
            if (it != m_floors.end()) {
                ShopData shop{pp, wp, ap, pv, wv, av};
                shop.classicNpcId = classicNpcId;
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
    std::string invToken;
    ifs >> invToken;
    if (invToken == "EXTRA") {
        ifs >> hasLc >> shopUse >> wbUsed >> suUsed >> sdUsed;
        // v3 扩展: 万能钥匙 + 临时护盾次数
        if (ifs.peek() != '\n' && ifs.peek() != '\r' && ifs.peek() != EOF) {
            ifs >> mkUses >> tsc;
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

    for (int i = 0; i < invCount; ++i) {
        std::string iname; int ival;
        ifs >> iname >> ival;
        auto item = createItemByName(iname, ival);
        if (item) m_player.AddItem(std::move(item));
    }

    m_currentFloor = &m_floors[m_floor];
    return true;
}
