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
    Key,
    PenguinDoll,
    MatchaParfait,
    TempShield,
    StairUpper,
    StairLower,
    WallBreaker,
    MagicKey,
    AnonGlasses
};

class Potion : public Item {
public:
    Potion(int healAmount);
    void Apply(Player& player) const override;
    int HealAmount() const { return m_heal; }
private:
    int m_heal;
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
};

class MatchaParfait : public Item {
public:
    MatchaParfait();
    void Apply(Player& player) const override;
};

class TempShield : public Item {
public:
    TempShield();
    void Apply(Player& player) const override;
private:
};

class StairUpper : public Item {
public:
    StairUpper();
    void Apply(Player& player) const override;
};

class StairLower : public Item {
public:
    StairLower();
    void Apply(Player& player) const override;
};

class WallBreaker : public Item {
public:
    WallBreaker();
    void Apply(Player& player) const override;
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
};
