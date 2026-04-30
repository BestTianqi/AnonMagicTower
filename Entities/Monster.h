#pragma once

#include <string>

class Monster {
public:
    Monster(const std::string& name = "Monster", int hp = 30, int atk = 8, int def = 2, int gold = 0);

    const std::string& GetName() const { return m_name; }
    int GetHP()   const { return m_hp; }
    int GetATK()  const { return m_atk; }
    int GetDEF()  const { return m_def; }
    int GetGold() const { return m_gold; }

    int  TakeDamage(int dmg);
    bool IsDead() const { return m_hp <= 0; }
    int  Attack() const { return m_atk; }

private:
    std::string m_name;
    int m_hp;
    int m_atk;
    int m_def;
    int m_gold;
};
