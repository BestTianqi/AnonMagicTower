#pragma once

#include <string>

class Player; // 前向声明，避免循环包含

struct ClassicItemTier {
    int rubyAttack;
    int sapphireDefense;
    int smallPotionHp;
    int largePotionHp;
};

struct ClassicShopOffer {
    int hp;
    int atk;
    int def;
    int price;
};

// 按原版楼层段落计算四种基础道具的数值。
ClassicItemTier classicItemTierForFloor(int floor);
ClassicShopOffer classicShopOfferForFloor(int floor, int purchaseCount);

class Item {
public:
    Item(const std::string& name, int value);
    virtual ~Item();

    std::string GetName() const;
    int GetValue() const;

    // 应用道具效果到玩家（默认不做任何事）
    virtual void Apply(Player& player) const;

    // 是否放入背包手动使用（默认 false = 拾取即用）
    virtual bool IsUseItem() const { return false; }

    // 是否为原版中的可重复使用道具。可重复使用道具使用后仍留在背包。
    virtual bool IsReusable() const { return false; }

    // 是否被动永久效果（放入背包但不消耗，仅查看描述）
    virtual bool IsPassiveEffect() const { return false; }

private:
    std::string name;
    int value;
};

// 存档或自定义地图遇到未注册名称时使用的安全占位道具。
// 它不会产生任何效果，但会保留原始名称，便于后续补充道具定义。
class UnknownItem : public Item {
public:
    UnknownItem(const std::string& sourceName, int value)
        : Item("未知道具", value), m_sourceName(sourceName) {}
    const std::string& SourceName() const { return m_sourceName; }
    void Apply(Player& /*player*/) const override {}

private:
    std::string m_sourceName;
};

// 新增道具种类
enum class ItemType {
    Potion,
    Weapon,
    Armor,
    Treasure,
    RubyGem,
    SapphireGem,
    SmallPotion,
    LargePotion,
    Key,
    PenguinDoll,
    MatchaParfait,
    TempShield,
    StairUpper,
    StairLower,
    WallBreaker,
    MagicKey,
    AnonGlasses,
    LuckyCoin,
    Pickaxe,
    Bomb,
    EarthquakeScroll,
    Cross,
    DragonSlayer,
    FreezeMagic,
    FlyingWand,
    FloorTeleporter,
    SymmetryFlyer,
    MonsterBook,
    NoteBook,
    HolyShield,
    DivineShield
};

class Potion : public Item {
public:
    Potion(int healAmount, const std::string& displayName = "Potion");
    void Apply(Player& player) const override;
    int HealAmount() const { return m_heal; }
private:
    int m_heal;
};

// 原版50层魔塔固定数值药水：小血瓶+200，大血瓶+500。
class SmallPotion : public Potion {
public:
    explicit SmallPotion(int healAmount = 50, const std::string& displayName = "灯的热牛奶");
};

class LargePotion : public Potion {
public:
    explicit LargePotion(int healAmount = 200, const std::string& displayName = "爱音能量饮");
};

// 原版宝石拾取即生效：红宝石+3攻击，蓝宝石+3防御。
class RubyGem : public Item {
public:
    explicit RubyGem(int attackBonus = 1, const std::string& displayName = "MyGO应援红章");
    void Apply(Player& player) const override;
};

class SapphireGem : public Item {
public:
    explicit SapphireGem(int defenseBonus = 1, const std::string& displayName = "Mujica应援蓝章");
    void Apply(Player& player) const override;
};

class HolyWater : public Item {
public:
    HolyWater();
    void Apply(Player& player) const override;
    bool IsUseItem() const override { return true; }
};

class Weapon : public Item {
public:
    Weapon(int atkBonus, const std::string& displayName = "Weapon");
    void Apply(Player& player) const override;
    int AtkBonus() const { return m_atk; }
private:
    int m_atk;
};

class Armor : public Item {
public:
    Armor(int defBonus, const std::string& displayName = "Armor");
    void Apply(Player& player) const override;
    int DefBonus() const { return m_def; }
private:
    int m_def;
};

class Treasure : public Item {
public:
    Treasure(int gold);
    void Apply(Player& player) const override;
    int Gold() const { return m_gold; }
private:
    int m_gold;
};

