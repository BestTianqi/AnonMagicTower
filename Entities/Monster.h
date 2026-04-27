#pragma once

#include <string>

class Monster {
public:
    Monster(const std::string& name = "Monster", int hp = 30, int atk = 8, int def = 2, int gold = 0);

    const std::string& GetName() const { return m_name; }
    int GetHP() const { return m_hp; }
    int GetATK() const { return m_atk; }
    int GetDEF() const { return m_def; }
    int GetGold() const { return m_gold; }

    // 受到伤害后返回剩余 hp
    int TakeDamage(int dmg);

    // 是否死亡
    bool IsDead() const { return m_hp <= 0; }

    // 攻击时造成的伤害（简单返回 atk，可扩展为计算伤害）
    int Attack() const { return m_atk; }

private:
    std::string m_name;
    int m_hp;
    int m_atk;
    int m_def;
    int m_gold;
};
