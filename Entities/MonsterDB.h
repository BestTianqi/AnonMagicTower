#pragma once

#include "Monster.h"
#include <vector>

namespace MonsterDB {

// 18种怪物预设
inline std::vector<Monster> all() {
    return {
        //  name             hp  atk def gold
        { "小猫",           25,  8,  1,   3  },
        { "墨提斯",           40, 12,  3,   6  },
        { "客服",         50, 32,  5,  10  },
        { "企鹅",           30, 18,  15,   8  },
        { "熊猫",         110, 60,  5,  12  },
        { "大猫",       260, 45, 15,  15  },
        { "狗狗",         175, 30, 50,  20  },
        { "妈妈",         260, 80,  20,  16  },
        { "恐惧姐",         380, 70, 40,  18  },
        { "要乐奈",        800, 110, 80,  50  },
        { "高松灯",         1080, 180,  120,  80  },
        { "椎名立希",       2200, 200,  160,  100  },
        { "八幡海铃",       2600, 240, 180,  120  },
        { "祐天寺若麦",     3200, 280, 160,  150 },
        { "若叶睦",         6000, 300,  200,  200  },
        { "三角初华",       10080, 420, 320,  300  },
        { "丰川祥子",       32000, 700, 350,  1000  },
        { "长崎素世",       69696, 969, 400,  2000 },
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
