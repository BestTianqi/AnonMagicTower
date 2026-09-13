#include "Items.h"
#include "Player.h"
#include <QString>
#include <algorithm>

ClassicItemTier classicItemTierForFloor(int floor)
{
    const int clamped = std::max(1, std::min(50, floor));
    const int tier = ((clamped - 1) / 10) + 1;
    return {tier, tier, tier * 50, tier * 200};
}

ClassicShopOffer classicShopOfferForFloor(int floor, int purchaseCount)
{
    const int safeFloor = std::max(1, std::min(50, floor));
    const int priceCount = std::max(0, purchaseCount);
    int atk = 2, def = 4;
    if (safeFloor >= 46) { atk = 10; def = 20; }
    else if (safeFloor >= 32) { atk = 8; def = 16; }
    else if (safeFloor >= 12) { atk = 4; def = 8; }
    // 原版商店价格由全局购买次数共享：20、40、80、140。
    // 楼层只影响本次提供的属性数值，不影响价格序列。
    static constexpr int prices[] = {20, 40, 80, 140};
    const int price = prices[std::min(priceCount, 3)];
    return {100, atk, def, price};
}

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
Potion::Potion(int healAmount, const std::string& displayName)
    : Item(displayName, healAmount), m_heal(healAmount)
{
}

void Potion::Apply(Player& player) const
{
    player.hp += m_heal;
}

SmallPotion::SmallPotion(int healAmount, const std::string& displayName)
    : Potion(healAmount, displayName)
{
}

LargePotion::LargePotion(int healAmount, const std::string& displayName)
    : Potion(healAmount, displayName)
{
}

RubyGem::RubyGem(int attackBonus, const std::string& displayName)
    : Item(displayName, attackBonus)
{
}

void RubyGem::Apply(Player& player) const
{
    player.atk += GetValue();
}

SapphireGem::SapphireGem(int defenseBonus, const std::string& displayName)
    : Item(displayName, defenseBonus)
{
}

void SapphireGem::Apply(Player& player) const
{
    player.def += GetValue();
}

HolyWater::HolyWater()
    : Item(QString::fromUtf8("立希水壶").toStdString(), 0)
{
}

void HolyWater::Apply(Player& player) const
{
    player.hp += player.atk + player.def;
}

// Weapon
Weapon::Weapon(int atkBonus, const std::string& displayName)
    : Item(displayName, atkBonus), m_atk(atkBonus)
{
}

void Weapon::Apply(Player& player) const
{
    player.atk += m_atk;
}

// Armor
Armor::Armor(int defBonus, const std::string& displayName)
    : Item(displayName, defBonus), m_def(defBonus)
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
    case KeyType::Red: return QString::fromUtf8("红色Live票").toStdString();
    case KeyType::Blue: return QString::fromUtf8("蓝色Live票").toStdString();
    case KeyType::Green: return QString::fromUtf8("黄色Live票").toStdString();
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
    : Item(QString::fromUtf8("立希企鹅挂件").toStdString(), 0) {}

void PenguinDoll::Apply(Player& player) const {
    player.hasPenguinDoll = true;
}

MatchaParfait::MatchaParfait()
    : Item(QString::fromUtf8("乐奈抹茶芭菲").toStdString(), 0) {}

void MatchaParfait::Apply(Player& player) const {
    player.hasMatchaParfait = true;
}

AnonGlasses::AnonGlasses()
    : Item(QString::fromUtf8("爱音自拍眼镜").toStdString(), 0) {}

void AnonGlasses::Apply(Player& player) const {
    player.hasGlasses = true;
}

TempShield::TempShield()
    : Item(QString::fromUtf8("乐队护盾贴").toStdString(), 0) {}

void TempShield::Apply(Player& player) const {
    player.tempShieldCharges = 1;
}

StairUpper::StairUpper()
    : Item(QString::fromUtf8("舞台升降卡").toStdString(), 0) {}

void StairUpper::Apply(Player& player) const {
    player.stairUpUsed = true;
}

StairLower::StairLower()
    : Item(QString::fromUtf8("撤场通行卡").toStdString(), 0) {}

void StairLower::Apply(Player& player) const {
    player.stairDownUsed = true;
}

WallBreaker::WallBreaker(const std::string& displayName)
    : Item(displayName, 0) {}

void WallBreaker::Apply(Player& player) const {
    player.wallBreakerUsed = true;
}

MagicKey::MagicKey()
    : Item(QString::fromUtf8("后台万能通行证").toStdString(), 0) {}

void MagicKey::Apply(Player& player) const {
    player.magicKeyUses += 3;
}

LuckyCoin::LuckyCoin()
    : Item(QString::fromUtf8("乐奈幸运硬币").toStdString(), 0) {}

void LuckyCoin::Apply(Player& player) const {
    player.hasLuckyCoin = true;
}

Pickaxe::Pickaxe()
    : WallBreaker(QString::fromUtf8("睦的镐子").toStdString()) {}

Bomb::Bomb()
    : WallBreaker(QString::fromUtf8("Mujica烟雾弹").toStdString()) {}

void Bomb::Apply(Player& /*player*/) const {}

EarthquakeScroll::EarthquakeScroll()
    : WallBreaker(QString::fromUtf8("Mujica舞台震响卷").toStdString()) {}

void EarthquakeScroll::Apply(Player& /*player*/) const {}

void Cross::Apply(Player& player) const {
    player.hasCross = true;
}

Cross::Cross()
    : Item(QString::fromUtf8("MyGO和解徽章").toStdString(), 0) {}

DragonSlayer::DragonSlayer()
    : Item(QString::fromUtf8("祥子指挥棒").toStdString(), 0) {}

void DragonSlayer::Apply(Player& player) const {
    player.hasDragonSlayer = true;
}

FreezeMagic::FreezeMagic()
    : Item(QString::fromUtf8("海铃冷静指令").toStdString(), 0) {}

void FreezeMagic::Apply(Player& player) const {
    player.freezeMagicUsed = true;
}

FlyingWand::FlyingWand()
    : Item(QString::fromUtf8("爱音手机").toStdString(), 0) {}

void FlyingWand::Apply(Player& player) const {
    ++player.flyWandUses;
}

FloorTeleporter::FloorTeleporter()
    : Item(QString::fromUtf8("楼层传送器").toStdString(), 0) {}

void FloorTeleporter::Apply(Player& player) const {
    ++player.flyWandUses;
}

SymmetryFlyer::SymmetryFlyer()
    : Item(QString::fromUtf8("Mujica镜面舞台票").toStdString(), 3) {}

void SymmetryFlyer::Apply(Player& player) const {
    player.symmetryFlyerUses += 3;
}

NoteBook::NoteBook()
    : Item(QString::fromUtf8("灯的歌词本").toStdString(), 0) {}

HolyShield::HolyShield(int defBonus, const std::string& displayName)
    : Armor(defBonus, displayName) {}

void HolyShield::Apply(Player& player) const {
    Armor::Apply(player);
    player.hasHolyShield = true;
}

DivineShield::DivineShield(int defBonus, const std::string& displayName)
    : HolyShield(defBonus, displayName) {}
