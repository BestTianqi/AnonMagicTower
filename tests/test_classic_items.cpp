#include <cassert>
#include <memory>
#include "Entities/Items.h"
#include "Entities/NPC.h"
#include "Entities/Player.h"

int main() {
    Player player;
    player.hp = 100;
    player.atk = 10;
    player.def = 10;

    RubyGem ruby;
    ruby.Apply(player);
    assert(player.atk == 13);

    SapphireGem sapphire;
    sapphire.Apply(player);
    assert(player.def == 13);

    SmallPotion small;
    small.Apply(player);
    assert(player.hp == 300);

    LargePotion large;
    large.Apply(player);
    assert(player.hp == 800);

    NPC npc("商人", {"测试"}, nullptr, true, 25,
            std::make_unique<RubyGem>(), 15);
    assert(npc.ClassicId() == 15);
    return 0;
}
