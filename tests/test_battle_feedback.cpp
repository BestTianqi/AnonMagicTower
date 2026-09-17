#include <cassert>
#include <string>
#include <vector>
#include "UI/BattleFeedback.h"
#include "UI/StoryPresentation.h"

int main() {
    assert(summarizeBattleLog({}) == "战斗结束");
    const std::vector<std::string> log = {"你攻击了怪物。", "怪物被击败！", "获得 7 金币。"};
    assert(summarizeBattleLog(log) == "你攻击了怪物。\n怪物被击败！\n获得 7 金币。");

    // 每页剧情必须拥有全窗口背景：有专属CG时优先使用，否则使用当前场景快照。
    assert(storyBackdropSource(true) == StoryBackdropSource::ExplicitCg);
    assert(storyBackdropSource(false) == StoryBackdropSource::SceneSnapshot);
    StoryPager pager(3);
    assert(pager.page() == 0 && !pager.finished());
    assert(pager.advance() && pager.page() == 1);
    assert(pager.advance() && pager.page() == 2);
    assert(!pager.advance() && pager.finished());

    // 第一层开场只属于新游戏首次进入；读档、回到一层或已经播放过都不触发。
    assert(shouldShowFirstFloorOpening(true, 1, false));
    assert(!shouldShowFirstFloorOpening(false, 1, false));
    assert(!shouldShowFirstFloorOpening(true, 2, false));
    assert(!shouldShowFirstFloorOpening(true, 1, true));
    return 0;
}
