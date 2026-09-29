#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTimer>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <vector>

#include "Game/Game.h"
#include "UI/MainWindow.h"
#include "UI/MapWidget.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    const QImage highQualityWalkSheet(
        QStringLiteral(":/images/characters/player_outfits/anon_reference_walk_8x8_hq.png"));
    assert(!highQualityWalkSheet.isNull());
    assert(highQualityWalkSheet.width() % 8 == 0);
    assert(highQualityWalkSheet.height() % 8 == 0);
    assert(highQualityWalkSheet.hasAlphaChannel());

    // Story portraits must resolve to full-size transparent art, not 60px map sprites.
    for (const QString& path : {
             QStringLiteral(":/images/characters/portraits/taki.png"),
             QStringLiteral(":/images/characters/portraits/umiri.png"),
             QStringLiteral(":/images/characters/portraits/sakiko_stage.png"),
             QStringLiteral(":/images/characters/portraits/ririko.png"),
             QStringLiteral(":/images/characters/portraits/variants/anon_worried.png"),
             QStringLiteral(":/images/characters/portraits/variants/soyo_school_angry.png"),
             QStringLiteral(":/images/characters/portraits/variants/soyo_witch_more_battle.png")}) {
        const QImage portrait(path);
        assert(!portrait.isNull());
        assert(portrait.width() >= 200);
        assert(portrait.hasAlphaChannel());
        assert(qAlpha(portrait.pixel(0, 0)) <= 1);
    }

    const QImage stairsUp(QStringLiteral(":/images/runtime/tiles/stairs_up.png"));
    const QImage stairsDown(QStringLiteral(":/images/runtime/tiles/stairs_down.png"));
    assert(!stairsUp.isNull() && !stairsDown.isNull());
    assert(stairsUp.size() == QSize(60, 60));
    assert(stairsDown.size() == QSize(60, 60));
    assert(stairsUp.hasAlphaChannel() && stairsDown.hasAlphaChannel());
    assert(stairsUp != stairsDown);

    for (const QString& path : {
             QStringLiteral(":/images/characters/npcs/marina_pico_240.png"),
             QStringLiteral(":/images/characters/npcs/ririko_pico_240.png"),
             QStringLiteral(":/images/characters/npcs/kokoro_pico_240.png")}) {
        const QImage npcSprite(path);
        assert(!npcSprite.isNull());
        assert(npcSprite.size() == QSize(240, 240));
        assert(npcSprite.hasAlphaChannel());
        assert(qAlpha(npcSprite.pixel(0, 0)) == 0);
    }

    Game scriptedWalls;
    scriptedWalls.generateClassicTower();
    scriptedWalls.debugTeleport(35, 7, 6);
    assert(scriptedWalls.tileAt(7, 4) == Tile_Wall);
    assert(scriptedWalls.tileAt(5, 10) == Tile_Wall);
    scriptedWalls.completeFloor35MichelleStory();
    assert(scriptedWalls.tileAt(7, 4) == Tile_DarkWall);
    assert(scriptedWalls.tileAt(5, 10) == Tile_DarkWall);
    scriptedWalls.debugTeleport(39, 7, 7);
    assert(scriptedWalls.tileAt(7, 11) == Tile_DarkWall);

    // 用正式资源地图核对47层阻击巫师，而不只验证手工摆放的测试地图。
    Game authenticRetreat;
    assert(authenticRetreat.loadDefaultMap());
    assert(authenticRetreat.debugTeleport(47, 2, 9));
    authenticRetreat.player().hp = 1000;
    assert(authenticRetreat.monsterAt(2, 10) != nullptr);
    assert(authenticRetreat.tileAt(2, 11) == Tile_Floor);
    assert(authenticRetreat.tryMovePlayer(2, 10) == Game::Move_Ok);
    assert(authenticRetreat.player().hp == 800);
    assert(authenticRetreat.monsterAt(2, 11) != nullptr);
    const QString retreatSavePath = QDir(QDir::tempPath()).filePath(
        QStringLiteral("mota_floor47_retreat_%1.save").arg(QCoreApplication::applicationPid()));
    assert(authenticRetreat.saveToFile(retreatSavePath.toStdString()));
    Game resumedRetreat;
    assert(resumedRetreat.loadFromFile(retreatSavePath.toStdString()));
    assert(QFile::remove(retreatSavePath));
    assert(resumedRetreat.currentFloor() == 47);
    assert(resumedRetreat.monsterAt(2, 11) != nullptr);
    assert(resumedRetreat.tryMovePlayer(2, 11) == Game::Move_Ok);
    assert(resumedRetreat.player().hp == 600);
    assert(resumedRetreat.monsterAt(2, 12) != nullptr);
    assert(resumedRetreat.tryMovePlayer(2, 12) == Game::Move_Encounter);
    assert(resumedRetreat.player().hp == 600);

    Game upperLaneRetreat;
    assert(upperLaneRetreat.loadDefaultMap());
    assert(upperLaneRetreat.debugTeleport(47, 9, 4));
    upperLaneRetreat.player().hp = 1000;
    assert(upperLaneRetreat.monsterAt(9, 3) != nullptr);
    assert(upperLaneRetreat.tryMovePlayer(9, 3) == Game::Move_Ok);
    assert(upperLaneRetreat.monsterAt(9, 2) != nullptr);
    assert(upperLaneRetreat.tryMovePlayer(9, 2) == Game::Move_Encounter);
    assert(upperLaneRetreat.player().hp == 800);

    Game ordinaryFloor47Wizards;
    assert(ordinaryFloor47Wizards.loadDefaultMap());
    assert(ordinaryFloor47Wizards.debugTeleport(47, 7, 12));
    assert(ordinaryFloor47Wizards.monsterAt(6, 12) != nullptr);
    assert(ordinaryFloor47Wizards.tryMovePlayer(6, 12) == Game::Move_Encounter);
    assert(ordinaryFloor47Wizards.monsterAt(6, 12) != nullptr);
    assert(ordinaryFloor47Wizards.debugTeleport(47, 6, 6));
    assert(ordinaryFloor47Wizards.monsterAt(6, 5) != nullptr);
    assert(ordinaryFloor47Wizards.tryMovePlayer(6, 5) == Game::Move_Encounter);
    assert(ordinaryFloor47Wizards.monsterAt(6, 5) != nullptr);

    Game npcArtGame;
    npcArtGame.loadDefaultMap();
    npcArtGame.setTile(4, 7, Tile_NPC);
    npcArtGame.addNPCAt(4, 7, NPC("米歇尔", {"快来找我。"}));
    MainWindow npcArtWindow(&npcArtGame, nullptr, false);
    npcArtWindow.loadAssets();
    npcArtWindow.show();
    QApplication::processEvents();
    auto* npcArtMap = npcArtWindow.findChild<MapWidget*>();
    assert(npcArtMap);
    const QImage michelleTile = npcArtMap->grab(QRect(4 * TILE_SIZE, 7 * TILE_SIZE,
                                                     TILE_SIZE, TILE_SIZE)).toImage();
    int pinkPixels = 0;
    for (int y = 0; y < michelleTile.height(); ++y)
        for (int x = 0; x < michelleTile.width(); ++x) {
            const QColor pixel = michelleTile.pixelColor(x, y);
            if (pixel.red() > 190 && pixel.blue() > 120 && pixel.green() < 170)
                ++pinkPixels;
        }
    assert(pinkPixels > 10);
    npcArtWindow.close();

    Game game;
    game.loadDefaultMap();

    // Give the input test a deterministic straight corridor without events.
    game.player().x = 3;
    game.player().y = 7;
    for (int x = 3; x <= 10; ++x)
        game.setTile(x, 7, Tile_Floor);

    MainWindow window(&game);
    auto* mapWidget = window.findChild<MapWidget*>();
    assert(mapWidget);
    int finishedSignals = 0;
    QObject::connect(mapWidget, &MapWidget::playerMotionFinished,
                     [&]() { ++finishedSignals; });
    window.show();
    QApplication::processEvents();

    auto* monsterScroll = window.findChild<QScrollArea*>(QStringLiteral("monsterScroll"));
    assert(monsterScroll);
    assert(!monsterScroll->isVisible());

    game.player().AddItem(std::make_unique<MonsterBook>());
    MainWindow handbookWindow(&game);
    handbookWindow.show();
    QApplication::processEvents();
    auto* unlockedMonsterScroll = handbookWindow.findChild<QScrollArea*>(QStringLiteral("monsterScroll"));
    assert(unlockedMonsterScroll && unlockedMonsterScroll->isVisible());
    handbookWindow.close();

    auto* settings = window.findChild<QPushButton*>(QStringLiteral("settingsButton"));
    assert(settings);
    bool settingsOpened = false;
    QTimer modalCloser;
    modalCloser.setInterval(1);
    QObject::connect(&modalCloser, &QTimer::timeout, [&]() {
        if (auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
            settingsOpened = true;
            dialog->reject();
        }
    });
    modalCloser.start();
    settings->click();
    QEventLoop settingsWait;
    QTimer::singleShot(80, &settingsWait, &QEventLoop::quit);
    settingsWait.exec();
    modalCloser.stop();
    mapWidget->setMovementAnimationEnabled(false);
    QKeyEvent press(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier,
                    QString(), false, 1);
    QApplication::sendEvent(&window, &press);
    QEventLoop movementWait;
    QTimer::singleShot(650, &movementWait, &QEventLoop::quit);
    movementWait.exec();
    QKeyEvent release(QEvent::KeyRelease, Qt::Key_Right, Qt::NoModifier,
                      QString(), false, 1);
    QApplication::sendEvent(&window, &release);

    // At 360 px/s, a held key should traverse at least three 60 px tiles.
    const bool heldMovementWithoutAnimation = game.player().x >= 6;

    game.player().x = 3;
    game.player().y = 7;
    mapWidget->snapPlayerToGame();
    mapWidget->setMovementAnimationEnabled(true);
    finishedSignals = 0;
    QApplication::sendEvent(&window, &press);
    QEventLoop animatedMovementWait;
    QTimer::singleShot(650, &animatedMovementWait, &QEventLoop::quit);
    animatedMovementWait.exec();
    QApplication::sendEvent(&window, &release);
    const bool heldMovementWithAnimation = game.player().x >= 6 && finishedSignals > 0;

    std::fprintf(stderr,
                 "settingsOpened=%d noAnimation=%d withAnimation=%d playerX=%d signals=%d\n",
                 settingsOpened, heldMovementWithoutAnimation, heldMovementWithAnimation,
                 game.player().x, finishedSignals);
    assert(settingsOpened);
    assert(heldMovementWithoutAnimation);
    assert(heldMovementWithAnimation);

    // 属性商店必须在一页内呈现全局次数、价格、余额与三个购买后的属性值。
    Game shopGame;
    assert(shopGame.loadDefaultMap());
    assert(shopGame.debugTeleport(4, 2, 2));
    shopGame.player().gold = 19;
    assert(!shopGame.currentFloorData().shops.empty());
    const int shopKey = shopGame.currentFloorData().shops.begin()->first;
    const int shopX = shopKey % shopGame.width();
    const int shopY = shopKey / shopGame.width();
    struct Entry { int px; int py; int key; };
    const Entry candidates[] = {
        {shopX - 1, shopY, Qt::Key_Right}, {shopX + 1, shopY, Qt::Key_Left},
        {shopX, shopY - 1, Qt::Key_Down}, {shopX, shopY + 1, Qt::Key_Up}
    };
    Entry entry = candidates[0];
    for (const auto& candidate : candidates) {
        if (candidate.px >= 2 && candidate.px <= 12 && candidate.py >= 2 && candidate.py <= 12) {
            entry = candidate;
            break;
        }
    }
    shopGame.setTile(entry.px, entry.py, Tile_Floor);
    shopGame.player().x = entry.px;
    shopGame.player().y = entry.py;
    MainWindow shopWindow(&shopGame, nullptr, false);
    shopWindow.show();
    QApplication::processEvents();
    bool shopVerified = false;
    bool shopStorySeen = false;
    QTimer shopCloser;
    shopCloser.setInterval(1);
    QObject::connect(&shopCloser, &QTimer::timeout, [&]() {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (dialog && dialog->findChild<QLabel*>(QStringLiteral("vnText"))) {
            shopStorySeen = true;
            QKeyEvent nextPage(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
            QApplication::sendEvent(dialog, &nextPage);
            return;
        }
        if (!dialog || dialog->objectName() != QStringLiteral("classicShopDialog")) return;
        const auto* price = dialog->findChild<QLabel*>(QStringLiteral("shopPriceBadge"));
        const auto* balance = dialog->findChild<QLabel*>(QStringLiteral("shopBalance"));
        const auto* hpCard = dialog->findChild<QWidget*>(QStringLiteral("shopOffer_hp"));
        const auto* atkCard = dialog->findChild<QWidget*>(QStringLiteral("shopOffer_atk"));
        const auto* defCard = dialog->findChild<QWidget*>(QStringLiteral("shopOffer_def"));
        const auto* hpButton = dialog->findChild<QPushButton*>(QStringLiteral("shopBuy_hp"));
        shopVerified = price && price->text().contains(QString::fromUtf8("20 金币")) &&
            balance && balance->text().contains(QString::fromUtf8("还差 1")) &&
            hpCard && atkCard && defCard && hpButton && !hpButton->isEnabled();
        dialog->reject();
    });
    shopCloser.start();
    QKeyEvent shopPress(QEvent::KeyPress, entry.key, Qt::NoModifier);
    QApplication::sendEvent(&shopWindow, &shopPress);
    shopCloser.stop();
    assert(shopStorySeen);
    assert(shopVerified);
    shopWindow.close();

    // 击败挡路怪物后，只要物理方向键仍按住，就应走进原怪物格并继续前进。
    Game battleGame;
    assert(battleGame.loadDefaultMap());
    battleGame.player().x = 3;
    battleGame.player().y = 7;
    for (int x = 3; x <= 12; ++x)
        battleGame.setTile(x, 7, Tile_Floor);
    battleGame.setTile(4, 7, Tile_Monster);
    battleGame.spawnMonster(4, 7, Monster("训练怪", 1, 0, 0, 0));
    MainWindow battleWindow(&battleGame, nullptr, false);
    battleWindow.show();
    QApplication::processEvents();
    auto* battleMap = battleWindow.findChild<MapWidget*>();
    assert(battleMap);
    battleMap->setMovementAnimationEnabled(false);
    QApplication::sendEvent(&battleWindow, &press);
    QEventLoop battleMovementWait;
    QTimer::singleShot(550, &battleMovementWait, &QEventLoop::quit);
    battleMovementWait.exec();
    QApplication::sendEvent(&battleWindow, &release);
    assert(!battleGame.hasMonsterAt(4, 7));
    assert(battleGame.player().x >= 5);
    const int stoppedAt = battleGame.player().x;
    QEventLoop releasedWait;
    QTimer::singleShot(230, &releasedWait, &QEventLoop::quit);
    releasedWait.exec();
    assert(battleGame.player().x == stoppedAt);

    battleGame.player().x = 3;
    battleGame.player().y = 7;
    battleGame.setTile(4, 7, Tile_Monster);
    battleGame.spawnMonster(4, 7, Monster("训练怪", 1, 0, 0, 0));
    battleMap->snapPlayerToGame();
    battleMap->setMovementAnimationEnabled(true);
    QApplication::sendEvent(&battleWindow, &press);
    QEventLoop animatedBattleWait;
    QTimer::singleShot(700, &animatedBattleWait, &QEventLoop::quit);
    animatedBattleWait.exec();
    QApplication::sendEvent(&battleWindow, &release);
    assert(!battleGame.hasMonsterAt(4, 7));
    assert(battleGame.player().x >= 5);
    battleWindow.close();

    // 致死战斗应在开战前拦下，怪物与玩家状态均保持原样。
    Game lethalGame;
    assert(lethalGame.loadDefaultMap());
    lethalGame.player().x = 3;
    lethalGame.player().y = 7;
    lethalGame.player().hp = 10;
    lethalGame.player().atk = 10;
    lethalGame.player().def = 0;
    lethalGame.setTile(3, 7, Tile_Floor);
    lethalGame.setTile(4, 7, Tile_Monster);
    lethalGame.spawnMonster(4, 7, Monster("致死测试怪", 20, 20, 0, 0));
    MainWindow lethalWindow(&lethalGame, nullptr, false);
    lethalWindow.show();
    QApplication::processEvents();
    QApplication::sendEvent(&lethalWindow, &press);
    QApplication::sendEvent(&lethalWindow, &release);
    assert(lethalGame.player().hp == 10);
    assert(lethalGame.player().x == 3 && lethalGame.player().y == 7);
    assert(lethalGame.hasMonsterAt(4, 7));
    const auto* lethalNotice = lethalWindow.findChild<QLabel*>(QStringLiteral("battleLabel"));
    assert(lethalNotice && lethalNotice->text().contains(QString::fromUtf8("无法击败")));
    lethalWindow.close();

    // 连通区域中的门和怪物都能直接点选，仍须遵守钥匙与战斗规则。
    Game clickTargetGame;
    assert(clickTargetGame.loadDefaultMap());
    clickTargetGame.player().x = 2;
    clickTargetGame.player().y = 3;
    for (int x = 2; x <= 5; ++x) clickTargetGame.setTile(x, 3, Tile_Floor);
    clickTargetGame.player().AddKey(KeyType::Red);
    clickTargetGame.setTile(4, 3, Tile_DoorRed);
    clickTargetGame.spawnMonster(5, 3, Monster("练习怪", 1, 0, 0, 0));
    clickTargetGame.setTile(5, 3, Tile_Monster);
    MainWindow clickTargetWindow(&clickTargetGame, nullptr, false);
    clickTargetWindow.show();
    QApplication::processEvents();
    auto* clickTargetMap = clickTargetWindow.findChild<MapWidget*>();
    assert(clickTargetMap);
    clickTargetMap->setMovementAnimationEnabled(false);
    const auto clickTile = [clickTargetMap](int x, int y) {
        const QPointF pos((x + 0.5) * clickTargetMap->width() / MAP_SIZE,
                          (y + 0.5) * clickTargetMap->height() / MAP_SIZE);
        QMouseEvent click(QEvent::MouseButtonPress, pos, pos,
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(clickTargetMap, &click);
    };
    clickTile(4, 3);
    assert(clickTargetGame.tileAt(4, 3) == Tile_Floor);
    assert(clickTargetGame.player().KeyCount(KeyType::Red) == 0);
    assert(clickTargetGame.player().x == 4 && clickTargetGame.player().y == 3);
    clickTile(5, 3);
    assert(!clickTargetGame.hasMonsterAt(5, 3));
    clickTargetGame.setTile(6, 3, Tile_DarkWall);
    clickTile(6, 3);
    assert(clickTargetGame.tileAt(6, 3) == Tile_Floor);
    assert(clickTargetGame.player().x == 6 && clickTargetGame.player().y == 3);
    clickTargetGame.setTile(7, 3, Tile_DarkWall);
    clickTargetMap->setMovementAnimationEnabled(true);
    clickTile(7, 3);
    assert(clickTargetGame.tileAt(7, 3) == Tile_DarkWall);
    QEventLoop darkWallAnimationWait;
    QTimer::singleShot(450, &darkWallAnimationWait, &QEventLoop::quit);
    darkWallAnimationWait.exec();
    assert(clickTargetGame.tileAt(7, 3) == Tile_Floor);
    assert(clickTargetGame.player().x == 7 && clickTargetGame.player().y == 3);
    clickTargetWindow.close();

    // 47层巫师退敌动画必须先在地图上播完，之后才覆盖魔法领域CG。
    Game retreatUiGame;
    assert(retreatUiGame.debugTeleport(47, 2, 9));
    retreatUiGame.player().hp = 1000;
    retreatUiGame.spawnMonster(2, 10, MonsterDB::get("峰月律SP"));
    retreatUiGame.setTile(2, 10, Tile_Monster);
    MainWindow retreatUiWindow(&retreatUiGame, nullptr, false);
    retreatUiWindow.show();
    QApplication::processEvents();
    auto* retreatUiMap = retreatUiWindow.findChild<MapWidget*>();
    assert(retreatUiMap);
    retreatUiMap->setMovementAnimationEnabled(true);
    bool cgCoveredRetreat = false;
    bool retreatCgShown = false;
    QTimer retreatCgCloser;
    retreatCgCloser.setInterval(10);
    QObject::connect(&retreatCgCloser, &QTimer::timeout, [&]() {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog || !dialog->findChild<QLabel*>(QStringLiteral("vnText"))) return;
        retreatCgShown = true;
        if (retreatUiMap->isMonsterMoving()) cgCoveredRetreat = true;
        QKeyEvent advance(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
        QApplication::sendEvent(dialog, &advance);
    });
    retreatCgCloser.start();
    QKeyEvent retreatPress(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
    QApplication::sendEvent(&retreatUiWindow, &retreatPress);
    QKeyEvent retreatRelease(QEvent::KeyRelease, Qt::Key_Down, Qt::NoModifier);
    QApplication::sendEvent(&retreatUiWindow, &retreatRelease);
    QEventLoop retreatWait;
    QTimer::singleShot(550, &retreatWait, &QEventLoop::quit);
    retreatWait.exec();
    retreatCgCloser.stop();
    assert(retreatCgShown);
    assert(!cgCoveredRetreat);
    retreatUiWindow.close();

    // 剧情不再只能按空格：将普通字母键发送给对话文本，逐页推进直至结束。
    Game storyGame;
    assert(storyGame.loadDefaultMap());
    MainWindow storyWindow(&storyGame, nullptr, true);
    storyWindow.loadAssets();
    storyWindow.show();
    int storyKeyPresses = 0;
    std::vector<QString> openingPages;
    QTimer storyKeys;
    storyKeys.setInterval(20);
    QObject::connect(&storyKeys, &QTimer::timeout, [&]() {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog) return;
        if (++storyKeyPresses > 12) {
            dialog->done(QDialog::Rejected);
            return;
        }
        auto* text = dialog->findChild<QLabel*>(QStringLiteral("vnText"));
        auto* button = dialog->findChild<QPushButton*>();
        assert(text && button);
        if (openingPages.empty() || openingPages.back() != text->text())
            openingPages.push_back(text->text());
        const int keys[] = {Qt::Key_A, Qt::Key_Escape, Qt::Key_Left, Qt::Key_Return};
        const int key = keys[std::min(storyKeyPresses - 1, 3)];
        QKeyEvent pressStory(QEvent::KeyPress, key, Qt::NoModifier);
        QWidget* target = storyKeyPresses == 2 ? static_cast<QWidget*>(button)
                          : storyKeyPresses == 3 ? static_cast<QWidget*>(dialog)
                                                 : static_cast<QWidget*>(text);
        QApplication::sendEvent(target, &pressStory);
    });
    storyKeys.start();
    QEventLoop storyWait;
    QTimer::singleShot(800, &storyWait, &QEventLoop::quit);
    storyWait.exec();
    storyKeys.stop();
    assert(openingPages.size() == 6);
    const QString guide = openingPages[3] + openingPages[4];
    assert(guide.contains(QString::fromUtf8("方向键")));
    assert(guide.contains(QString::fromUtf8("点击")));
    assert(guide.contains(QString::fromUtf8("点击与当前位置连通的地板、道具、怪物和门")));
    assert(guide.contains(QString::fromUtf8("怪物手册")));
    assert(guide.contains(QString::fromUtf8("单词本")));
    assert(guide.contains(QStringLiteral("F5")));
    assert(guide.contains(QStringLiteral("Ctrl+Z")));
    assert(openingPages.back().contains(QString::fromUtf8("第一层")));
    assert(QApplication::activeModalWidget() == nullptr);
    storyWindow.close();

    // 35 层米歇尔只揭示暗道；身份必须留到 50 层才揭晓。
    Game floor35StoryGame;
    floor35StoryGame.generateClassicTower();
    assert(floor35StoryGame.debugTeleport(35, 6, 6));
    floor35StoryGame.setTile(6, 6, Tile_Floor);
    floor35StoryGame.activateFloor35Michelle();
    assert(floor35StoryGame.npcAt(7, 6));
    MainWindow floor35StoryWindow(&floor35StoryGame, nullptr, false);
    floor35StoryWindow.show();
    QApplication::processEvents();
    QStringList floor35Pages;
    QTimer advanceFloor35;
    advanceFloor35.setInterval(10);
    QObject::connect(&advanceFloor35, &QTimer::timeout, [&]() {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog) return;
        auto* text = dialog->findChild<QLabel*>(QStringLiteral("vnText"));
        if (!text) return;
        if (floor35Pages.isEmpty() || floor35Pages.back() != text->text())
            floor35Pages.push_back(text->text());
        QKeyEvent nextPage(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
        QApplication::sendEvent(dialog, &nextPage);
    });
    advanceFloor35.start();
    QKeyEvent walkToMichelle(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
    QApplication::sendEvent(&floor35StoryWindow, &walkToMichelle);
    advanceFloor35.stop();
    assert(!floor35Pages.isEmpty());
    assert(!floor35Pages.join(QStringLiteral(" ")).contains(QString::fromUtf8("摘下粉色熊头套")));
    assert(floor35StoryGame.tileAt(7, 4) == Tile_DarkWall);
    floor35StoryWindow.close();

    Game notebookUiGame;
    assert(notebookUiGame.loadDefaultMap());
    notebookUiGame.player().x = 3;
    notebookUiGame.player().y = 7;
    notebookUiGame.setTile(3, 7, Tile_Floor);
    notebookUiGame.setTile(4, 7, Tile_NPC);
    notebookUiGame.addNPCAt(4, 7, NPC("凛凛子", {"左侧浅色墙里藏有暗道。"}));
    notebookUiGame.player().AddItem(std::make_unique<NoteBook>());
    MainWindow notebookWindow(&notebookUiGame, nullptr, false);
    notebookWindow.show();
    QApplication::processEvents();
    QTimer npcAdvance;
    npcAdvance.setInterval(10);
    QObject::connect(&npcAdvance, &QTimer::timeout, [&]() {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog || !dialog->findChild<QLabel*>(QStringLiteral("vnText"))) return;
        QKeyEvent nextPage(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
        QApplication::sendEvent(dialog, &nextPage);
    });
    npcAdvance.start();
    QApplication::sendEvent(&notebookWindow, &press);
    QApplication::sendEvent(&notebookWindow, &release);
    npcAdvance.stop();
    assert(notebookUiGame.notebookEntries().size() == 1);
    assert(!notebookUiGame.npcAt(4, 7));
    bool notebookOpened = false;
    QTimer notebookCloser;
    notebookCloser.setInterval(10);
    QObject::connect(&notebookCloser, &QTimer::timeout, [&]() {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog || dialog->objectName() != QStringLiteral("notebookDialog")) return;
        const auto* clue = dialog->findChild<QLabel*>(QStringLiteral("notebookEntryText"));
        notebookOpened = clue && clue->text().contains(QString::fromUtf8("左侧浅色墙"));
        dialog->accept();
    });
    notebookCloser.start();
    const auto buttons = notebookWindow.findChildren<QPushButton*>();
    auto notebookButton = std::find_if(buttons.begin(), buttons.end(), [](const QPushButton* button) {
        return button->toolTip().contains(QString::fromUtf8("高松灯的单词本"));
    });
    assert(notebookButton != buttons.end());
    (*notebookButton)->click();
    notebookCloser.stop();
    assert(notebookOpened);
    assert(notebookUiGame.player().InventoryCount() == 1);
    notebookWindow.close();

    Game merchantGame;
    assert(merchantGame.loadDefaultMap());
    merchantGame.player().x = 3;
    merchantGame.player().y = 7;
    merchantGame.player().gold = 100;
    merchantGame.setTile(3, 7, Tile_Floor);
    merchantGame.setTile(4, 7, Tile_Shop);
    ShopData fixedMerchant;
    fixedMerchant.classicNpcId = 7;
    merchantGame.currentFloorData().shops[merchantGame.posKey(4, 7)] = fixedMerchant;
    MainWindow merchantWindow(&merchantGame, nullptr, false);
    merchantWindow.show();
    QApplication::processEvents();
    int merchantDialogs = 0;
    QTimer merchantFlow;
    merchantFlow.setInterval(10);
    QObject::connect(&merchantFlow, &QTimer::timeout, [&]() {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog) return;
        if (++merchantDialogs > 20) {
            dialog->done(QDialog::Rejected);
            return;
        }
        for (auto* button : dialog->findChildren<QPushButton*>()) {
            if (button->text() == QString::fromUtf8("购买")) {
                button->click();
                return;
            }
        }
        if (dialog->findChild<QLabel*>(QStringLiteral("vnText"))) {
            QKeyEvent nextPage(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
            QApplication::sendEvent(dialog, &nextPage);
        }
    });
    merchantFlow.start();
    QApplication::sendEvent(&merchantWindow, &press);
    QApplication::sendEvent(&merchantWindow, &release);
    merchantFlow.stop();
    assert(merchantGame.player().gold == 50);
    assert(merchantGame.player().KeyCount(KeyType::Blue) == 1);
    assert(!merchantGame.shopAt(4, 7));
    assert(merchantGame.notebookEntries().size() == 1);
    assert(merchantGame.notebookEntries()[0].text.find("花门") != std::string::npos);
    merchantWindow.close();

    Game brokeMerchantGame;
    assert(brokeMerchantGame.loadDefaultMap());
    brokeMerchantGame.player().x = 3;
    brokeMerchantGame.player().y = 7;
    brokeMerchantGame.player().gold = 0;
    brokeMerchantGame.setTile(3, 7, Tile_Floor);
    brokeMerchantGame.setTile(4, 7, Tile_Shop);
    brokeMerchantGame.currentFloorData().shops[brokeMerchantGame.posKey(4, 7)] = fixedMerchant;
    MainWindow brokeMerchantWindow(&brokeMerchantGame, nullptr, false);
    brokeMerchantWindow.show();
    QApplication::processEvents();
    QTimer noPurchaseFlow;
    noPurchaseFlow.setInterval(10);
    QObject::connect(&noPurchaseFlow, &QTimer::timeout, [&]() {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog) return;
        for (auto* button : dialog->findChildren<QPushButton*>()) {
            if (button->text() == QString::fromUtf8("金币不足")) {
                button->click();
                return;
            }
        }
        if (dialog->findChild<QLabel*>(QStringLiteral("vnText"))) {
            QKeyEvent nextPage(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
            QApplication::sendEvent(dialog, &nextPage);
        }
    });
    noPurchaseFlow.start();
    QApplication::sendEvent(&brokeMerchantWindow, &press);
    QApplication::sendEvent(&brokeMerchantWindow, &release);
    noPurchaseFlow.stop();
    assert(brokeMerchantGame.shopAt(4, 7));
    assert(brokeMerchantGame.notebookEntries().empty());
    brokeMerchantWindow.close();

    Game blessingGame;
    blessingGame.loadDefaultMap();
    blessingGame.debugTeleport(2, 3, 7);
    blessingGame.setTile(4, 7, Tile_NPC);
    blessingGame.addNPCAt(4, 7, NPC("老头", {"攻防提升。"}, nullptr,
                                    false, 0, nullptr, 33));
    blessingGame.player().atk = 100;
    blessingGame.player().def = 100;
    MainWindow blessingWindow(&blessingGame, nullptr, false);
    blessingWindow.show();
    QApplication::processEvents();
    bool blessingChoiceSeen = false;
    QTimer declineBlessing;
    declineBlessing.setInterval(10);
    QObject::connect(&declineBlessing, &QTimer::timeout, [&]() {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog) return;
        if (const auto* name = dialog->findChild<QLabel*>(QStringLiteral("vnName"));
            name && name->text() == QString::fromUtf8("凛凛子"))
            blessingChoiceSeen = true;
        dialog->reject();
    });
    declineBlessing.start();
    QApplication::sendEvent(&blessingWindow, &press);
    QApplication::sendEvent(&blessingWindow, &release);
    declineBlessing.stop();
    assert(blessingChoiceSeen);
    assert(blessingGame.player().atk == 100 && blessingGame.player().def == 100);
    assert(blessingGame.npcAt(4, 7) != nullptr);
    bool blessingAccepted = false;
    QTimer acceptBlessing;
    acceptBlessing.setInterval(10);
    QObject::connect(&acceptBlessing, &QTimer::timeout, [&]() {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog) return;
        for (auto* button : dialog->findChildren<QPushButton*>()) {
            if (button->text() == QString::fromUtf8("确认提升")) {
                blessingAccepted = true;
                button->click();
                return;
            }
        }
        if (dialog->findChild<QLabel*>(QStringLiteral("vnText"))) {
            QKeyEvent nextPage(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
            QApplication::sendEvent(dialog, &nextPage);
        }
    });
    acceptBlessing.start();
    QApplication::sendEvent(&blessingWindow, &press);
    QApplication::sendEvent(&blessingWindow, &release);
    acceptBlessing.stop();
    assert(blessingAccepted);
    assert(blessingGame.player().atk == 103 && blessingGame.player().def == 103);
    assert(blessingGame.npcAt(4, 7) == nullptr);
    blessingWindow.close();

    Game toolGame;
    toolGame.loadDefaultMap();
    toolGame.debugTeleport(1, 5, 5);
    toolGame.setTile(4, 5, Tile_Wall);
    toolGame.setTile(5, 4, Tile_DoorGreen);
    toolGame.setTile(6, 5, Tile_Monster);
    toolGame.spawnMonster(6, 5, MonsterDB::getByIndex(0));
    toolGame.player().AddItem(std::make_unique<EarthquakeScroll>());
    toolGame.player().AddItem(std::make_unique<MagicKey>());
    toolGame.player().AddItem(std::make_unique<Bomb>());
    MainWindow toolWindow(&toolGame, nullptr, false);
    toolWindow.loadAssets();
    toolWindow.show();
    QApplication::processEvents();
    const auto findTool = [&](const QString& name) -> QPushButton* {
        for (auto* button : toolWindow.findChildren<QPushButton*>())
            if (button->toolTip().startsWith(name)) return button;
        return nullptr;
    };
    assert(findTool(QString::fromUtf8("Mujica舞台震响卷")) != nullptr);
    const auto cancelTool = [&](const QString& name) {
        auto* button = findTool(name);
        assert(button);
        bool sawConfirmation = false;
        QTimer cancel;
        cancel.setInterval(10);
        QObject::connect(&cancel, &QTimer::timeout, [&]() {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            if (const auto* text = dialog->findChild<QLabel*>(QStringLiteral("vnText"));
                text && text->text().contains(name))
                sawConfirmation = true;
            dialog->reject();
        });
        cancel.start();
        button->click();
        cancel.stop();
        assert(sawConfirmation);
        assert(toolGame.player().InventoryCount() == 3);
        assert(toolGame.tileAt(4, 5) == Tile_Wall);
        assert(toolGame.tileAt(5, 4) == Tile_DoorGreen);
        assert(toolGame.hasMonsterAt(6, 5));
    };
    cancelTool(QString::fromUtf8("Mujica舞台震响卷"));
    cancelTool(QString::fromUtf8("大黄门钥匙"));
    cancelTool(QString::fromUtf8("Mujica烟雾弹"));
    const auto confirmTool = [&](const QString& name) {
        auto* button = findTool(name);
        assert(button);
        bool sawConfirmation = false;
        QTimer accept;
        accept.setInterval(10);
        QObject::connect(&accept, &QTimer::timeout, [&]() {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            for (auto* choice : dialog->findChildren<QPushButton*>()) {
                if (choice->text() == QString::fromUtf8("确认使用")) {
                    sawConfirmation = true;
                    choice->click();
                    return;
                }
            }
        });
        accept.start();
        button->click();
        accept.stop();
        assert(sawConfirmation);
    };
    confirmTool(QString::fromUtf8("Mujica舞台震响卷"));
    assert(toolGame.tileAt(4, 5) == Tile_Floor);
    assert(toolGame.player().InventoryCount() == 2);
    confirmTool(QString::fromUtf8("大黄门钥匙"));
    assert(toolGame.tileAt(5, 4) == Tile_Floor);
    assert(toolGame.player().InventoryCount() == 1);
    confirmTool(QString::fromUtf8("Mujica烟雾弹"));
    assert(!toolGame.hasMonsterAt(6, 5));
    assert(toolGame.player().InventoryCount() == 0);
    toolWindow.close();

    // 六类属性拾取物在格子上标出自身实际加成，其他道具不出现属性标记。
    Game gainGame;
    assert(gainGame.loadDefaultMap());
    gainGame.setTile(3, 7, Tile_Item);
    MapWidget gainMap(&gainGame);
    gainMap.show();
    QApplication::processEvents();
    const auto gainTile = QRect(3 * TILE_SIZE, 7 * TILE_SIZE, TILE_SIZE, TILE_SIZE);
    const auto renderGainTile = [&]() {
        gainMap.update();
        QApplication::processEvents();
        QImage frame(gainMap.size(), QImage::Format_ARGB32_Premultiplied);
        frame.fill(Qt::transparent);
        gainMap.render(&frame);
        return frame.copy(gainTile);
    };
    Treasure nonStatItem(1);
    assert(itemGainBadge(nonStatItem).text.isEmpty());
    gainGame.addItemAt(3, 7, std::make_unique<Treasure>(1));
    const QImage unmarkedTile = renderGainTile();
    const auto expectVisibleGain = [&](std::unique_ptr<Item> item, const QString& expected) {
        assert(itemGainBadge(*item).text == expected);
        gainGame.addItemAt(3, 7, std::move(item));
        assert(unmarkedTile != renderGainTile());
    };
    expectVisibleGain(std::make_unique<Weapon>(10, "爱音拨片"), QString::fromUtf8("攻+10"));
    expectVisibleGain(std::make_unique<Armor>(20, "素世谱架"), QString::fromUtf8("防+20"));
    expectVisibleGain(std::make_unique<HolyShield>(50), QString::fromUtf8("防+50"));
    expectVisibleGain(std::make_unique<RubyGem>(2), QString::fromUtf8("攻+2"));
    expectVisibleGain(std::make_unique<SapphireGem>(3), QString::fromUtf8("防+3"));
    expectVisibleGain(std::make_unique<SmallPotion>(100), QString::fromUtf8("血+100"));
    expectVisibleGain(std::make_unique<LargePotion>(400), QString::fromUtf8("血+400"));

    const QString originalAppName = QCoreApplication::applicationName();
    const QString saveTestAppName = QStringLiteral("mota_quicksave_test_%1")
        .arg(QCoreApplication::applicationPid());
    QCoreApplication::setApplicationName(saveTestAppName);
    Game quickGame;
    assert(quickGame.loadDefaultMap());
    quickGame.player().hp = 321;
    MainWindow quickWindow(&quickGame, nullptr, false);
    quickWindow.show();
    QApplication::processEvents();
    auto* quickSaveButton = quickWindow.findChild<QPushButton*>(QStringLiteral("quickSaveButton"));
    auto* saveFeedback = quickWindow.findChild<QLabel*>(QStringLiteral("battleLabel"));
    assert(quickSaveButton && saveFeedback);
    quickSaveButton->click();
    const QString quickDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString quickPath = QDir(quickDir).filePath(QStringLiteral("quicksave.sav"));
    assert(QFileInfo::exists(quickPath));
    assert(saveFeedback->isVisible());
    assert(saveFeedback->text().contains(QString::fromUtf8("即时存档已保存")));
    assert(QApplication::activeModalWidget() == nullptr);
    quickGame.player().hp = 10;
    bool quickSlotListed = false;
    QTimer readQuickSave;
    readQuickSave.setInterval(10);
    QObject::connect(&readQuickSave, &QTimer::timeout, [&]() {
        auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        if (!dialog) return;
        auto* slotList = dialog->findChild<QListWidget*>();
        if (!slotList || slotList->count() == 0) return;
        quickSlotListed = slotList->item(0)->text().contains(QString::fromUtf8("即时存档"));
        if (!quickSlotListed) {
            dialog->reject();
            return;
        }
        slotList->setCurrentRow(0);
        for (auto* button : dialog->findChildren<QPushButton*>()) {
            if (button->text() == QString::fromUtf8("读取选中槽位")) {
                button->click();
                return;
            }
        }
    });
    auto* loadButton = quickWindow.findChild<QPushButton*>(QStringLiteral("loadButton"));
    assert(loadButton);
    readQuickSave.start();
    loadButton->click();
    readQuickSave.stop();
    assert(quickSlotListed);
    assert(quickGame.player().hp == 321);
    quickGame.player().hp = 7;
    bool quickLoadPoppedModal = false;
    QTimer dismissQuickLoadPopup;
    dismissQuickLoadPopup.setInterval(10);
    QObject::connect(&dismissQuickLoadPopup, &QTimer::timeout, [&]() {
        if (auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
            quickLoadPoppedModal = true;
            dialog->reject();
        }
    });
    dismissQuickLoadPopup.start();
    QKeyEvent quickLoadKey(QEvent::KeyPress, Qt::Key_F9, Qt::NoModifier);
    QApplication::sendEvent(&quickWindow, &quickLoadKey);
    dismissQuickLoadPopup.stop();
    assert(quickGame.player().hp == 321);
    assert(!quickLoadPoppedModal);
    quickWindow.close();
    assert(QFile::remove(quickPath));
    QDir().rmdir(quickDir);
    QCoreApplication::setApplicationName(originalAppName);
    return 0;
}
