#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "Items.h"

class Player {
public:
    Player();

    int x;
    int y;
    int hp;
    int atk;
    int def;
    int gold; // 新增金币属性

    // 钥匙相关接口
    void AddKey(KeyType type, int count = 1);
    bool HasKey(KeyType type) const;
    bool UseKey(KeyType type); // 如果有钥匙则消耗并返回 true
    int KeyCount(KeyType type) const;

    // 通用物品背包接口
    void AddItem(const Item& item);
    const std::vector<Item>& Inventory() const;

private:
    std::unordered_map<KeyType, int> m_keys;
    std::vector<Item> m_items;
};
