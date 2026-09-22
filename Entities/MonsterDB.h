#pragma once

#include "Monster.h"
#include <array>
#include <string_view>
#include <vector>

namespace MonsterDB {

// 主题名称按原版 50 层魔塔的怪物 ID 一一对应；数值保持原版。
inline std::vector<Monster> all() {
    return {
        // 显示名只保留人物名与 SP；原版怪物类型由稳定索引表达。
        { "要乐奈",                 35,   18,   1,    1 },
        { "若叶睦",                 45,   20,   2,    2 },
        { "丰川祥子",               35,   38,   3,    3 },
        { "高松灯",                 60,   32,   8,    5 },
        { "椎名立希",               50,   42,   6,    6 },
        { "祐天寺若麦",             55,   52,  12,    8 },
        { "三角初华",               50,   48,  22,   12 },
        { "八幡海铃",              100,   65,  15,   30 },
        { "仲町あられ",            130,   60,   3,    8 },
        { "宫永ののか",             60,  100,   8,   12 },
        { "薇欧拉",                100,   95,  30,   22 },
        { "峰月律",                260,   85,   5,   18 },
        { "藤都子",                320,  120,  15,   30 },
        { "千石ユノ",               20,  100,  68,   28 },
        { "宫永ののかSP",         1200,  180,  20,  100 },
        { "凑友希那",              444,  199,  66,  144 },
        { "户山香澄",             4500,  560, 310, 1000 },
        { "要乐奈SP",              220,  180,  30,   35 },
        { "高松灯SP",              210,  200,  65,   45 },
        { "Soyorin",               320,  140,  20,   30 },
        { "椎名立希SP",            100,  180, 110,  100 },
        { "丰川祥子SP",            100,  680,  50,   55 },
        { "薇欧拉SP",             1500,  600, 250,  800 },
        { "若叶睦SP",              160,  230, 105,   65 },
        { "幼年长崎素世SP",        120,  150,  50,  100 },
        { "仲町あられSP",          220,  370, 110,   80 },
        { "峰月律SP",              200,  380, 130,   90 },
        { "祐天寺若麦SP",          360,  310,  20,   40 },
        { "千石ユノSP",            200,  390,  90,   50 },
        { "八幡海铃SP",            180,  430, 210,  120 },
        { "藤都子SP",              230,  450, 100,  100 },
        { "三角初华SP",            180,  460, 360,  200 },
        { "长崎素世SP",           8000, 5000,1000,  500 },
        { "长崎素世",             5000, 1580, 190,  500 },
    };
}

// 旧存档仍保存“人物名·原版怪物名”；只用于读档和历史调用兼容。
inline constexpr std::array<std::string_view, 34> legacyNames = {
    "要乐奈·绿色史莱姆", "若叶睦·红色史莱姆", "丰川祥子·小蝙蝠", "高松灯·初级法师",
    "椎名立希·骷髅人", "祐天寺若麦·骷髅士兵", "三角初华·初级卫兵", "八幡海铃·骷髅队长",
    "仲町あられ·大史莱姆", "宫永ののか·大蝙蝠", "薇欧拉·高级法师", "峰月律·兽人",
    "藤都子·兽人武士", "千石ユノ·石头人", "宫永ののかSP·巨型章鱼", "凑友希那·吸血鬼",
    "户山香澄·大法师", "要乐奈SP·鬼战士", "高松灯SP·战士", "Soyorin·幽灵",
    "椎名立希SP·中级卫兵", "丰川祥子SP·双手剑士", "薇欧拉SP·魔龙", "若叶睦SP·骑士",
    "幼年长崎素世·骑士队长", "仲町あられSP·初级巫师", "峰月律SP·高级巫师", "祐天寺若麦SP·史莱姆王",
    "千石ユノSP·吸血蝙蝠", "八幡海铃SP·黑骑士", "藤都子SP·魔法警卫", "三角初华SP·高级卫兵",
    "长崎素世·幻影", "长崎素世·本体"
};

inline int indexOf(const std::string& name) {
    const auto monsters = all();
    for (size_t i = 0; i < monsters.size(); ++i) {
        if (monsters[i].GetName() == name || legacyNames[i] == name)
            return static_cast<int>(i);
    }
    return -1;
}

inline Monster getByIndex(int index) {
    const auto monsters = all();
    if (index >= 0 && index < static_cast<int>(monsters.size()))
        return monsters[index];
    return Monster();
}

inline Monster get(const std::string& name) {
    return getByIndex(indexOf(name));
}

inline bool hasIndex(const std::string& name, int index) {
    return indexOf(name) == index;
}

inline bool isVampireOrOrc(const std::string& name) {
    const int index = indexOf(name);
    return index == 11 || index == 12 || index == 15 || index == 28 ||
           name.find("吸血") != std::string::npos || name.find("兽人") != std::string::npos;
}

inline bool isDragon(const std::string& name) {
    return indexOf(name) == 22 || name.find("魔龙") != std::string::npos;
}

inline int mageFieldDamage(const std::string& name) {
    const int index = indexOf(name);
    if (index == 26 || name.find("高级巫师") != std::string::npos) return 200;
    if (index == 25 || name.find("初级巫师") != std::string::npos) return 100;
    return 0;
}

inline bool isMagicGuard(const std::string& name) {
    return indexOf(name) == 30 || name.find("魔法警卫") != std::string::npos;
}

inline bool isTomori(const std::string& name) {
    const int index = indexOf(name);
    return index == 3 || index == 18 || name.find("高松灯") != std::string::npos;
}

inline bool isRana(const std::string& name) {
    const int index = indexOf(name);
    return index == 0 || index == 17 || name.find("要乐奈") != std::string::npos;
}

inline bool isBombImmune(const std::string& name) {
    const int index = indexOf(name);
    return index == 16 || index == 22 || index == 24 || index == 32 || index == 33 ||
           name.find("魔王") != std::string::npos || name.find("魔龙") != std::string::npos ||
           name.find("大法师") != std::string::npos;
}

} // namespace MonsterDB
