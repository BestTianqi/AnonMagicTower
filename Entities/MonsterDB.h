#pragma once

#include "Monster.h"
#include <vector>

namespace MonsterDB {

// 18种怪物预设
inline std::vector<Monster> all() {
    return {
        //  name             hp  atk def gold
        { "要乐奈",           25,  8,  1,   3  },
        { "若叶睦",           40, 12,  3,   6  },
        { "丰川祥子",         50, 15,  5,  10  },
        { "高松灯",           30, 18,  2,   8  },
        { "椎名立希",         45, 22,  5,  12  },
        { "祐天寺若麦",       60, 16, 10,  15  },
        { "三角初华",         75, 18, 10,  20  },
        { "长崎素世",         55, 24,  5,  16  },
        { "八幡海铃",         80, 16, 12,  18  },
        { "大要乐奈",        100, 22, 12,  25  },
        { "大高松灯",         70, 28,  8,  28  },
        { "大椎名立希",       60, 33,  6,  30  },
        { "大八幡海铃",       120, 18, 18,  28  },
        { "大祐天寺若麦",     110, 28, 16,  38  },
        { "墨提斯",           80, 38,  8,  40  },
        { "大三角初华",       140, 32, 16,  42  },
        { "大丰川祥子",       180, 40, 20,  60  },
        { "大长崎素世",       300, 50, 25,  120 },
    };
}

// 按名称获取
inline Monster get(const std::string& name) {
    for (const auto& m : all())
        if (m.GetName() == name)
            return m;
    return Monster();  // 未找到返回默认
}

// 按索引获取 (0-17)
inline Monster getByIndex(int index) {
    auto v = all();
    if (index >= 0 && index < (int)v.size())
        return v[index];
    return Monster();
}

} // namespace MonsterDB
