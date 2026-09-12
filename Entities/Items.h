#pragma once

#include <string>

class Player; // 前向声明，避免循环包含

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

    // 是否被动永久效果（放入背包但不消耗，仅查看描述）
    virtual bool IsPassiveEffect() const { return false; }

private:
    std::string name;
    int value;
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
    LuckyCoin
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
    SmallPotion();
};

class LargePotion : public Potion {
public:
    LargePotion();
};

// 原版宝石拾取即生效：红宝石+3攻击，蓝宝石+3防御。
class RubyGem : public Item {
public:
    RubyGem();
    void Apply(Player& player) const override;
};

class SapphireGem : public Item {
public:
    SapphireGem();
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
    Weapon(int atkBonus);
    void Apply(Player& player) const override;
    int AtkBonus() const { return m_atk; }
private:
    int m_atk;
};

class Armor : public Item {
public:
    Armor(int defBonus);
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
    WallBreaker();
    void Apply(Player& player) const override;
    bool IsUseItem() const override { return true; }
};

class MagicKey : public Item {
public:
    MagicKey();
    void Apply(Player& player) const override;
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
