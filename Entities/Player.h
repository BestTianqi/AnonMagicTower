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
    int gold;
    bool hasGlasses = false;
    bool hasPenguinDoll = false;
    bool hasMatchaParfait = false;


    // 钥匙相关接口
    void AddKey(KeyType type, int count = 1);
    bool HasKey(KeyType type) const;
    bool UseKey(KeyType type); // 如果有钥匙则消耗并返回 true
    int KeyCount(KeyType type) const;

    // 通用物品背包接口
    void AddItem(const Item& item);
    const std::vector<Item>& Inventory() const;
    int  InventoryCount() const;

    // 使用道具：按索引使用背包中的道具，成功返回 true
    bool UseItem(int index);
    // 查看道具（不消耗）
    const Item* GetItem(int index) const;

private:
    std::unordered_map<KeyType, int> m_keys;
    std::vector<Item> m_items;
};
