#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "Items.h"

class Player {
public:
    Player();

    int x;
    int y;
    int hp;
    int atk;
    int def;
    int gold;
    bool hasGlasses = false;
    bool hasPenguinDoll = false;
    bool hasMatchaParfait = false;
    bool hasLuckyCoin = false;
    int  magicKeyUses = 0;         // 万能钥匙剩余使用次数
    int  tempShieldCharges = 0;    // 剩余战斗次数，每场+50防
    int  shopUseCount = 0;
    bool wallBreakerUsed = false;  // 破墙锤已激活
    bool stairUpUsed = false;      // 上楼器已激活
    bool stairDownUsed = false;    // 下楼器已激活
    bool hasCross = false;          // 十字架：对吸血鬼/兽人攻击翻倍
    bool hasDragonSlayer = false;   // 屠龙匕：对魔龙攻击翻倍
    bool hasHolyShield = false;     // 圣盾/神圣盾：免疫魔法攻击
    bool freezeMagicUsed = false;   // 冰冻魔法：下一次进入岩浆时冻结
    int flyWandUses = 0;            // 飞行魔杖使用次数
    int symmetryFlyerUses = 0;      // 对称飞行器剩余次数

    // 钥匙相关接口
    void AddKey(KeyType type, int count = 1);
    bool HasKey(KeyType type) const;
    bool UseKey(KeyType type);
    int KeyCount(KeyType type) const;

    // 通用物品背包接口
    void AddItem(std::unique_ptr<Item> item);
    const std::vector<std::unique_ptr<Item>>& Inventory() const;
    int  InventoryCount() const;

    // 使用道具：按索引使用背包中的道具，成功返回 true
    bool UseItem(int index);
    // 查看道具（不消耗）
    const Item* GetItem(int index) const;

private:
    std::unordered_map<KeyType, int> m_keys;
    std::vector<std::unique_ptr<Item>> m_items;
};
