#include <cassert>
#include <memory>
#include "Entities/Items.h"
#include "Entities/NPC.h"
#include "Entities/Player.h"

int main() {
    assert(classicItemTierForFloor(1).rubyAttack == 1);
    assert(classicItemTierForFloor(10).largePotionHp == 200);
    assert(classicItemTierForFloor(11).rubyAttack == 2);
    assert(classicItemTierForFloor(15).smallPotionHp == 100);
    assert(classicItemTierForFloor(20).sapphireDefense == 2);
    assert(classicItemTierForFloor(21).rubyAttack == 3);
    assert(classicItemTierForFloor(50).largePotionHp == 1000);

    Player player;
    player.hp = 100;
    player.atk = 10;
    player.def = 10;

    RubyGem ruby;
    ruby.Apply(player);
    assert(player.atk == 11);

    SapphireGem sapphire;
    sapphire.Apply(player);
    assert(player.def == 11);

    SmallPotion small;
    small.Apply(player);
    assert(player.hp == 150);

    LargePotion large;
    large.Apply(player);
    assert(player.hp == 350);

    NPC npc("商人", {"测试"}, nullptr, true, 25,
            std::make_unique<RubyGem>(), 15);
    assert(npc.ClassicId() == 15);
    return 0;
}
