#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Items.h"

class Player; // forward

class NPC {
public:
    NPC(const std::string& name, const std::vector<std::string>& dialog,
        std::unique_ptr<Item> reward = nullptr,
        bool isTrader = false, int tradeGoldCost = 0,
        std::unique_ptr<Item> tradeReward = nullptr);

    const std::string& GetName() const { return m_name; }
    const std::vector<std::string>& Dialog() const { return m_dialog; }

    // 与玩家交互
    std::string Interact(Player& player);

    // 奖励
    bool HasGivenReward() const { return m_given; }
    void SetGiven(bool v) { m_given = v; }
    const Item* GetReward() const { return m_reward.get(); }

    // 交易
    bool IsTrader() const { return m_isTrader; }
    void SetTrader(bool v) { m_isTrader = v; }
    int  GetTradeGoldCost() const { return m_tradeGoldCost; }
    void SetTradeGoldCost(int v) { m_tradeGoldCost = v; }
    const Item* GetTradeReward() const { return m_tradeReward.get(); }
    bool IsTradeDone() const { return m_tradeDone; }
    void SetTradeDone(bool v) { m_tradeDone = v; }

private:
    std::string m_name;
    std::vector<std::string> m_dialog;
    std::unique_ptr<Item> m_reward;
    bool m_given;

    bool m_isTrader;
    int  m_tradeGoldCost;
    std::unique_ptr<Item> m_tradeReward;
    bool m_tradeDone;
};
