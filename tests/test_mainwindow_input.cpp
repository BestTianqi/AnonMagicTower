#include <QApplication>
#include <QDialog>
#include <QEventLoop>
#include <QImage>
#include <QKeyEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <cassert>
#include <cstdio>

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
    return 0;
}
