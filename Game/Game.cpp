#include "Game.h"
#include "MapData.h"
#include <QString>
#include <algorithm>
#include <fstream>
#include <sstream>

Game::Game()
    : m_width(MAP_SIZE), m_height(MAP_SIZE)
{
    initFloor(1);
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
    // 优先加载用户设计的默认地图
    {
        std::ifstream check("default_map.txt");
        if (check.good()) {
            check.close();
            if (loadFromFile("default_map.txt"))
                return true;
        }
    }

    m_floor = 1;
    MapData::loadAllFloors(*this);
    m_currentFloor = &m_floors[1];

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
        m_currentFloor->map[idx] = Tile_Floor;
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

    case Tile_DarkWall:
        setTile(nx, ny, Tile_Floor);
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
            setTile(x, y, Tile_Floor);

            std::ostringstream ss;
            ss << "你击败了 " << bossName << " 并获得 " << gold << " 金币。";
            outLog.push_back(ss.str());
            if (hasShield) m_player.tempShieldCharges--;
            if (bossName == "长崎素世")
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
                << n.IsTradeDone() << "\n";
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
                << s.armorValue << "\n";
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
    if (iname == "Green Key" || iname == QString::fromUtf8("绿钥匙").toStdString())
        return std::make_unique<Key>(KeyType::Green);
    if (iname == "Potion" || iname == QString::fromUtf8("生命药").toStdString())
        return std::make_unique<Potion>(ival);
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
            auto npc = NPC(nname, dialog, std::move(reward), isTrader, tradeGoldCost, std::move(tradeReward));
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
                fd.shops.emplace(key, ShopData{pp, wp, ap, pv, wv, av});
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
            int fnum, sx, sy, pp, wp, ap, pv = 200, wv = 5, av = 8;
            ifs >> fnum >> sx >> sy >> pp >> wp >> ap;
            if (ifs.peek() != '\n' && ifs.peek() != EOF)
                ifs >> pv >> wv >> av;
            auto it = m_floors.find(fnum);
            if (it != m_floors.end())
                it->second.shops.emplace(sy * m_width + sx, ShopData{pp, wp, ap, pv, wv, av});
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
