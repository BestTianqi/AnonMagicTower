#include "Game.h"
#include <QString>
#include <fstream>
#include <sstream>

Game::Game()
    : m_width(MAP_SIZE), m_height(MAP_SIZE)
{
}

void Game::initFloor(int floor)
{
    if (m_floors.find(floor) == m_floors.end()) {
        FloorData fd;
        fd.map.assign(m_width * m_height, Tile_Floor);
        // 四周墙壁
        for (int x = 0; x < m_width; ++x) {
            fd.map[x]                      = Tile_Wall;
            fd.map[(m_height-1)*m_width + x] = Tile_Wall;
        }
        for (int y = 0; y < m_height; ++y) {
            fd.map[y*m_width]              = Tile_Wall;
            fd.map[y*m_width + (m_width-1)] = Tile_Wall;
        }
        m_floors[floor] = std::move(fd);
    }
    m_currentFloor = &m_floors[floor];
}

bool Game::loadDefaultMap()
{
    m_floor = 1;
    loadDefaultFloor1();
    loadDefaultFloor2();

    m_player.x   = 1;
    m_player.y   = 2;
    m_player.hp  = 100;
    m_player.atk = 10;
    m_player.def = 5;

    return true;
}

void Game::loadDefaultFloor1()
{
    initFloor(1);
    auto& f = *m_currentFloor;

    // 上楼点
    f.map[1*m_width + 1] = Tile_StairsUp;

    // 墙壁迷宫
    f.map[2*m_width + 4] = Tile_Wall;
    f.map[3*m_width + 4] = Tile_Wall;
    f.map[4*m_width + 4] = Tile_Wall;
    f.map[5*m_width + 2] = Tile_Wall;
    f.map[5*m_width + 3] = Tile_Wall;
    f.map[6*m_width + 6] = Tile_Wall;
    f.map[7*m_width + 6] = Tile_Wall;
    f.map[8*m_width + 6] = Tile_Wall;

    // 红门
    f.map[1*m_width + 5] = Tile_DoorRed;

    // 蓝门
    f.map[3*m_width + 2] = Tile_DoorBlue;

    // 绿门
    f.map[7*m_width + 3] = Tile_DoorGreen;

    // 道具
    addItemAt(2, 3, std::make_unique<Key>(KeyType::Red));
    f.map[3*m_width + 2] = Tile_Item;

    addItemAt(3, 5, std::make_unique<Potion>(50));
    f.map[5*m_width + 3] = Tile_Item;

    addItemAt(4, 1, std::make_unique<Weapon>(5));
    f.map[1*m_width + 4] = Tile_Item;

    addItemAt(5, 8, std::make_unique<Treasure>(10));
    f.map[8*m_width + 5] = Tile_Item;

    addItemAt(2, 7, std::make_unique<Key>(KeyType::Blue));
    f.map[7*m_width + 2] = Tile_Item;

    addItemAt(6, 1, std::make_unique<Armor>(3));
    f.map[1*m_width + 6] = Tile_Item;

    addItemAt(8, 1, std::make_unique<MagicKey>());
    f.map[1*m_width + 8] = Tile_Item;

    addItemAt(10, 1, std::make_unique<AnonGlasses>());
    f.map[1*m_width + 10] = Tile_Item;

    addItemAt(12, 3, std::make_unique<WallBreaker>());
    f.map[3*m_width + 12] = Tile_Item;

    addItemAt(8, 10, std::make_unique<TempShield>());
    f.map[10*m_width + 8] = Tile_Item;

    // 怪物
    spawnMonster(3, 3, Monster("要乐奈", 25, 8, 1, 3));
    f.map[3*m_width + 3] = Tile_Monster;

    spawnMonster(5, 5, Monster("若叶睦", 40, 12, 3, 6));
    f.map[5*m_width + 5] = Tile_Monster;

    spawnMonster(7, 1, Monster("丰川祥子", 50, 15, 5, 10));
    f.map[1*m_width + 7] = Tile_Monster;

    spawnMonster(10, 3, Monster("高松灯", 30, 18, 2, 8));
    f.map[3*m_width + 10] = Tile_Monster;

    spawnMonster(10, 8, Monster("椎名立希", 45, 22, 5, 12));
    f.map[8*m_width + 10] = Tile_Monster;

    spawnMonster(4, 10, Monster("祐天寺若麦", 60, 16, 10, 15));
    f.map[10*m_width + 4] = Tile_Monster;

    // NPC
    addNPCAt(12, 8, NPC("向导", {"欢迎来到魔塔！收集钥匙打开门，击败怪物提升实力。",
                                  "眼镜可以看穿怪物的属性。",
                                  "破墙锤可以摧毁一堵墙壁。"},
                         std::make_unique<Potion>(30)));
    f.map[8*m_width + 12] = Tile_NPC;

    addNPCAt(4, 6, NPC("商人", {"我可以卖给你一些装备。"},
                         std::make_unique<Weapon>(3)));
    f.map[6*m_width + 4] = Tile_NPC;
}

