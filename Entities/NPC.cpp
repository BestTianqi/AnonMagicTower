#include "NPC.h"
#include "Player.h"

NPC::NPC(const std::string& name, const std::vector<std::string>& dialog,
         std::unique_ptr<Item> reward,
         bool isTrader, int tradeGoldCost,
         std::unique_ptr<Item> tradeReward)
    : m_name(name), m_dialog(dialog), m_reward(std::move(reward)), m_given(false),
      m_isTrader(isTrader), m_tradeGoldCost(tradeGoldCost),
      m_tradeReward(std::move(tradeReward)), m_tradeDone(false)
{
}

std::string NPC::Interact(Player& player)
{
    if (m_reward && !m_given) {
        m_reward->Apply(player);
        m_given = true;
        return m_name + ": 谢谢你，接受我的礼物。";
    }
    // 返回第一条对话作为示例
    if (!m_dialog.empty()) return m_name + ": " + m_dialog[0];
    return m_name + ": ...";
}
