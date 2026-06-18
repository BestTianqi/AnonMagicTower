#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Items.h"

class Player; // forward

class NPC {
public:
    NPC(const std::string& name, const std::vector<std::string>& dialog, std::unique_ptr<Item> reward = nullptr);

    const std::string& GetName() const { return m_name; }
    const std::vector<std::string>& Dialog() const { return m_dialog; }

    // 与玩家交互：如果有 reward 且未发放，则发放并返回描述；否则返回对话内容
    std::string Interact(Player& player);

    bool HasGivenReward() const { return m_given; }
    void SetGiven(bool v) { m_given = v; }

private:
    std::string m_name;
    std::vector<std::string> m_dialog;
    std::unique_ptr<Item> m_reward;
    bool m_given;
};