void Game::loadDefaultFloor2()
{
    initFloor(2);
    auto& f = *m_currentFloor;

    // 下楼点
    f.map[1*m_width + 1] = Tile_StairsDown;

    // 墙壁
    f.map[2*m_width + 3] = Tile_Wall;
    f.map[2*m_width + 5] = Tile_Wall;
    f.map[2*m_width + 7] = Tile_Wall;
    f.map[3*m_width + 3] = Tile_Wall;
    f.map[3*m_width + 5] = Tile_Wall;
    f.map[3*m_width + 7] = Tile_Wall;
    f.map[6*m_width + 2] = Tile_Wall;
    f.map[6*m_width + 3] = Tile_Wall;
    f.map[6*m_width + 4] = Tile_Wall;
    f.map[6*m_width + 5] = Tile_Wall;

    // 红门
    f.map[1*m_width + 7] = Tile_DoorRed;

    // 蓝门
    f.map[5*m_width + 7] = Tile_DoorBlue;

    // 道具
    addItemAt(2, 2, std::make_unique<Weapon>(8));
    f.map[2*m_width + 2] = Tile_Item;

    addItemAt(4, 4, std::make_unique<Armor>(5));
    f.map[4*m_width + 4] = Tile_Item;

    addItemAt(1, 10, std::make_unique<Potion>(100));
    f.map[10*m_width + 1] = Tile_Item;

    addItemAt(3, 10, std::make_unique<Treasure>(25));
    f.map[10*m_width + 3] = Tile_Item;

    addItemAt(10, 10, std::make_unique<StairLower>());
    f.map[10*m_width + 10] = Tile_Item;

    // 怪物
    spawnMonster(4, 2, Monster("三角初华", 75, 18, 10, 20));
    f.map[2*m_width + 4] = Tile_Monster;

    spawnMonster(8, 2, Monster("长崎素世", 55, 24, 5, 16));
    f.map[2*m_width + 8] = Tile_Monster;

    spawnMonster(8, 6, Monster("八幡海铃", 80, 16, 12, 18));
    f.map[6*m_width + 8] = Tile_Monster;

    spawnMonster(10, 6, Monster("大要乐奈", 100, 22, 12, 25));
    f.map[6*m_width + 10] = Tile_Monster;

    spawnMonster(6, 10, Monster("大高松灯", 70, 28, 8, 28));
    f.map[10*m_width + 6] = Tile_Monster;

    spawnMonster(12, 10, Monster("大椎名立希", 60, 33, 6, 30));
    f.map[10*m_width + 12] = Tile_Monster;
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

void Game::goUpFloor()
{
    m_floor++;
    if (m_floors.find(m_floor) == m_floors.end()) {
        // 自动生成新楼层
        initFloor(m_floor);
    }
    m_currentFloor = &m_floors[m_floor];
}

void Game::goDownFloor()
{
    if (m_floor <= 1) return;
    m_floor--;
    m_currentFloor = &m_floors[m_floor];
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

    case Tile_Floor:
        m_player.x = nx; m_player.y = ny;
        // 消耗移动类道具标记
        m_player.wallBreakerUsed = false;
        m_player.stairUpUsed = false;
        m_player.stairDownUsed = false;
        return Move_Ok;

    case Tile_DoorRed:
        if (m_player.HasKey(KeyType::Red)) {
            m_player.UseKey(KeyType::Red);
            setTile(nx, ny, Tile_Floor);
            m_player.x = nx; m_player.y = ny;
            return Move_Ok;
        }
        return Move_DoorLocked;

    case Tile_DoorBlue:
        if (m_player.HasKey(KeyType::Blue)) {
            m_player.UseKey(KeyType::Blue);
            setTile(nx, ny, Tile_Floor);
            m_player.x = nx; m_player.y = ny;
            return Move_Ok;
        }
        return Move_DoorLocked;

    case Tile_DoorGreen:
        if (m_player.HasKey(KeyType::Green)) {
            m_player.UseKey(KeyType::Green);
            setTile(nx, ny, Tile_Floor);
            m_player.x = nx; m_player.y = ny;
            return Move_Ok;
        }
        return Move_DoorLocked;

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
            item->Apply(m_player);
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

    while (true) {
        // 玩家攻击：atk - 怪物def (至少为0)
        int dmgToMonster = m_player.atk - m->GetDEF();
        if (dmgToMonster < 0) dmgToMonster = 0;
        m->TakeDamageRaw(dmgToMonster);
        {
            std::ostringstream ss;
            ss << "你对 " << m->GetName() << " 造成 " << dmgToMonster << " 点伤害。 剩余HP=" << m->GetHP();
            outLog.push_back(ss.str());
        }
        if (m->IsDead()) {
            int gold = m->GetGold();
            if (gold > 0) m_player.gold += gold;

            int key = posKey(x, y);
            m_currentFloor->monsters.erase(key);
            setTile(x, y, Tile_Floor);

            std::ostringstream ss;
            ss << "你击败了 " << m->GetName() << " 并获得 " << gold << " 金币。";
            outLog.push_back(ss.str());
            return Fight_PlayerWin;
        }

        // 怪物攻击：atk - 玩家def (至少为0)
        int dmgToPlayer = m->Attack() - m_player.def;
        if (dmgToPlayer < 0) dmgToPlayer = 0;
        m_player.hp -= dmgToPlayer;
        {
            std::ostringstream ss;
            ss << m->GetName() << " 对你造成 " << dmgToPlayer << " 点伤害。 你的剩余HP=" << m_player.hp;
            outLog.push_back(ss.str());
        }
        if (m_player.hp <= 0) {
            outLog.push_back(std::string("你被击败了。\n"));
            return Fight_PlayerDead;
        }
    }
}

bool Game::saveToFile(const std::string& path) const
{
    std::ofstream ofs(path);
    if (!ofs) return false;

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

        // NPC (仅保存对话，奖励物品通过物品系统保存)
        ofs << fd.npcs.size() << "\n";
        for (auto& nkv : fd.npcs) {
            int key = nkv.first;
            const NPC& n = nkv.second;
            int x = key % m_width;
            int y = key / m_width;
            ofs << x << " " << y << " " << n.GetName() << " "
                << n.HasGivenReward() << " "
                << n.Dialog().size() << "\n";
            for (auto& d : n.Dialog())
                ofs << d << "\n";
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

static std::unique_ptr<Item> createItemByName(const std::string& iname, int ival) {
    if (iname == "Red Key")
        return std::make_unique<Key>(KeyType::Red);
    if (iname == "Blue Key")
        return std::make_unique<Key>(KeyType::Blue);
    if (iname == "Green Key")
        return std::make_unique<Key>(KeyType::Green);
    if (iname == "Potion")
        return std::make_unique<Potion>(ival);
    if (iname == "Weapon")
        return std::make_unique<Weapon>(ival);
    if (iname == "Armor")
        return std::make_unique<Armor>(ival);
    if (iname == "Treasure")
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
    return nullptr;
}

bool Game::loadFromFile(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs) return false;

    m_floors.clear();

    ifs >> m_width >> m_height;
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
        size_t icount; ifs >> icount;
        for (size_t i = 0; i < icount; ++i) {
            int ix, iy; std::string iname; int ival;
            ifs >> ix >> iy >> iname >> ival;
            int key = iy * m_width + ix;
            auto item = createItemByName(iname, ival);
            if (item) fd.items[key] = std::move(item);
        }

        // 怪物
        size_t mcount; ifs >> mcount;
        for (size_t i = 0; i < mcount; ++i) {
            int key; std::string name; int hp, atk, def, gold;
            ifs >> key >> name >> hp >> atk >> def >> gold;
            fd.monsters.emplace(key, Monster(name, hp, atk, def, gold));
        }

        // NPC
        size_t ncount; ifs >> ncount;
        for (size_t i = 0; i < ncount; ++i) {
            int nx, ny; std::string nname; bool given; size_t dsize;
            ifs >> nx >> ny >> nname >> given >> dsize;
            ifs.ignore();
            std::vector<std::string> dialog;
            for (size_t d = 0; d < dsize; ++d) {
                std::string line;
                std::getline(ifs, line);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                dialog.push_back(line);
            }
            int key = ny * m_width + nx;
            auto npc = NPC(nname, dialog, nullptr);
            npc.SetGiven(given);
            fd.npcs.emplace(key, std::move(npc));
        }

        m_floors[fnum] = std::move(fd);
    }

    // 玩家 — 先保存已读数据
    int px, py, php, patk, pdef, pgold;
    ifs >> px >> py >> php >> patk >> pdef >> pgold;

    int r, b, g; ifs >> r >> b >> g;
    bool hasGl, hasPen, hasMat;
    ifs >> hasGl >> hasPen >> hasMat;

    int invCount;
    ifs >> invCount;

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

    for (int i = 0; i < invCount; ++i) {
        std::string iname; int ival;
        ifs >> iname >> ival;
        auto item = createItemByName(iname, ival);
        if (item) m_player.AddItem(std::move(item));
    }

    m_currentFloor = &m_floors[m_floor];
    return true;
}
