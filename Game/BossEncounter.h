#pragma once

#include "../Entities/MonsterDB.h"
#include <string>
#include <string_view>

enum class BossEncounterId {
    None,
    Floor10Umiri,
    Floor15Octopus,
    Floor20Yukina,
    Floor25Kasumi,
    Floor32Knight,
    Floor35Viola,
    Floor40Knight,
    Floor49Phantom,
    Floor50Soyo
};

inline BossEncounterId classifyBossEncounter(int floor, std::string_view name)
{
    const int monsterIndex = MonsterDB::indexOf(std::string(name));
    if (floor == 10 && monsterIndex == 7) return BossEncounterId::Floor10Umiri;
    if (floor == 15 && monsterIndex == 14) return BossEncounterId::Floor15Octopus;
    if (floor == 20 && monsterIndex == 15) return BossEncounterId::Floor20Yukina;
    if (floor == 25 && monsterIndex == 16) return BossEncounterId::Floor25Kasumi;
    if (floor == 32 && monsterIndex == 24) return BossEncounterId::Floor32Knight;
    if (floor == 35 && monsterIndex == 22) return BossEncounterId::Floor35Viola;
    if (floor == 40 && monsterIndex == 24) return BossEncounterId::Floor40Knight;
    if (floor == 49 && monsterIndex == 32) return BossEncounterId::Floor49Phantom;
    if (floor == 50 && monsterIndex == 33) return BossEncounterId::Floor50Soyo;
    return BossEncounterId::None;
}
