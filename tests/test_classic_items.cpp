#include <cassert>
#include <memory>
#include "Entities/Items.h"
#include "Entities/NPC.h"
#include "Entities/Player.h"
#include "Game/Game.h"

int main() {
    assert(classicItemTierForFloor(1).rubyAttack == 1);
    assert(classicItemTierForFloor(10).largePotionHp == 200);
    assert(classicItemTierForFloor(11).rubyAttack == 2);
    assert(classicItemTierForFloor(15).smallPotionHp == 100);
    assert(classicItemTierForFloor(20).sapphireDefense == 2);
    assert(classicItemTierForFloor(21).rubyAttack == 3);
    assert(classicItemTierForFloor(50).largePotionHp == 1000);
    const auto shop4 = classicShopOfferForFloor(4, 0);
    assert(shop4.hp == 100 && shop4.atk == 2 && shop4.def == 4 && shop4.price == 20);
    const auto shop12 = classicShopOfferForFloor(12, 2);
    assert(shop12.hp == 100 && shop12.atk == 4 && shop12.def == 8 && shop12.price == 40);
    const auto shop46 = classicShopOfferForFloor(46, 3);
    assert(shop46.atk == 10 && shop46.def == 20 && shop46.price == 80);

    // 道具名称校验：中英文别名归一到游戏内显示名，未知名称必须被拒绝。
    assert(Game::isKnownItemName("Red Key"));
    assert(Game::canonicalItemName("Red Key") == "红钥匙");
    assert(Game::canonicalItemName("Yellow Key") == "黄钥匙");
    assert(Game::canonicalItemName("Small Potion") == "小血瓶");
    assert(!Game::isKnownItemName("未知道具"));
    assert(Game::canonicalItemName("未知道具").empty());
    auto unknown = Game::createItemByName("Mystery Relic", 42);
    assert(unknown != nullptr);
    assert(unknown->GetName() == "未知道具");
    assert(unknown->GetValue() == 42);
    assert(dynamic_cast<UnknownItem*>(unknown.get())->SourceName() == "Mystery Relic");

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
