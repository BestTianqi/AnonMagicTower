#pragma once

#include <string_view>

#include "Game/BossEncounter.h"

struct BossBattleDescriptor {
    BossEncounterId id;
    std::string_view storyKey;
    std::string_view cgPath;
    std::string_view speaker;
    std::string_view dialogue;
    std::string_view portraitPath;
    std::string_view accent;
};

inline const BossBattleDescriptor* bossBattleDescriptor(BossEncounterId id)
{
    static constexpr BossBattleDescriptor descriptors[] = {
        {BossEncounterId::Floor10Umiri, "boss_prebattle_f10_umiri", ":/images/runtime/cg/floor10_ambush.png", "八幡海铃", "两侧的声音安静下来了。最后是我。别急着抢拍，听清楚再动。", ":/images/characters/portraits/umiri.png", "#8fd4ff"},
        {BossEncounterId::Floor15Octopus, "boss_prebattle_f15_octopus", ":/images/runtime/cg/boss15_octopus_prebattle.png", "宫永ののかSP", "大章鱼守着通路。想继续向上，就先突破这片舞台！", ":/images/characters/portraits/nonoka_stage.png", "#b78cff"},
        {BossEncounterId::Floor20Yukina, "boss_prebattle_f20_yukina", ":/images/runtime/cg/boss20_yukina_vampire_prebattle.png", "凑友希那", "准备好了？这次不要移开目光。把你自己的那一段，唱到最后。", ":/images/characters/portraits/yukina_stage.png", "#d7c6ff"},
        {BossEncounterId::Floor25Kasumi, "boss_prebattle_f25_kasumi", ":/images/runtime/cg/boss25_kasumi_archmage_prebattle.png", "户山香澄", "这里连我的声音也变成魔法了！爱音，别跟着回声跑，看准我站的位置。准备好，我们就开始！", ":/images/characters/portraits/kasumi_stage.png", "#ff78a6"},
        {BossEncounterId::Floor32Knight, "floor32_knight_dialogue", ":/images/runtime/cg/floor32_child_soyo_charge.png", "幼年长崎素世", "终于追上你了。骑士队长将先攻！", ":/images/characters/portraits/variants/soyo_child_angry.png", "#b58cff"},
        {BossEncounterId::Floor35Viola, "boss_prebattle_f35_viola_dragon", ":/images/runtime/cg/boss35_viola_dragon_prebattle.png", "薇欧拉SP", "火焰把这里封了太久。想穿过去，就打破我身上的魔龙幻象。别停在火光里。", ":/images/characters/portraits/viola_stage.png", "#cf9dff"},
        {BossEncounterId::Floor40Knight, "boss_prebattle_f40_knight", ":/images/runtime/cg/boss40_knight_prebattle.png", "幼年长崎素世", "这一次不会再让你过去。", ":/images/characters/portraits/variants/soyo_child_angry.png", "#b58cff"},
        {BossEncounterId::Floor49Phantom, "boss_prebattle_f49_phantom", ":/images/runtime/cg/boss49_soyo_phantom_prebattle.png", "长崎素世SP", "你一定要让这段演出结束吗？门一旦打开，我就再也不能替她留下你了。", ":/images/characters/portraits/variants/soyo_witch_more_battle.png", "#be83ff"},
        {BossEncounterId::Floor50Soyo, "boss_prebattle_f50_soyo", ":/images/runtime/cg/boss50_soyo_final_prebattle.png", "长崎素世", "头套已经摘下。爱音，让这一切在这里结束吧。", ":/images/characters/portraits/variants/soyo_witch_more_battle.png", "#be83ff"}
    };
    for (const auto& descriptor : descriptors) {
        if (descriptor.id == id) return &descriptor;
    }
    return nullptr;
}
