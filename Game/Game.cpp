#include "Game.h"
#include <fstream>
#include <sstream>

Game::Game()
    : m_width(MAP_SIZE), m_height(MAP_SIZE)
{
}

bool Game::loadDefaultMap()
{
    int w = m_width;
    int h = m_height;
    m_map.assign(w * h, Tile_Floor);

    // 四周墙壁
    for (int x = 0; x < w; ++x) {
        m_map[x]               = Tile_Wall;               // 上边
        m_map[(h-1)*w + x]     = Tile_Wall;               // 下边
    }
    for (int y = 0; y < h; ++y) {
        m_map[y*w]             = Tile_Wall;               // 左边
        m_map[y*w + (w-1)]     = Tile_Wall;               // 右边
    }

    // 上楼点
    m_map[1*w + 1] = Tile_StairsUp;

    // 道具
    m_map[2*w + 3] = Tile_Item;
    m_map[3*w + 5] = Tile_Item;

    // 怪物
    spawnMonster(3, 2, Monster("要乐奈", 25, 8, 1, 3));
    m_map[2*w + 3] = Tile_Monster;

    spawnMonster(5, 3, Monster("若叶睦", 40, 12, 3, 6));
    m_map[3*w + 5] = Tile_Monster;

    spawnMonster(7, 4, Monster("丰川祥子", 50, 15, 5, 10));
    m_map[4*w + 7] = Tile_Monster;

    spawnMonster(10, 6, Monster("高松灯", 30, 18, 2, 8));
    m_map[6*w + 10] = Tile_Monster;

    // 道具 (覆盖之前怪物占用的位置需要重新设置)
    m_map[5*w + 8] = Tile_Item;

    // 玩家
    m_player.x   = 1;
    m_player.y   = 2;
    m_player.hp  = 100;
    m_player.atk = 10;
    m_player.def = 5;

    return true;
}

int Game::tileAt(int x, int y) const
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height) return Tile_Empty;
    return m_map[y * m_width + x];
}

void Game::setTile(int x, int y, int tile)
{
    if (x < 0 || y < 0 || x >= m_width || y >= m_height) return;
    m_map[y * m_width + x] = tile;
}

void Game::spawnMonster(int x, int y, const Monster& m)
{
    int key = y * m_width + x;
    m_monsters.emplace(key, m);
}

bool Game::hasMonsterAt(int x, int y) const
{
    int key = y * m_width + x;
    return m_monsters.find(key) != m_monsters.end();
}

Monster* Game::monsterAt(int x, int y)
{
    int key = y * m_width + x;
    auto it = m_monsters.find(key);
    if (it == m_monsters.end()) return nullptr;
    return &it->second;
}

Game::MoveResult Game::tryMovePlayer(int nx, int ny)
{
    if (nx < 0 || ny < 0 || nx >= m_width || ny >= m_height) return Move_Block;
    int tile = tileAt(nx, ny);
    switch (tile) {
    case Tile_Wall:
        return Move_Block;
    case Tile_Floor:
        m_player.x = nx; m_player.y = ny; return Move_Ok;
    case Tile_Monster:
        if (hasMonsterAt(nx, ny))
            return Move_Encounter;
        else {
            m_player.x = nx; m_player.y = ny;
            return Move_Ok;
        }
    case Tile_Item:
        m_player.AddKey(KeyType::Red, 1);
        setTile(nx, ny, Tile_Floor);
        m_player.x = nx; m_player.y = ny;
        return Move_Pickup;
    case Tile_StairsUp:
        m_player.x = nx; m_player.y = ny; return Move_StairsUp;
    case Tile_StairsDown:
        m_player.x = nx; m_player.y = ny; return Move_StairsDown;
    default:
        m_player.x = nx; m_player.y = ny; return Move_Ok;
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
        int dmgToMonster = m_player.atk;
        m->TakeDamage(dmgToMonster);
        {
            std::ostringstream ss;
            ss << "你对 " << m->GetName() << " 造成 " << dmgToMonster << " 点伤害。 剩余HP=" << m->GetHP();
            outLog.push_back(ss.str());
        }
        if (m->IsDead()) {
            int gold = m->GetGold();
            if (gold > 0) m_player.gold += gold;

            int key = y * m_width + x;
            m_monsters.erase(key);
            setTile(x, y, Tile_Floor);

            std::ostringstream ss;
            ss << "你击败了 " << m->GetName() << " 并获得 " << gold << " 金币。";
            outLog.push_back(ss.str());
            return Fight_PlayerWin;
        }

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

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            ofs << m_map[y*m_width + x] << (x+1 == m_width ? '\n' : ' ');
        }
    }

    ofs << m_player.x << " " << m_player.y << " "
        << m_player.hp << " " << m_player.atk << " "
        << m_player.def << " " << m_player.gold << "\n";

    ofs << m_player.KeyCount(KeyType::Red) << " "
        << m_player.KeyCount(KeyType::Blue) << " "
        << m_player.KeyCount(KeyType::Green) << "\n";

    ofs << m_monsters.size() << "\n";
    for (auto& kv : m_monsters) {
        int key = kv.first;
        const Monster& m = kv.second;
        ofs << key << " " << m.GetName() << " "
            << m.GetHP() << " " << m.GetATK() << " "
            << m.GetDEF() << " " << m.GetGold() << "\n";
    }

    return true;
}

bool Game::loadFromFile(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs) return false;

    m_monsters.clear();

    ifs >> m_width >> m_height;
    m_map.assign(m_width * m_height, Tile_Floor);

    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int v; ifs >> v;
            m_map[y*m_width + x] = v;
        }
    }

    ifs >> m_player.x >> m_player.y
        >> m_player.hp >> m_player.atk
        >> m_player.def >> m_player.gold;

    int r, b, g; ifs >> r >> b >> g;
    for (int i = 0; i < r; ++i) m_player.AddKey(KeyType::Red, 1);
    for (int i = 0; i < b; ++i) m_player.AddKey(KeyType::Blue, 1);
    for (int i = 0; i < g; ++i) m_player.AddKey(KeyType::Green, 1);

    size_t mcount; ifs >> mcount;
    for (size_t i = 0; i < mcount; ++i) {
        int key; std::string name; int hp, atk, def, gold;
        ifs >> key >> name >> hp >> atk >> def >> gold;
        int x = key % m_width;
        int y = key / m_width;
        spawnMonster(x, y, Monster(name, hp, atk, def, gold));
    }

    return true;
}
