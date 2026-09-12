#include <cassert>
#include <memory>
#include <string>
#include <cstdio>
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
    assert(Game::canonicalItemName("Red Key") == "红色Live票");
    assert(Game::canonicalItemName("Yellow Key") == "黄色Live票");
    assert(Game::canonicalItemName("Small Potion") == "灯的热牛奶");
    assert(Game::canonicalItemName("Iron Sword") == "爱音拨片");
    assert(Game::canonicalItemName("Divine Shield") == "Mujica终幕面具");
    assert(Game::canonicalItemName("Cross") == "MyGO和解徽章");
    assert(Game::canonicalItemName("Dragon Slayer") == "祥子指挥棒");
    assert(Game::canonicalItemName("Freeze Magic") == "海铃冷静指令");
    assert(Game::canonicalItemName("Flying Wand") == "爱音手机");
    assert(Game::canonicalItemName("Earthquake Scroll") == "Mujica舞台震响卷");
    assert(Game::isKnownItemName("Mujica镜面舞台票"));
    assert(Game::isKnownItemName("小血瓶")); // 旧存档别名继续可读
    assert(!Game::isKnownItemName("未知道具"));
    assert(Game::canonicalItemName("未知道具").empty());
    auto unknown = Game::createItemByName("Mystery Relic", 42);
    assert(unknown != nullptr);
    assert(unknown->GetName() == "未知道具");
    assert(unknown->GetValue() == 42);
    assert(dynamic_cast<UnknownItem*>(unknown.get())->SourceName() == "Mystery Relic");

    auto ironSword = Game::createItemByName("Iron Sword", 10);
    assert(ironSword && ironSword->GetName() == "爱音拨片");
    assert(dynamic_cast<Weapon*>(ironSword.get())->AtkBonus() == 10);
    auto divineShield = Game::createItemByName("神圣盾", 100);
    assert(divineShield && divineShield->GetName() == "Mujica终幕面具");
    assert(dynamic_cast<Armor*>(divineShield.get())->DefBonus() == 100);

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
    assert(small.GetName() == "灯的热牛奶");
    small.Apply(player);
    assert(player.hp == 150);

    LargePotion large;
    assert(large.GetName() == "爱音能量饮");
    large.Apply(player);
    assert(player.hp == 350);

    Key redKey(KeyType::Red);
    assert(redKey.GetName() == "红色Live票");
    MagicKey backstagePass;
    assert(backstagePass.GetName() == "后台万能通行证");
    HolyWater kettle;
    assert(kettle.GetName() == "立希水壶");

    Cross cross;
    cross.Apply(player);
    assert(player.hasCross && cross.GetName() == "MyGO和解徽章");
    DragonSlayer dragonSlayer;
    dragonSlayer.Apply(player);
    assert(player.hasDragonSlayer);
    FreezeMagic freeze;
    freeze.Apply(player);
    assert(player.freezeMagicUsed);
    FlyingWand flying;
    flying.Apply(player);
    assert(player.flyWandUses == 1);
    SymmetryFlyer symmetry;
    symmetry.Apply(player);
    assert(player.symmetryFlyerUses == 3);

    // 新增原版状态应能跨存档保留。
    Game saved;
    saved.player().hasCross = true;
    saved.player().hasDragonSlayer = true;
    saved.player().hasHolyShield = true;
    saved.player().freezeMagicUsed = true;
    saved.player().flyWandUses = 2;
    saved.player().symmetryFlyerUses = 1;
    const std::string savePath = "classic_items_roundtrip.sav";
    assert(saved.saveToFile(savePath));
    Game restored;
    assert(restored.loadFromFile(savePath));
    assert(restored.player().hasCross);
    assert(restored.player().hasDragonSlayer);
    assert(restored.player().hasHolyShield);
    assert(restored.player().freezeMagicUsed);
    assert(restored.player().flyWandUses == 2);
    assert(restored.player().symmetryFlyerUses == 1);
    std::remove(savePath.c_str());

    Game combat;
    combat.player().hp = 100;
    combat.player().atk = 10;
    combat.player().def = 0;
    combat.player().hasCross = true;
    combat.spawnMonster(3, 3, Monster("吸血鬼", 15, 0, 10, 0));
    std::vector<std::string> log;
    assert(combat.fightAt(3, 3, log) == Game::Fight_PlayerWin);

    Game dragonFight;
    dragonFight.player().hp = 100;
    dragonFight.player().atk = 10;
    dragonFight.player().def = 0;
    dragonFight.player().hasDragonSlayer = true;
    dragonFight.spawnMonster(3, 3, Monster("魔龙", 15, 0, 10, 0));
    log.clear();
    assert(dragonFight.fightAt(3, 3, log) == Game::Fight_PlayerWin);

    Game magicFight;
    magicFight.player().hp = 100;
    magicFight.player().atk = 100;
    magicFight.player().def = 0;
    magicFight.player().hasHolyShield = true;
    magicFight.spawnMonster(3, 3, Monster("高级法师", 150, 100, 0, 0));
    log.clear();
    assert(magicFight.fightAt(3, 3, log) == Game::Fight_PlayerWin);
    assert(magicFight.player().hp == 100);

    Game lava;
    lava.setTile(3, 3, Tile_Lava);
    lava.player().freezeMagicUsed = true;
    assert(lava.tryMovePlayer(3, 3) == Game::Move_Ok);
    assert(lava.tileAt(3, 3) == Tile_Floor);
    assert(!lava.player().freezeMagicUsed);

    Game bombGame;
    bombGame.player().x = 5;
    bombGame.player().y = 5;
    bombGame.spawnMonster(6, 5, Monster("普通怪物", 10, 1, 0, 7));
    bombGame.spawnMonster(4, 5, Monster("魔龙", 10, 1, 0, 99));
    assert(bombGame.useBomb() == 1);
    assert(!bombGame.hasMonsterAt(6, 5));
    assert(bombGame.hasMonsterAt(4, 5));
    assert(bombGame.player().gold == 7);

    Game quakeGame;
    quakeGame.setTile(4, 4, Tile_Wall);
    quakeGame.setTile(5, 5, Tile_DarkWall);
    assert(quakeGame.useEarthquakeScroll() == 2);
    assert(quakeGame.tileAt(4, 4) == Tile_Floor);
    assert(quakeGame.tileAt(5, 5) == Tile_Floor);

    Game freezeGame;
    freezeGame.setTile(4, 4, Tile_Lava);
    freezeGame.setTile(6, 6, Tile_Lava);
    freezeGame.player().freezeMagicUsed = true;
    assert(freezeGame.useFreezeMagic() == 2);
    assert(freezeGame.tileAt(4, 4) == Tile_Floor);
    assert(freezeGame.tileAt(6, 6) == Tile_Floor);
    assert(!freezeGame.player().freezeMagicUsed);

    NPC npc("商人", {"测试"}, nullptr, true, 25,
            std::make_unique<RubyGem>(), 15);
    assert(npc.ClassicId() == 15);
    return 0;
}
