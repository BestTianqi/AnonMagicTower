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
    int shopUseCount = 0;
    bool wallBreakerUsed = false;  // 破墙锤已激活，下次移动撞墙时破墙
    bool stairUpUsed = false;      // 上楼器已激活
    bool stairDownUsed = false;    // 下楼器已激活

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
