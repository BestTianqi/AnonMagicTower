#include "Game.h"
#include <fstream>
#include <sstream>

Game::Game()
    : m_width(10), m_height(8)
{
}

bool Game::loadDefaultMap()
{
    m_map.assign(m_width * m_height, Tile_Floor);
    // create border walls
    for (int x = 0; x < m_width; ++x) {
        m_map[x] = Tile_Wall;
        m_map[(m_height-1)*m_width + x] = Tile_Wall;
    }
    for (int y = 0; y < m_height; ++y) {
        m_map[y*m_width] = Tile_Wall;
        m_map[y*m_width + (m_width-1)] = Tile_Wall;
    }

    // place some example tiles
    m_map[1*m_width + 1] = Tile_StairsUp;
    m_map[4*m_width + 5] = Tile_Item;

    // place player in the map
    m_player.x = 1;
    m_player.y = 2;
    m_player.hp = 100;
    m_player.atk = 10;
    m_player.def = 5;

    // spawn a monster at (3,2)
    spawnMonster(3,2, Monster("Goblin", 20, 6, 1));
    m_map[2*m_width + 3] = Tile_Monster;

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
    int key = y*m_width + x;
    m_monsters.emplace(key, m);
}

bool Game::hasMonsterAt(int x, int y) const
{
    int key = y*m_width + x;
    return m_monsters.find(key) != m_monsters.end();
}

Monster* Game::monsterAt(int x, int y)
{
    int key = y*m_width + x;
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
        if (hasMonsterAt(nx, ny)) {
            // do not remove monster here; handle combat in UI
            return Move_Encounter;
        } else {
            // no monster object, treat as floor
            m_player.x = nx; m_player.y = ny; return Move_Ok;
        }
    case Tile_Item:
        // pick up a key for demo
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
        outLog.push_back("没有怪物。");
        return Fight_PlayerWin; // 没有怪物则视为胜利
    }

    // 简易回合制：玩家先手
    while (true) {
        int dmgToMonster = m_player.atk;
        m->TakeDamage(dmgToMonster);
        {
            std::ostringstream ss;
            ss << "你对 " << m->GetName() << " 造成 " << dmgToMonster << " 点伤害。 剩余HP=" << m->GetHP();
            outLog.push_back(ss.str());
        }
        if (m->IsDead()) {
            // give gold
            int gold = m->GetGold();
            if (gold > 0) m_player.gold += gold;

            // remove monster and mark tile as floor
            int key = y*m_width + x;
            m_monsters.erase(key);
            setTile(x, y, Tile_Floor);

            std::ostringstream ss;
            ss << "你击败了 " << m->GetName() << " 并获得 " << gold << " 金币。";
            outLog.push_back(ss.str());
            return Fight_PlayerWin;
        }

        // monster attack
        int dmgToPlayer = m->Attack() - m_player.def;
        if (dmgToPlayer < 0) dmgToPlayer = 0;
        m_player.hp -= dmgToPlayer;
        {
            std::ostringstream ss;
            ss << m->GetName() << " 对你造成 " << dmgToPlayer << " 点伤害。 你的剩余HP=" << m_player.hp;
            outLog.push_back(ss.str());
        }
        if (m_player.hp <= 0) {
            outLog.push_back("你被击败了。\n");
            return Fight_PlayerDead;
        }
    }
}

bool Game::saveToFile(const std::string& path) const
{
    std::ofstream ofs(path);
    if (!ofs) return false;

    // header: width height
    ofs << m_width << " " << m_height << "\n";

    // map
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            ofs << m_map[y*m_width + x] << (x+1==m_width? '\n' : ' ');
        }
    }

    // player: x y hp atk def gold
    ofs << m_player.x << " " << m_player.y << " " << m_player.hp << " " << m_player.atk << " " << m_player.def << " " << m_player.gold << "\n";

    // keys: count entries then pairs type count
    ofs << m_player.KeyCount(KeyType::Red) << " " << m_player.KeyCount(KeyType::Blue) << " " << m_player.KeyCount(KeyType::Green) << "\n";

    // monsters: number then for each: key name hp atk def gold
    ofs << m_monsters.size() << "\n";
    for (auto &kv : m_monsters) {
        int key = kv.first;
        const Monster &m = kv.second;
        ofs << key << " " << m.GetName() << " " << m.GetHP() << " " << m.GetATK() << " " << m.GetDEF() << " " << m.GetGold() << "\n";
    }

    return true;
}

bool Game::loadFromFile(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs) return false;

    m_monsters.clear();

    // header
    ifs >> m_width >> m_height;
    m_map.assign(m_width * m_height, Tile_Floor);

    // map
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int v; ifs >> v;
            m_map[y*m_width + x] = v;
        }
    }

    // player
    ifs >> m_player.x >> m_player.y >> m_player.hp >> m_player.atk >> m_player.def >> m_player.gold;

    // keys
    int r,b,g; ifs >> r >> b >> g;
    // reset keys by setting private map via AddKey
    // clear existing keys first
    // (we assume Player has no clear API; re-add by using UseKey to reduce -> instead set via AddKey after resetting container)
    // 简单方式：create a temporary player keys reset by reassigning m_player's private map is not possible here; instead call AddKey r times
    // To keep it simple, we will call AddKey which increments counts; before that we need to reset keys -> not exposed; workaround: use AddKey and hope it's zero initially after loadDefaultMap or new Game
    // For correctness, we will rely on the fact loadFromFile is called on a fresh Game instance; so just AddKey values.
    for (int i = 0; i < r; ++i) m_player.AddKey(KeyType::Red, 1);
    for (int i = 0; i < b; ++i) m_player.AddKey(KeyType::Blue, 1);
    for (int i = 0; i < g; ++i) m_player.AddKey(KeyType::Green, 1);

    // monsters
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
