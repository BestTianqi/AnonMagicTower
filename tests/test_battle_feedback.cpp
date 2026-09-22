#include <cassert>
#include <string>
#include <vector>
#include "Game/BossEncounter.h"
#include "UI/BattleFeedback.h"
#include "UI/BossBattlePresentation.h"
#include "UI/BossEncounterFlow.h"
#include "UI/MonsterVisual.h"
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

    StoryCompletionPolicy ordinary(false);
    assert(ordinary.canReject(false));
    StoryCompletionPolicy mandatory(true);
    assert(!mandatory.canReject(false));
    assert(mandatory.canReject(true));

    struct ExpectedBoss {
        int floor;
        const char* name;
        BossEncounterId id;
        const char* storyKey;
        const char* cg;
    };
    const std::vector<ExpectedBoss> bosses = {
        {10, "八幡海铃", BossEncounterId::Floor10Umiri, "boss_prebattle_f10_umiri", ":/images/runtime/cg/floor10_ambush.png"},
        {15, "宫永ののかSP", BossEncounterId::Floor15Octopus, "boss_prebattle_f15_octopus", ":/images/runtime/cg/boss15_octopus_prebattle.png"},
        {20, "凑友希那", BossEncounterId::Floor20Yukina, "boss_prebattle_f20_yukina", ":/images/runtime/cg/boss20_yukina_vampire_prebattle.png"},
        {25, "户山香澄", BossEncounterId::Floor25Kasumi, "boss_prebattle_f25_kasumi", ":/images/runtime/cg/boss25_kasumi_archmage_prebattle.png"},
        {32, "幼年长崎素世SP", BossEncounterId::Floor32Knight, "floor32_knight_dialogue", ":/images/runtime/cg/floor32_child_soyo_charge.png"},
        {35, "薇欧拉SP", BossEncounterId::Floor35Viola, "boss_prebattle_f35_viola_dragon", ":/images/runtime/cg/boss35_viola_dragon_prebattle.png"},
        {40, "幼年长崎素世SP", BossEncounterId::Floor40Knight, "boss_prebattle_f40_knight", ":/images/runtime/cg/boss40_knight_prebattle.png"},
        {49, "长崎素世SP", BossEncounterId::Floor49Phantom, "boss_prebattle_f49_phantom", ":/images/runtime/cg/boss49_soyo_phantom_prebattle.png"},
        {50, "长崎素世", BossEncounterId::Floor50Soyo, "boss_prebattle_f50_soyo", ":/images/runtime/cg/boss50_soyo_final_prebattle.png"}
    };
    for (const auto& expected : bosses) {
        assert(classifyBossEncounter(expected.floor, expected.name) == expected.id);
        const auto* descriptor = bossBattleDescriptor(expected.id);
        assert(descriptor);
        assert(descriptor->storyKey == expected.storyKey);
        assert(descriptor->cgPath == expected.cg);
    }
    assert(classifyBossEncounter(42, "幼年长崎素世SP") == BossEncounterId::None);
    assert(classifyBossEncounter(48, "藤都子SP") == BossEncounterId::None);
    assert(monsterSpriteResourcePath("要乐奈") ==
           ":/images/characters/monsters/monster_01.png");
    assert(monsterSpriteResourcePath("长崎素世") ==
           ":/images/characters/monsters/monster_34.png");
    assert(monsterSpriteResourcePath("不存在的怪物").empty());

    std::vector<std::string> events;
    BossEncounterFlowCallbacks normalFlow;
    normalFlow.present = [&]() { events.push_back("present"); return true; };
    normalFlow.markStory = [&]() { events.push_back("mark"); };
    normalFlow.fight = [&]() { events.push_back("fight"); return BossFlowFightResult::PlayerWin; };
    normalFlow.postFight = [&](BossFlowFightResult) { events.push_back("post"); };
    assert(runBossEncounterFlow({true, false, false}, normalFlow) == BossEncounterFlowResult::Completed);
    assert((events == std::vector<std::string>{"present", "mark", "fight", "post"}));

    events.clear();
    BossEncounterFlowCallbacks knightFlow = normalFlow;
    knightFlow.firstStrike = [&]() { events.push_back("first"); return true; };
    assert(runBossEncounterFlow({true, false, true}, knightFlow) == BossEncounterFlowResult::Completed);
    assert((events == std::vector<std::string>{"present", "mark", "first", "fight", "post"}));

    events.clear();
    knightFlow.firstStrike = [&]() { events.push_back("first"); return false; };
    knightFlow.gameOver = [&]() { events.push_back("gameover"); };
    assert(runBossEncounterFlow({true, false, true}, knightFlow) == BossEncounterFlowResult::KilledByFirstStrike);
    assert((events == std::vector<std::string>{"present", "mark", "first", "gameover"}));

    events.clear();
    assert(runBossEncounterFlow({false, false, false}, normalFlow) == BossEncounterFlowResult::Blocked);
    assert(events.empty());

    events.clear();
    assert(runBossEncounterFlow({true, true, false}, normalFlow) == BossEncounterFlowResult::Completed);
    assert((events == std::vector<std::string>{"fight", "post"}));

    // CG file missing时，表现层使用场景快照完成展示，随后仍然只战斗一次。
    events.clear();
    normalFlow.present = [&]() {
        assert(storyBackdropSource(false) == StoryBackdropSource::SceneSnapshot);
        events.push_back("fallback");
        return true;
    };
    assert(runBossEncounterFlow({true, false, false}, normalFlow) == BossEncounterFlowResult::Completed);
    assert((events == std::vector<std::string>{"fallback", "mark", "fight", "post"}));
    return 0;
}
