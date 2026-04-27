#include "Items.h"
#include "Player.h"

// Item
Item::Item(const std::string& name, int value)
    : name(name), value(value)
{
}

Item::~Item() {}

std::string Item::GetName() const { return name; }
int Item::GetValue() const { return value; }

void Item::Apply(Player& /*player*/) const { /* default: nothing */ }

// Potion
Potion::Potion(int healAmount)
    : Item("Potion", healAmount), m_heal(healAmount)
{
}

void Potion::Apply(Player& player) const
{
    player.hp += m_heal;
}

// Weapon
Weapon::Weapon(int atkBonus)
    : Item("Weapon", atkBonus), m_atk(atkBonus)
{
}

void Weapon::Apply(Player& player) const
{
    player.atk += m_atk;
}

// Armor
Armor::Armor(int defBonus)
    : Item("Armor", defBonus), m_def(defBonus)
{
}

void Armor::Apply(Player& player) const
{
    player.def += m_def;
}

// Treasure
Treasure::Treasure(int gold)
    : Item("Treasure", gold), m_gold(gold)
{
}

void Treasure::Apply(Player& player) const
{
    player.gold += m_gold;
}

// Key
Key::Key(KeyType type)
    : Item(nameForType(type), valueForType(type)), m_type(type)
{
}

KeyType Key::GetType() const { return m_type; }

Key Key::CreateRed() const { return Key(KeyType::Red); }
Key Key::CreateBlue() const { return Key(KeyType::Blue); }
Key Key::CreateGreen() const { return Key(KeyType::Green); }

void Key::Apply(Player& player) const
{
    player.AddKey(m_type, 1);
}

std::string Key::nameForType(KeyType t) {
    switch (t) {
    case KeyType::Red: return "Red Key";
    case KeyType::Blue: return "Blue Key";
    case KeyType::Green: return "Green Key";
    default: return "Key";
    }
}

int Key::valueForType(KeyType t) {
    switch (t) {
    case KeyType::Red: return 1;
    case KeyType::Blue: return 2;
    case KeyType::Green: return 3;
    default: return 0;
    }
}