#pragma once

#include <string>
#include <vector>

inline std::string summarizeBattleLog(const std::vector<std::string>& log)
{
    if (log.empty()) return "战斗结束";
    std::string summary;
    for (size_t i = 0; i < log.size(); ++i) {
        if (i > 0) summary += '\n';
        summary += log[i];
    }
    return summary;
}
