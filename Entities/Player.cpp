#include "Player.h"

Player::Player()
    : x(0), y(0), hp(100), atk(10), def(10), gold(0)
{
}

void Player::AddKey(KeyType type, int count)
{
    m_keys[type] += count;
}

bool Player::HasKey(KeyType type) const
{
    auto it = m_keys.find(type);
    return it != m_keys.end() && it->second > 0;
}

bool Player::UseKey(KeyType type)
{
    auto it = m_keys.find(type);
    if (it == m_keys.end() || it->second <= 0) return false;
    --(it->second);
    return true;
}

int Player::KeyCount(KeyType type) const
{
    auto it = m_keys.find(type);
    if (it == m_keys.end()) return 0;
    return it->second;
}

void Player::AddItem(std::unique_ptr<Item> item)
{
    m_items.push_back(std::move(item));
}

const std::vector<std::unique_ptr<Item>>& Player::Inventory() const
{
    return m_items;
}

int Player::InventoryCount() const
{
    return (int)m_items.size();
}

bool Player::UseItem(int index)
{
    if (index < 0 || index >= (int)m_items.size())
        return false;
    if (m_items[index]->IsPassiveEffect())
        return true;  // 被动效果不消耗，仅查看
    if (auto* flyer = dynamic_cast<SymmetryFlyer*>(m_items[index].get())) {
        if (symmetryFlyerUses <= 0)
            flyer->Apply(*this); // 首次使用初始化原版的3次次数
        --symmetryFlyerUses;
        if (symmetryFlyerUses <= 0)
            m_items.erase(m_items.begin() + index);
        return true;
    }
    m_items[index]->Apply(*this);
    m_items.erase(m_items.begin() + index);
    return true;
}

const Item* Player::GetItem(int index) const
{
    if (index < 0 || index >= (int)m_items.size())
        return nullptr;
    return m_items[index].get();
}
