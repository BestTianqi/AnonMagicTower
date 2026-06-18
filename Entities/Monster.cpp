#include "Monster.h"

Monster::Monster(const std::string& name, int hp, int atk, int def, int gold)
    : m_name(name), m_hp(hp), m_atk(atk), m_def(def), m_gold(gold)
{
}

int Monster::TakeDamage(int dmg)
{
    int real = dmg - m_def;
    if (real < 0) real = 0;
    m_hp -= real;
    return m_hp;
}

int Monster::TakeDamageRaw(int dmg)
{
    m_hp -= dmg;
    return m_hp;
}
