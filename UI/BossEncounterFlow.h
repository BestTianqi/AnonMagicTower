#pragma once

#include <functional>

enum class BossFlowFightResult {
    PlayerWin,
    PlayerDead,
    GameWin,
    Stalemate
};

enum class BossEncounterFlowResult {
    Blocked,
    PresentationIncomplete,
    KilledByFirstStrike,
    Completed
};

struct BossEncounterFlowInput {
    bool ready = false;
    bool storyAlreadyShown = false;
    bool hasFirstStrike = false;
};

struct BossEncounterFlowCallbacks {
    std::function<bool()> present;
    std::function<void()> markStory;
    std::function<bool()> firstStrike;
    std::function<BossFlowFightResult()> fight;
    std::function<void(BossFlowFightResult)> postFight;
    std::function<void()> gameOver;
};

inline BossEncounterFlowResult runBossEncounterFlow(
    const BossEncounterFlowInput& input,
    const BossEncounterFlowCallbacks& callbacks)
{
    if (!input.ready) return BossEncounterFlowResult::Blocked;
    if (!input.storyAlreadyShown) {
        if (!callbacks.present || !callbacks.present())
            return BossEncounterFlowResult::PresentationIncomplete;
        if (callbacks.markStory) callbacks.markStory();
    }
    if (input.hasFirstStrike && callbacks.firstStrike && !callbacks.firstStrike()) {
        if (callbacks.gameOver) callbacks.gameOver();
        return BossEncounterFlowResult::KilledByFirstStrike;
    }
    const BossFlowFightResult result = callbacks.fight
        ? callbacks.fight() : BossFlowFightResult::Stalemate;
    if (callbacks.postFight) callbacks.postFight(result);
    return BossEncounterFlowResult::Completed;
}
