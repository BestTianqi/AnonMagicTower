#pragma once

#include <iomanip>
#include <sstream>
#include <string>

#include "Entities/MonsterDB.h"

inline std::string monsterSpriteResourcePath(const std::string& name)
{
    const int index = MonsterDB::indexOf(name);
    if (index < 0) return {};
    std::ostringstream path;
    path << ":/images/characters/monsters/monster_"
         << std::setw(2) << std::setfill('0') << (index + 1) << ".png";
    return path.str();
}
