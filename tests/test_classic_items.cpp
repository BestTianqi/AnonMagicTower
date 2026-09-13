#include <cassert>
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <cstdio>
#include "Entities/Items.h"
#include "Entities/NPC.h"
#include "Entities/Player.h"
#include "Entities/MonsterDB.h"
#include "Game/Game.h"
#include "UI/MapEditor.h"

int main() {
    const auto roster = MonsterDB::all();
    assert(MapEditor::kMaxEditableFloor == 50);
    assert(roster.size() == 34);
    assert(roster[0].GetName() == "要乐奈·绿色史莱姆");
    assert(roster[14].GetName() == "宫永ののかSP·巨型章鱼");
    assert(roster[15].GetName() == "凑友希那·吸血鬼");
    assert(roster[16].GetName() == "户山香澄·大法师");
    assert(roster[30].GetName() == "藤都子SP·魔法警卫");
    assert(roster[33].GetName() == "长崎素世·本体");
    assert(roster[15].GetHP() == 444 && roster[15].GetATK() == 199 &&
           roster[15].GetDEF() == 66 && roster[15].GetGold() == 144);

    assert(classicItemTierForFloor(1).rubyAttack == 1);
    assert(classicItemTierForFloor(10).largePotionHp == 200);
    assert(classicItemTierForFloor(11).rubyAttack == 2);
    assert(classicItemTierForFloor(15).smallPotionHp == 100);
    assert(classicItemTierForFloor(20).sapphireDefense == 2);
    assert(classicItemTierForFloor(21).rubyAttack == 3);
    assert(classicItemTierForFloor(50).largePotionHp == 1000);
    const auto shop4 = classicShopOfferForFloor(4, 0);
    assert(shop4.hp == 100 && shop4.atk == 2 && shop4.def == 4 && shop4.price == 20);
    const auto shop4Next = classicShopOfferForFloor(4, 1);
    assert(shop4Next.hp == 200 && shop4Next.atk == 2 && shop4Next.def == 4 && shop4Next.price == 40);
    const auto shop12 = classicShopOfferForFloor(12, 2);
    assert(shop12.hp == 300 && shop12.atk == 4 && shop12.def == 8 && shop12.price == 80);
    const auto shop46 = classicShopOfferForFloor(46, 3);
    assert(shop46.hp == 400 && shop46.atk == 10 && shop46.def == 20 && shop46.price == 140);
    assert(classicShopOfferForFloor(46, 4).price == 220);

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
    assert(Game::canonicalItemName("Floor Teleporter") == "楼层传送器");
    assert(Game::isKnownItemName("楼层传送器"));
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
    auto floorTeleporter = Game::createItemByName("楼层传送器", 0);
    assert(floorTeleporter && floorTeleporter->GetName() == "楼层传送器");
    assert(floorTeleporter->IsUseItem());
    assert(floorTeleporter->IsReusable());
    floorTeleporter->Apply(player);
    assert(player.flyWandUses == 2);
    player.AddItem(std::move(floorTeleporter));
    const int teleporterCount = player.InventoryCount();
    assert(player.UseItem(teleporterCount - 1));
    assert(player.InventoryCount() == teleporterCount);
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

    // 暗墙是不可直接穿过的墙体；只有破墙道具才能打开。
    Game hiddenWall;
    hiddenWall.player().x = 3;
    hiddenWall.player().y = 3;
    hiddenWall.setTile(4, 3, Tile_DarkWall);
    assert(hiddenWall.tryMovePlayer(4, 3) == Game::Move_Block);
    assert(hiddenWall.player().x == 3 && hiddenWall.player().y == 3);
    assert(hiddenWall.tileAt(4, 3) == Tile_Floor);
    assert(hiddenWall.tryMovePlayer(4, 3) == Game::Move_Ok);
    assert(hiddenWall.player().x == 4 && hiddenWall.player().y == 3);

    // 鼠标瞬移只能到当前格子连通的可行走区域，不能跨墙。
    Game reachable;
    reachable.player().x = 3;
    reachable.player().y = 3;
    for (int y = 2; y <= 12; ++y) reachable.setTile(4, y, Tile_Wall);
    assert(!reachable.isTeleportReachable(5, 3));
    reachable.setTile(4, 3, Tile_Floor);
    assert(reachable.isTeleportReachable(5, 3));
    reachable.setTile(5, 3, Tile_Wall);
    assert(!reachable.isTeleportReachable(5, 3));

    // 鼠标瞬移复用移动交互：到达道具格时立即拾取，而不是只改坐标。
    Game teleportItem;
    teleportItem.player().x = 3;
    teleportItem.player().y = 3;
    teleportItem.addItemAt(4, 3, std::make_unique<SmallPotion>(200));
    teleportItem.setTile(4, 3, Tile_Item);
    const int hpBeforeTeleport = teleportItem.player().hp;
    assert(teleportItem.teleportPlayerTo(4, 3) == Game::Move_Pickup);
    assert(teleportItem.player().x == 4 && teleportItem.player().y == 3);
    assert(teleportItem.player().hp == hpBeforeTeleport + 200);
    assert(teleportItem.itemAt(4, 3) == nullptr);

    Game teleportNpc;
    teleportNpc.player().x = 3;
    teleportNpc.player().y = 3;
    teleportNpc.addNPCAt(4, 3, NPC("凛凛子", {"欢迎来到商店。"}));
    teleportNpc.setTile(4, 3, Tile_NPC);
    assert(teleportNpc.teleportPlayerTo(4, 3) == Game::Move_NPC);
    assert(teleportNpc.player().x == 4 && teleportNpc.player().y == 3);
    teleportNpc.setTile(5, 3, Tile_Floor);
    assert(teleportNpc.teleportPlayerTo(5, 3) == Game::Move_Ok);
    assert(teleportNpc.player().x == 5 && teleportNpc.player().y == 3);

    Game teleportMonster;
    teleportMonster.player().x = 3;
    teleportMonster.player().y = 3;
    teleportMonster.spawnMonster(4, 3, Monster("练习怪", 1, 0, 0, 0));
    teleportMonster.setTile(4, 3, Tile_Monster);
    assert(teleportMonster.teleportPlayerTo(4, 3) == Game::Move_Encounter);
    assert(teleportMonster.player().x == 4 && teleportMonster.player().y == 3);

    // 鼠标瞬移到连通的有钥匙红门时，应开门、消耗钥匙并进入门格。
    Game teleportDoor;
    teleportDoor.player().x = 3;
    teleportDoor.player().y = 3;
    teleportDoor.player().AddKey(KeyType::Red);
    teleportDoor.setTile(4, 3, Tile_DoorRed);
    assert(teleportDoor.teleportPlayerTo(4, 3) == Game::Move_Ok);
    assert(teleportDoor.player().x == 4 && teleportDoor.player().y == 3);
    assert(teleportDoor.tileAt(4, 3) == Tile_Floor);
    assert(teleportDoor.player().KeyCount(KeyType::Red) == 0);

    Game bombGame;
    bombGame.player().x = 5;
    bombGame.player().y = 5;
    bombGame.spawnMonster(6, 5, Monster("普通怪物", 10, 1, 0, 7));
    bombGame.spawnMonster(4, 5, Monster("魔龙", 10, 1, 0, 99));
    assert(bombGame.useBomb() == 1);
    assert(!bombGame.hasMonsterAt(6, 5));
    assert(bombGame.hasMonsterAt(4, 5));
    assert(bombGame.player().gold == 7);

    // 原版机关门：2 层六扇铁门在两名中级卫兵都被击败后自动打开。
    Game mechanismDoor;
    mechanismDoor.initFloor(2);
    mechanismDoor.player().x = 3;
    mechanismDoor.player().y = 3;
    mechanismDoor.player().atk = 100;
    mechanismDoor.setTile(4, 3, Tile_DoorIron);
    const Monster intermediateGuard("椎名立希SP·中级卫兵", 1, 0, 0, 0);
    mechanismDoor.spawnMonster(5, 3, intermediateGuard);
    mechanismDoor.spawnMonster(5, 4, intermediateGuard);
    assert(mechanismDoor.tryMovePlayer(4, 3) == Game::Move_DoorLocked);
    log.clear();
    assert(mechanismDoor.fightAt(5, 3, log) == Game::Fight_PlayerWin);
    assert(mechanismDoor.tileAt(4, 3) == Tile_DoorIron);
    log.clear();
    assert(mechanismDoor.fightAt(5, 4, log) == Game::Fight_PlayerWin);
    assert(mechanismDoor.tileAt(4, 3) == Tile_Floor);

    // 第八层花门只需要击败门旁两只三角初华；远处同类怪物不阻挡开门。
    Game floor8FlowerDoor;
    floor8FlowerDoor.initFloor(8);
    floor8FlowerDoor.player().x = 3;
    floor8FlowerDoor.player().y = 4;
    floor8FlowerDoor.player().atk = 100000;
    floor8FlowerDoor.player().hp = 1000000000;
    floor8FlowerDoor.setTile(4, 4, Tile_DoorMagic);
    floor8FlowerDoor.spawnMonster(5, 4, MonsterDB::getByIndex(6));
    floor8FlowerDoor.spawnMonster(5, 5, MonsterDB::getByIndex(6));
    floor8FlowerDoor.spawnMonster(8, 4, MonsterDB::getByIndex(6));
    assert(floor8FlowerDoor.tryMovePlayer(4, 4) == Game::Move_DoorLocked);
    std::vector<std::string> floor8Log;
    assert(floor8FlowerDoor.fightAt(5, 4, floor8Log) == Game::Fight_PlayerWin);
    assert(floor8FlowerDoor.tileAt(4, 4) == Tile_DoorMagic);
    floor8Log.clear();
    assert(floor8FlowerDoor.fightAt(8, 4, floor8Log) == Game::Fight_PlayerWin);
    assert(floor8FlowerDoor.tileAt(4, 4) == Tile_DoorMagic);
    floor8Log.clear();
    assert(floor8FlowerDoor.fightAt(5, 5, floor8Log) == Game::Fight_PlayerWin);
    assert(floor8FlowerDoor.tileAt(4, 4) == Tile_Floor);

    // 49层上下花门分别只检测各自正下方横向三格，互不共享守卫。
    Game floor49FlowerDoors;
    floor49FlowerDoors.initFloor(49);
    floor49FlowerDoors.player().x = 7;
    floor49FlowerDoors.player().y = 7;
    floor49FlowerDoors.player().atk = 100000;
    floor49FlowerDoors.player().hp = 1000000000;
    floor49FlowerDoors.setTile(7, 8, Tile_DoorMagic);
    floor49FlowerDoors.setTile(7, 10, Tile_DoorMagic);
    const Monster floor49Guard("49层守卫", 1, 0, 0, 0);
    floor49FlowerDoors.spawnMonster(6, 9, floor49Guard);
    floor49FlowerDoors.spawnMonster(8, 9, floor49Guard);
    floor49FlowerDoors.spawnMonster(6, 11, floor49Guard);
    floor49FlowerDoors.spawnMonster(8, 11, floor49Guard);
    floor49FlowerDoors.spawnMonster(3, 3, floor49Guard);
    assert(floor49FlowerDoors.tryMovePlayer(7, 8) == Game::Move_DoorLocked);
    std::vector<std::string> floor49Log;
    assert(floor49FlowerDoors.fightAt(6, 9, floor49Log) == Game::Fight_PlayerWin);
    assert(floor49FlowerDoors.tileAt(7, 8) == Tile_DoorMagic);
    floor49Log.clear();
    assert(floor49FlowerDoors.fightAt(8, 9, floor49Log) == Game::Fight_PlayerWin);
    assert(floor49FlowerDoors.tileAt(7, 8) == Tile_Floor);
    assert(floor49FlowerDoors.tileAt(7, 10) == Tile_DoorMagic);
    floor49Log.clear();
    assert(floor49FlowerDoors.fightAt(6, 11, floor49Log) == Game::Fight_PlayerWin);
    assert(floor49FlowerDoors.tileAt(7, 10) == Tile_DoorMagic);
    floor49Log.clear();
    assert(floor49FlowerDoors.fightAt(8, 11, floor49Log) == Game::Fight_PlayerWin);
    assert(floor49FlowerDoors.tileAt(7, 10) == Tile_Floor);
    assert(floor49FlowerDoors.hasMonsterAt(3, 3));

    // 48 层原版花门是坏门，只能用镐破坏，不会因清怪自动开启。
    Game brokenFlowerDoor;
    brokenFlowerDoor.initFloor(48);
    brokenFlowerDoor.player().x = 3;
    brokenFlowerDoor.player().y = 3;
    brokenFlowerDoor.player().wallBreakerUsed = true;
    brokenFlowerDoor.setTile(4, 3, Tile_DoorMagic);
    assert(brokenFlowerDoor.tryMovePlayer(4, 3) == Game::Move_Ok);
    assert(brokenFlowerDoor.tileAt(4, 3) == Tile_Floor);

    // 10 层完整 Boss 事件：到达八幡海铃前一格后，海铃退到最上方，
    // 左右花门取消，侧翼怪保持原位，上下门锁定；清完侧翼怪后上下门开启，
    // 击败海铃会出现奖励提示与向上楼梯。
    Game floor10Ambush;
    for (int i = 0; i < 9; ++i) floor10Ambush.goUpFloor(7, 12, false);
    floor10Ambush.initFloor(10);
    floor10Ambush.player().x = 7;
    floor10Ambush.player().y = 7;
    floor10Ambush.setTile(7, 6, Tile_Floor);
    floor10Ambush.setTile(5, 5, Tile_DoorMagic);
    floor10Ambush.setTile(9, 5, Tile_DoorMagic);
    floor10Ambush.setTile(7, 5, Tile_Monster);
    floor10Ambush.spawnMonster(7, 5, MonsterDB::getByIndex(7));
    const Monster skeletonSoldier = MonsterDB::getByIndex(5);
    for (int i = 0; i < 4; ++i)
        floor10Ambush.spawnMonster(2 + i, 4, skeletonSoldier);
    for (int i = 0; i < 4; ++i)
        floor10Ambush.spawnMonster(8 + i, 5, skeletonSoldier);
    floor10Ambush.player().atk = 1000;
    floor10Ambush.player().hp = 10000;
    assert(floor10Ambush.tryMovePlayer(7, 6) == Game::Move_Ok);
    assert(floor10Ambush.tileAt(5, 5) == Tile_Floor);
    assert(floor10Ambush.tileAt(9, 5) == Tile_Floor);
    assert(floor10Ambush.tileAt(7, 5) == Tile_DoorMagic);
    assert(floor10Ambush.tileAt(7, 7) == Tile_DoorMagic);
    assert(floor10Ambush.takeFloor10AmbushMovementAnimations().empty());
    assert(floor10Ambush.hasMonsterAt(2, 4));
    assert(floor10Ambush.monsterAt(7, 2) != nullptr);
    assert(floor10Ambush.monsterAt(7, 2)->GetName() == "八幡海铃·骷髅队长");
    int surrounded = 0;
    for (int y = 4; y <= 5; ++y)
        for (int x = 2; x <= 12; ++x)
            if (floor10Ambush.hasMonsterAt(x, y) &&
                (floor10Ambush.monsterAt(x, y)->GetName() == "椎名立希·骷髅人" ||
                 floor10Ambush.monsterAt(x, y)->GetName() == "祐天寺若麦·骷髅士兵")) ++surrounded;
    assert(surrounded == 8);
    while (true) {
        int guardX = -1, guardY = -1;
        for (int y = 4; y <= 5 && guardX < 0; ++y) {
            for (int x = 2; x <= 12; ++x) {
                auto* guard = floor10Ambush.monsterAt(x, y);
                if (guard && (guard->GetName() == "椎名立希·骷髅人" ||
                              guard->GetName() == "祐天寺若麦·骷髅士兵")) {
                    guardX = x; guardY = y; break;
                }
            }
        }
        if (guardX < 0) break;
        log.clear();
        assert(floor10Ambush.fightAt(guardX, guardY, log) == Game::Fight_PlayerWin);
    }
    assert(floor10Ambush.tileAt(5, 5) == Tile_Floor);
    assert(floor10Ambush.tileAt(9, 5) == Tile_Floor);
    assert(floor10Ambush.tileAt(7, 5) == Tile_Floor);
    assert(floor10Ambush.tileAt(7, 7) == Tile_Floor);
    log.clear();
    assert(floor10Ambush.fightAt(7, 2, log) == Game::Fight_PlayerWin);
    assert(floor10Ambush.tileAt(7, 12) == Tile_StairsUp);
    assert(floor10Ambush.tileAt(7, 2) == Tile_Item);
    assert(floor10Ambush.itemAt(7, 2) != nullptr);
    bool floor10RewardShown = false;
    for (const auto& line : log)
        if (line.find("奖励") != std::string::npos || line.find("楼梯") != std::string::npos)
            floor10RewardShown = true;
    assert(floor10RewardShown);

    // 20层 Boss 击败后生成蓝宝石奖励与向上楼梯。
    Game floor20Boss;
    for (int i = 0; i < 19; ++i) floor20Boss.goUpFloor(7, 12, false);
    floor20Boss.initFloor(20);
    floor20Boss.player().x = 7;
    floor20Boss.player().y = 7;
    floor20Boss.player().atk = 100000;
    floor20Boss.player().hp = 1000000000;
    floor20Boss.setTile(7, 6, Tile_Monster);
    floor20Boss.spawnMonster(7, 6, MonsterDB::getByIndex(15));
    log.clear();
    assert(floor20Boss.fightAt(7, 6, log) == Game::Fight_PlayerWin);
    assert(floor20Boss.tileAt(7, 6) == Tile_Item);
    assert(floor20Boss.itemAt(7, 6) != nullptr);
    assert(floor20Boss.tileAt(7, 12) == Tile_StairsUp);

    // 40层 Boss 击败后生成圣水奖励与向上楼梯。
    Game floor40Boss;
    for (int i = 0; i < 39; ++i) floor40Boss.goUpFloor(7, 12, false);
    floor40Boss.initFloor(40);
    floor40Boss.player().x = 7;
    floor40Boss.player().y = 7;
    floor40Boss.player().atk = 100000;
    floor40Boss.player().hp = 1000000000;
    floor40Boss.setTile(7, 6, Tile_Monster);
    floor40Boss.spawnMonster(7, 6, MonsterDB::getByIndex(24));
    log.clear();
    assert(floor40Boss.fightAt(7, 6, log) == Game::Fight_PlayerWin);
    assert(floor40Boss.tileAt(7, 6) == Tile_Item);
    assert(floor40Boss.itemAt(7, 6) != nullptr);
    assert(floor40Boss.tileAt(7, 12) == Tile_StairsUp);

    // 前三层原版序章：3层先显现包围怪物，等待点击确认后才传送回2层。
    Game openingStory;
    openingStory.goUpFloor(2, 12, false);
    openingStory.goUpFloor(2, 12, false);
    openingStory.initFloor(3);
    openingStory.player().x = 2;
    openingStory.player().y = 12;
    openingStory.player().x = 5;
    openingStory.player().y = 9;
    assert(openingStory.tileAt(6, 7) == Tile_DarkWall);
    assert(openingStory.monsterAt(6, 7) == nullptr);
    openingStory.setTile(6, 9, Tile_Floor);
    assert(openingStory.tryMovePlayer(6, 9) == Game::Move_Ok);
    assert(openingStory.currentFloor() == 3);
    assert(openingStory.floor3PrisonStoryPending());
    assert(openingStory.player().x == 6 && openingStory.player().y == 9);
    assert(openingStory.tryMovePlayer(5, 9) == Game::Move_Block);
    openingStory.resolveFloor3PrisonStory();
    assert(openingStory.currentFloor() == 2);
    assert(openingStory.player().x == 4 && openingStory.player().y == 9);

    // 真实经典塔序章：三层入口触发围攻、固定伤害、攻防降至10并回到二层小偷下方。
    Game classicOpening;
    classicOpening.generateClassicTower();
    classicOpening.player().hp = 1000;
    // 测试目标使用经典塔的 100/100 初始攻防，兼容无 Qt 资源的测试进程。
    classicOpening.player().atk = 100;
    classicOpening.player().def = 100;
    classicOpening.goUpFloor(2, 2);
    classicOpening.goUpFloor(2, 12);
    assert(classicOpening.currentFloor() == 3);
    classicOpening.player().x = 5;
    classicOpening.player().y = 9;
    assert(classicOpening.tileAt(6, 7) == Tile_DarkWall);
    assert(classicOpening.monsterAt(6, 7) == nullptr);
    classicOpening.setTile(6, 9, Tile_Floor);
    assert(classicOpening.tryMovePlayer(6, 9) == Game::Move_Ok);
    assert(classicOpening.currentFloor() == 3);
    assert(classicOpening.floor3PrisonStoryPending());
    assert(classicOpening.player().hp == 1000);
    assert(classicOpening.player().atk == 100);
    assert(classicOpening.player().def == 100);
    assert(classicOpening.floor3TrapActive());
    assert(classicOpening.tileAt(6, 7) == Tile_Monster);
    assert(classicOpening.monsterAt(6, 7) != nullptr);
    assert(classicOpening.monsterAt(6, 7)->GetName() == "长崎素世·幻影");
    for (const auto& position : std::array<std::pair<int, int>, 4>{
             std::pair<int, int>{5, 9}, std::pair<int, int>{7, 9},
             std::pair<int, int>{6, 8}, std::pair<int, int>{6, 10}}) {
        assert(classicOpening.monsterAt(position.first, position.second) != nullptr);
        assert(classicOpening.monsterAt(position.first, position.second)->GetName() == "藤都子SP·魔法警卫");
    }
    classicOpening.resolveFloor3PrisonStory();
    assert(classicOpening.currentFloor() == 2);
    assert(classicOpening.player().hp == 400);
    assert(classicOpening.player().atk == 10);
    assert(classicOpening.player().def == 10);
    classicOpening.goUpFloor(4, 9, false);
    assert(classicOpening.monsterAt(6, 7) == nullptr);
    for (const auto& position : std::array<std::pair<int, int>, 4>{
             std::pair<int, int>{5, 9}, std::pair<int, int>{7, 9},
             std::pair<int, int>{6, 8}, std::pair<int, int>{6, 10}})
        assert(classicOpening.monsterAt(position.first, position.second) == nullptr);

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

    // 管理员调试传送可跨楼层定位到任意地图坐标，但拒绝越界楼层/坐标。
    Game adminTeleport;
    adminTeleport.generateClassicTower();
    assert(!adminTeleport.debugTeleport(0, 7, 7));
    assert(!adminTeleport.debugTeleport(51, 7, 7));
    assert(!adminTeleport.debugTeleport(10, -1, 7));
    assert(!adminTeleport.debugTeleport(10, 7, 15));
    assert(adminTeleport.debugTeleport(10, 4, 8));
    assert(adminTeleport.currentFloor() == 10);
    assert(adminTeleport.player().x == 4 && adminTeleport.player().y == 8);
    assert(adminTeleport.debugTeleport(50, 14, 14));
    assert(adminTeleport.currentFloor() == 50);
    assert(adminTeleport.player().x == 14 && adminTeleport.player().y == 14);

    NPC npc("商人", {"测试"}, nullptr, true, 25,
            std::make_unique<RubyGem>(), 15);
    assert(npc.ClassicId() == 15);
    return 0;
}