enum class KeyType {
    Red,
    Blue,
    Green
};

class Key : public Item {
public:
    explicit Key(KeyType type);

    KeyType GetType() const;

    Key CreateRed() const;
    Key CreateBlue() const;
    Key CreateGreen() const;

    void Apply(Player& player) const override; // 使用时给予玩家钥匙

private:
    KeyType m_type;

    static std::string nameForType(KeyType t);
    static int valueForType(KeyType t);
};

class PenguinDoll : public Item {
public:
    PenguinDoll();
    void Apply(Player& player) const override;
    bool IsPassiveEffect() const override { return true; }
};

class MatchaParfait : public Item {
public:
    MatchaParfait();
    void Apply(Player& player) const override;
    bool IsPassiveEffect() const override { return true; }
};

class TempShield : public Item {
public:
    TempShield();
    void Apply(Player& player) const override;
    bool IsUseItem() const override { return true; }
};

class StairUpper : public Item {
public:
    StairUpper();
    void Apply(Player& player) const override;
    bool IsUseItem() const override { return true; }
};

class StairLower : public Item {
public:
    StairLower();
    void Apply(Player& player) const override;
    bool IsUseItem() const override { return true; }
};

class WallBreaker : public Item {
public:
    explicit WallBreaker(const std::string& displayName = "破墙锤");
    void Apply(Player& player) const override;
    bool IsUseItem() const override { return true; }
};

class MagicKey : public Item {
public:
    MagicKey();
    void Apply(Player& player) const override;
    bool IsUseItem() const override { return true; }
};

class AnonGlasses : public Item {
public:
    AnonGlasses();
    void Apply(Player& player) const override;
    bool IsPassiveEffect() const override { return true; }
};

class LuckyCoin : public Item {
public:
    LuckyCoin();
    void Apply(Player& player) const override;
    bool IsPassiveEffect() const override { return true; }
};

class Pickaxe : public WallBreaker {
public:
    Pickaxe();
};

class Bomb : public WallBreaker {
public:
    Bomb();
    void Apply(Player& player) const override;
};

class EarthquakeScroll : public WallBreaker {
public:
    EarthquakeScroll();
    void Apply(Player& player) const override;
};

class Cross : public Item {
public:
    Cross();
    void Apply(Player& player) const override;
    bool IsPassiveEffect() const override { return true; }
};

class DragonSlayer : public Item {
public:
    DragonSlayer();
    void Apply(Player& player) const override;
    bool IsPassiveEffect() const override { return true; }
};

class FreezeMagic : public Item {
public:
    FreezeMagic();
    void Apply(Player& player) const override;
    bool IsUseItem() const override { return true; }
    bool IsReusable() const override { return true; }
};

class FlyingWand : public Item {
public:
    FlyingWand();
    void Apply(Player& player) const override;
    bool IsUseItem() const override { return true; }
    bool IsReusable() const override { return true; }
};

// 楼层传送器：本地化的飞行魔杖变体，使用时由界面选择目标楼层。
class FloorTeleporter : public Item {
public:
    FloorTeleporter();
    void Apply(Player& player) const override;
    bool IsUseItem() const override { return true; }
    bool IsReusable() const override { return true; }
};

class SymmetryFlyer : public Item {
public:
    SymmetryFlyer();
    void Apply(Player& player) const override;
    bool IsUseItem() const override { return true; }
};

class NoteBook : public Item {
public:
    NoteBook();
    bool IsPassiveEffect() const override { return true; }
};

// 三层奖励：怪物手册。拾取后解锁地图上的怪物属性提示，保留在道具栏中。
class MonsterBook : public Item {
public:
    MonsterBook();
    void Apply(Player& player) const override;
    bool IsPassiveEffect() const override { return true; }
};

class HolyShield : public Armor {
public:
    HolyShield(int defBonus = 50, const std::string& displayName = "祥子黑色乐谱");
    void Apply(Player& player) const override;
};

class DivineShield : public HolyShield {
public:
    DivineShield(int defBonus = 100, const std::string& displayName = "Mujica终幕面具");
};
