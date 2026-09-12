#include <cassert>
#include <string>
#include <vector>
#include "UI/BattleFeedback.h"

int main() {
    assert(summarizeBattleLog({}) == "战斗结束");
    const std::vector<std::string> log = {"你攻击了怪物。", "怪物被击败！", "获得 7 金币。"};
    assert(summarizeBattleLog(log) == "你攻击了怪物。\n怪物被击败！\n获得 7 金币。");
    return 0;
}
