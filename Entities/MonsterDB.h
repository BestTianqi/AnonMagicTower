#pragma once

#include "Monster.h"
#include <vector>

namespace MonsterDB {

// 主题名称按原版 50 层魔塔的怪物 ID 一一对应；数值保持原版。
inline std::vector<Monster> all() {
    return {
        // 主题角色·原版怪物                 HP   ATK  DEF  GOLD
        { "小猫·绿色史莱姆",                 35,   18,   1,    1 },
        { "墨提斯·红色史莱姆",               45,   20,   2,    2 },
        { "客服·小蝙蝠",                     35,   38,   3,    3 },
        { "企鹅·初级法师",                   60,   32,   8,    5 },
        { "熊猫·骷髅人",                     50,   42,   6,    6 },
        { "大猫·骷髅士兵",                   55,   52,  12,    8 },
        { "狗狗·初级卫兵",                   50,   48,  22,   12 },
        { "妈妈·骷髅队长",                  100,   65,  15,   30 },
        { "恐惧姐·大史莱姆",                130,   60,   3,    8 },
        { "要乐奈·大蝙蝠",                   60,  100,   8,   12 },
        { "高松灯·高级法师",                100,   95,  30,   22 },
        { "椎名立希·兽人",                  260,   85,   5,   18 },
        { "八幡海铃·兽人武士",              320,  120,  15,   30 },
        { "祐天寺若麦·石头人",               20,  100,  68,   28 },
        { "若叶睦·巨型章鱼",               1200,  180,  20,  100 },
        { "三角初华·吸血鬼",                444,  199,  66,  144 },
        { "丰川祥子·大法师",               4500,  560, 310, 1000 },
        { "薇欧拉·鬼战士",                  220,  180,  30,   35 },
        { "仲町あられ·战士",                210,  200,  65,   45 },
        { "宫永ののか·幽灵",                320,  140,  20,   30 },
        { "峰月律·中级卫兵",                100,  180, 110,  100 },
        { "藤都子·双手剑士",                100,  680,  50,   55 },
        { "千石ユノ·魔龙",                 1500,  600, 250,  800 },
        { "要乐奈·骑士",                    160,  230, 105,   65 },
        { "椎名立希·骑士队长",              120,  150,  50,  100 },
        { "高松灯·初级巫师",                220,  370, 110,   80 },
        { "八幡海铃·高级巫师",              200,  380, 130,   90 },
        { "祐天寺若麦·史莱姆王",            360,  310,  20,   40 },
        { "若叶睦·吸血蝙蝠",                200,  390,  90,   50 },
        { "三角初华·黑骑士",                180,  430, 210,  120 },
        { "丰川祥子·魔法警卫",              230,  450, 100,  100 },
        { "薇欧拉·高级卫兵",                180,  460, 360,  200 },
        { "长崎素世·魔王幻影",             8000, 5000,1000,  500 },
        { "长崎素世·魔王本体",             5000, 1580, 190,  500 },
    };
}

inline Monster get(const std::string& name) {
    for (const auto& monster : all())
        if (monster.GetName() == name)
            return monster;
    return Monster();
}

inline Monster getByIndex(int index) {
    const auto monsters = all();
    if (index >= 0 && index < static_cast<int>(monsters.size()))
        return monsters[index];
    return Monster();
}

inline int indexOf(const std::string& name) {
    const auto monsters = all();
    for (size_t i = 0; i < monsters.size(); ++i)
        if (monsters[i].GetName() == name)
            return static_cast<int>(i);
    return -1;
}

} // namespace MonsterDB
