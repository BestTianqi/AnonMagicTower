#include "Items.h"
#include "Player.h"
#include <QString>

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
    int amount = m_gold;
    if (player.hasLuckyCoin) amount *= 2;
    player.gold += amount;
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

PenguinDoll::PenguinDoll()
    : Item(QString::fromUtf8("企鹅玩偶").toStdString(), 0) {}

void PenguinDoll::Apply(Player& player) const {
    player.hasPenguinDoll = true;
}

MatchaParfait::MatchaParfait()
    : Item(QString::fromUtf8("抹茶芭菲").toStdString(), 0) {}

void MatchaParfait::Apply(Player& player) const {
    player.hasMatchaParfait = true;
}

AnonGlasses::AnonGlasses()
    : Item(QString::fromUtf8("匿名眼镜").toStdString(), 0) {}

void AnonGlasses::Apply(Player& player) const {
    player.hasGlasses = true;
}

TempShield::TempShield()
    : Item(QString::fromUtf8("临时护盾").toStdString(), 0) {}

void TempShield::Apply(Player& player) const {
    player.tempShieldCharges = 1;
}

StairUpper::StairUpper()
    : Item(QString::fromUtf8("上楼器").toStdString(), 0) {}

void StairUpper::Apply(Player& player) const {
    player.stairUpUsed = true;
}

StairLower::StairLower()
    : Item(QString::fromUtf8("下楼器").toStdString(), 0) {}

void StairLower::Apply(Player& player) const {
    player.stairDownUsed = true;
}

WallBreaker::WallBreaker()
    : Item(QString::fromUtf8("破墙锤").toStdString(), 0) {}

void WallBreaker::Apply(Player& player) const {
    player.wallBreakerUsed = true;
}

MagicKey::MagicKey()
    : Item(QString::fromUtf8("万能钥匙").toStdString(), 0) {}

void MagicKey::Apply(Player& player) const {
    player.magicKeyUses += 3;
}

LuckyCoin::LuckyCoin()
    : Item(QString::fromUtf8("幸运金币").toStdString(), 0) {}

void LuckyCoin::Apply(Player& player) const {
    player.hasLuckyCoin = true;
}
