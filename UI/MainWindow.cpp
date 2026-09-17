#include "MainWindow.h"
#include "MapWidget.h"
#include "MapEditor.h"
#include "BattleFeedback.h"
#include "Entities/MonsterDB.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMessageBox>
#include <QString>
#include <QDir>
#include <QDialog>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFrame>
#include <QSpinBox>
#include <QCheckBox>
#include <QInputDialog>
#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include <QTabWidget>
#include <QIcon>
#include <QRandomGenerator>
#include <QLocale>
#include <QMap>
#include <QSettings>
#include <QStandardPaths>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QApplication>
#include <algorithm>
#include <iterator>

static QString formatNumber(int value)
{
    return QLocale(QLocale::Chinese, QLocale::China).toString(value);
}

static QString dialoguePortraitPath(QString path)
{
    // Galgame 对话统一使用高分辨率立绘；保留显式指定的剧情专属表情。
    if (path.endsWith(QStringLiteral("/anon.png")))
        return QStringLiteral(":/images/characters/portraits/variants/anon_calm.png");
    if (path.endsWith(QStringLiteral("/soyo.png")))
        return QStringLiteral(":/images/characters/portraits/variants/soyo_school_calm.png");
    if (path.endsWith(QStringLiteral("/yukina.png")))
        return QStringLiteral(":/images/characters/portraits/variants/yukina_new_school_calm.png");
    if (path.endsWith(QStringLiteral("/kasumi.png")))
        return QStringLiteral(":/images/characters/portraits/variants/kasumi_new_school_joy.png");
    return path;
}

static QString monsterPortraitPath(const std::string& name)
{
    // 与 MapWidget::loadAssets 中的原版怪物 ID 映射保持一致。
    // 怪物名称包含“原版形态”后缀，不能只靠角色名 contains 判断。
    const int generatedIndex = MonsterDB::indexOf(name);
    if (generatedIndex >= 0) {
        const QString generated = QString(":/images/characters/portraits/monster_variants/monster_%1.png")
                                      .arg(generatedIndex + 1, 2, 10, QChar('0'));
        if (!QPixmap(generated).isNull())
            return generated;
    }
    static const char* themedPortraits[] = {
        ":/images/characters/portraits/rana.png",
        ":/images/characters/portraits/mutsumi.png",
        ":/images/characters/portraits/sakiko.png",
        ":/images/characters/portraits/tomori.png",
        ":/images/characters/portraits/taki.png",
        ":/images/characters/portraits/nyamu.png",
        ":/images/characters/portraits/uika.png",
        ":/images/characters/portraits/umiri.png",
        ":/images/characters/portraits/arale.png",
        ":/images/characters/portraits/nonoka.png",
        ":/images/characters/portraits/viola.png",
        ":/images/characters/portraits/ritsu.png",
        ":/images/characters/portraits/miyako.png",
        ":/images/characters/portraits/yuno.png",
        ":/images/characters/portraits/nonoka_stage.png",
        ":/images/characters/portraits/yukina.png",
        ":/images/characters/portraits/kasumi.png",
        ":/images/characters/portraits/rana_stage.png",
        ":/images/characters/portraits/tomori_stage.png",
        ":/images/characters/portraits/soyo_stage.png",
        ":/images/characters/portraits/taki_stage.png",
        ":/images/characters/portraits/sakiko_stage.png",
        ":/images/characters/portraits/viola_stage.png",
        ":/images/characters/portraits/mutsumi_stage.png",
        ":/images/characters/portraits/soyo.png",
        ":/images/characters/portraits/arale_stage.png",
        ":/images/characters/portraits/ritsu_stage.png",
        ":/images/characters/portraits/nyamu_stage.png",
        ":/images/characters/portraits/yuno_stage.png",
        ":/images/characters/portraits/umiri_stage.png",
        ":/images/characters/portraits/miyako_stage.png",
        ":/images/characters/portraits/uika_stage.png",
        ":/images/characters/portraits/soyo_stage.png",
        ":/images/characters/portraits/soyo_stage.png"
    };
    const int monsterIndex = MonsterDB::indexOf(name);
    if (monsterIndex >= 0 && monsterIndex < static_cast<int>(std::size(themedPortraits)))
        return QString::fromUtf8(themedPortraits[monsterIndex]);

    // 自定义怪物没有原版 ID 时，仍按名称尝试匹配主题角色。
    const QString n = QString::fromStdString(name);
    if (n.contains(QString::fromUtf8("长崎素世"))) return QStringLiteral(":/images/characters/portraits/soyo.png");
    if (n.contains(QString::fromUtf8("高松灯"))) return QStringLiteral(":/images/characters/portraits/tomori.png");
    if (n.contains(QString::fromUtf8("椎名立希"))) return QStringLiteral(":/images/characters/portraits/taki.png");
    if (n.contains(QString::fromUtf8("要乐奈"))) return QStringLiteral(":/images/characters/portraits/rana.png");
    if (n.contains(QString::fromUtf8("八幡海铃"))) return QStringLiteral(":/images/characters/portraits/umiri.png");
    if (n.contains(QString::fromUtf8("祐天寺若麦"))) return QStringLiteral(":/images/characters/portraits/nyamu.png");
    if (n.contains(QString::fromUtf8("若叶睦"))) return QStringLiteral(":/images/characters/portraits/mutsumi.png");
    if (n.contains(QString::fromUtf8("三角初华"))) return QStringLiteral(":/images/characters/portraits/uika.png");
    if (n.contains(QString::fromUtf8("丰川祥子"))) return QStringLiteral(":/images/characters/portraits/sakiko.png");
    if (n.contains(QString::fromUtf8("薇欧拉"))) return QStringLiteral(":/images/characters/portraits/viola.png");
    if (n.contains(QString::fromUtf8("仲町"))) return QStringLiteral(":/images/characters/portraits/arale.png");
    if (n.contains(QString::fromUtf8("宫永"))) return QStringLiteral(":/images/characters/portraits/nonoka.png");
    if (n.contains(QString::fromUtf8("峰月"))) return QStringLiteral(":/images/characters/portraits/ritsu.png");
    if (n.contains(QString::fromUtf8("藤都子"))) return QStringLiteral(":/images/characters/portraits/miyako.png");
    if (n.contains(QString::fromUtf8("千石"))) return QStringLiteral(":/images/characters/portraits/yuno.png");
    if (n.contains(QString::fromUtf8("凑友希那"))) return QStringLiteral(":/images/characters/portraits/yukina.png");
    if (n.contains(QString::fromUtf8("户山香澄"))) return QStringLiteral(":/images/characters/portraits/kasumi.png");
    if (n.contains(QString::fromUtf8("Soyorin"))) return QStringLiteral(":/images/characters/portraits/soyo_stage.png");
    return {};
}

static void applyRuntimeArtSkin(QWidget& widget)
{
    widget.setStyleSheet(
        "QDialog, QMessageBox { background-image: url(:/images/runtime/ui/panel_texture.png); color: #f1e8d0; }"
        "QGroupBox, QTabWidget::pane, QListWidget, QScrollArea { background: rgba(14,15,25,210); color: #f1e8d0; border: 2px solid #777080; }"
        "QLineEdit, QTextEdit, QSpinBox, QComboBox { background: rgba(14,15,25,220); color: white; border: 1px solid #9b93a3; padding: 4px; }"
        "QLabel { color: #f1e8d0; }"
        "QPushButton { color: #fff7d0; border-image: url(:/images/runtime/ui/button_texture.png) 18 24 18 24 stretch stretch; padding: 7px 14px; font-weight: 700; min-height: 24px; }"
        "QPushButton:hover { color: white; }"
    );
    const QString buttonArt =
        "QPushButton { color: #fff7d0; border-image: url(:/images/runtime/ui/button_texture.png) 18 24 18 24 stretch stretch; padding: 7px 14px; font-weight: 700; min-height: 24px; }"
        "QPushButton:hover { color: white; }";
    for (QPushButton* button : widget.findChildren<QPushButton*>())
        button->setStyleSheet(buttonArt);
}

static QString itemIconPath(const std::string& rawName)
{
    const QString name = QString::fromStdString(Game::canonicalItemName(rawName));
    if (name == QString::fromUtf8("红色Live票")) return QStringLiteral(":/images/runtime/items/mygo/live_ticket_red.png");
    if (name == QString::fromUtf8("蓝色Live票")) return QStringLiteral(":/images/runtime/items/mygo/live_ticket_blue.png");
    if (name == QString::fromUtf8("黄色Live票")) return QStringLiteral(":/images/runtime/items/mygo/live_ticket_yellow.png");
    if (name == QString::fromUtf8("大黄门钥匙")) return QStringLiteral(":/images/runtime/items/key_magic.png");
    if (name == QStringLiteral("Weapon")) return QStringLiteral(":/images/runtime/items/weapon.png");
    if (name == QStringLiteral("Armor")) return QStringLiteral(":/images/runtime/items/armor.png");
    if (name == QStringLiteral("Treasure")) return QStringLiteral(":/images/runtime/items/treasure.png");
    if (name == QString::fromUtf8("现场补给")) return QStringLiteral(":/images/runtime/items/mygo/mygo_support_badge_red.png");
    if (name == QString::fromUtf8("灯的热牛奶")) return QStringLiteral(":/images/runtime/items/mygo/tomori_warm_milk.png");
    if (name == QString::fromUtf8("爱音能量饮")) return QStringLiteral(":/images/runtime/items/mygo/anon_energy_drink.png");
    if (name == QString::fromUtf8("MyGO应援红章")) return QStringLiteral(":/images/runtime/items/mygo/mygo_support_badge_red.png");
    if (name == QString::fromUtf8("Mujica应援蓝章")) return QStringLiteral(":/images/runtime/items/mygo/mujica_support_badge_blue.png");
    if (name == QString::fromUtf8("爱音拨片")) return QStringLiteral(":/images/runtime/items/mygo/anon_guitar_pick.png");
    if (name == QString::fromUtf8("立希鼓棒")) return QStringLiteral(":/images/runtime/items/mygo/taki_drumsticks.png");
    if (name == QString::fromUtf8("乐奈猫爪")) return QStringLiteral(":/images/runtime/items/mygo/rana_cat_claw.png");
    if (name == QString::fromUtf8("灯的麦克风")) return QStringLiteral(":/images/runtime/items/mygo/tomori_microphone.png");
    if (name == QString::fromUtf8("睦的贝斯")) return QStringLiteral(":/images/runtime/items/mygo/mutsumi_bass.png");
    if (name == QString::fromUtf8("素世谱架")) return QStringLiteral(":/images/runtime/items/mygo/soyo_music_stand.png");
    if (name == QString::fromUtf8("海铃节拍器")) return QStringLiteral(":/images/runtime/items/mygo/umiri_metronome.png");
    if (name == QString::fromUtf8("初华舞台耳返")) return QStringLiteral(":/images/runtime/items/mygo/uika_in_ear.png");
    if (name == QString::fromUtf8("祥子黑色乐谱")) return QStringLiteral(":/images/runtime/items/mygo/sakiko_sheet_music.png");
    if (name == QString::fromUtf8("Mujica终幕面具")) return QStringLiteral(":/images/runtime/items/mygo/mujica_finale_mask.png");
    if (name == QString::fromUtf8("爱音手机") || name == QString::fromUtf8("楼层传送器")) return QStringLiteral(":/images/runtime/items/mygo/anon_smartphone.png");
    if (name == QString::fromUtf8("Mujica镜面舞台票")) return QStringLiteral(":/images/runtime/items/mygo/mujica_mirror_ticket.png");
    if (name == QString::fromUtf8("高松灯的单词本")) return QStringLiteral(":/images/runtime/items/mygo/tomori_lyric_notebook.png");
    if (name == QString::fromUtf8("怪物手册")) return QStringLiteral(":/images/runtime/items/glasses.png");
    if (name == QString::fromUtf8("立希水壶")) return QStringLiteral(":/images/runtime/items/mygo/rikki_water_kettle.png");
    if (name == QString::fromUtf8("爱音自拍眼镜")) return QStringLiteral(":/images/runtime/items/mygo/anon_selfie_glasses.png");
    if (name == QString::fromUtf8("睦的镐子")) return QStringLiteral(":/images/runtime/items/mygo/mutsumi_pickaxe_toolbox.png");
    if (name == QString::fromUtf8("破墙锤")) return QStringLiteral(":/images/runtime/items/wall_breaker.png");
    if (name == QString::fromUtf8("Mujica烟雾弹")) return QStringLiteral(":/images/runtime/items/mygo/mujica_smoke_bomb.png");
    if (name == QString::fromUtf8("Mujica舞台震响卷")) return QStringLiteral(":/images/runtime/items/mygo/mujica_stage_quake_scroll.png");
    if (name == QString::fromUtf8("MyGO和解徽章") || name == QString::fromUtf8("MyGO团结徽章")) return QStringLiteral(":/images/runtime/items/mygo/mygo_reconciliation_badge.png");
    if (name == QString::fromUtf8("祥子指挥棒")) return QStringLiteral(":/images/runtime/items/mygo/sakiko_conductor_baton.png");
    if (name == QString::fromUtf8("海铃冷静指令")) return QStringLiteral(":/images/runtime/items/mygo/umiri_calm_command.png");
    if (name == QString::fromUtf8("乐队护盾贴")) return QStringLiteral(":/images/runtime/items/mygo/band_shield_sticker.png");
    if (name == QString::fromUtf8("立希企鹅挂件")) return QStringLiteral(":/images/runtime/items/mygo/rikki_penguin_keychain.png");
    if (name == QString::fromUtf8("乐奈抹茶芭菲")) return QStringLiteral(":/images/runtime/items/mygo/rana_matcha_parfait.png");
    if (name == QString::fromUtf8("乐奈幸运硬币")) return QStringLiteral(":/images/runtime/items/mygo/rana_lucky_coin.png");
    if (name == QString::fromUtf8("舞台升降卡")) return QStringLiteral(":/images/runtime/items/mygo/stage_lift_card.png");
    if (name == QString::fromUtf8("撤场通行卡")) return QStringLiteral(":/images/runtime/items/mygo/exit_pass.png");
    if (name == QString::fromUtf8("灯的热牛奶")) return QStringLiteral(":/images/runtime/items/potion_small.png");
    if (name == QString::fromUtf8("爱音能量饮")) return QStringLiteral(":/images/runtime/items/potion_large.png");
    return QStringLiteral(":/images/runtime/items/artifact.png");
}

MainWindow::MainWindow(Game* game, QWidget* parent)
    : QWidget(parent), m_game(game)
{
    setFocusPolicy(Qt::StrongFocus);
    ui.setupUi(this);
    setWindowTitle(QString::fromUtf8("MYGO!!!!! × Ave Mujica：梦限大魔塔"));
    setMinimumSize(1100, 760);
    setStyleSheet(
        "QWidget#MainWindow { background: #111322; color: #e8e9f2; }"
        "QWidget#sidePanel { background-color: rgba(10,12,28,238); background-image: url(:/images/runtime/ui/panel_texture.png); border-left: 2px solid #d35d9b; }"
        "QLabel#gameTitle { color: #ff76b6; font-size: 21px; font-weight: 900; letter-spacing: 1px; }"
        "QLabel#gameSubtitle { color: #9fd8ff; font-size: 11px; font-weight: 700; letter-spacing: 2px; }"
        "QLabel { color: #dfe3f5; }"
        "QLabel#floorLabel { color: #f5c96a; font-size: 20px; font-weight: 700; padding: 8px 4px; }"
        "QLabel#hpLabel { color: #ff7188; font-size: 16px; font-weight: 700; }"
        "QLabel#atkLabel, QLabel#defLabel { color: #9fc5ff; font-size: 14px; font-weight: 600; }"
        "QLabel#goldLabel { color: #ffd66b; font-size: 14px; font-weight: 600; }"
        "QLabel#keysLabel, QLabel#invItemsLabel { color: #c2c8df; font-size: 13px; }"
        "QPushButton { color: #fff7d0; border-image: url(:/images/runtime/ui/button_texture.png) 18 24 18 24 stretch stretch; padding: 8px 14px; font-size: 14px; font-weight: 700; }"
        "QPushButton:hover { color: white; }"
    );
    const QString buttonArt =
        "QPushButton { color: #fff7d0; border-image: url(:/images/runtime/ui/button_texture.png) 18 24 18 24 stretch stretch; padding: 2px 4px; font-size: 11px; font-weight: 700; }"
        "QPushButton:hover { color: white; }";
    for (QPushButton* button : {ui.saveButton, ui.quickSaveButton,
                                ui.undoButton, ui.loadButton, ui.settingsButton,
                                ui.modButton})
        button->setStyleSheet(buttonArt);
    ui.undoButton->setEnabled(false);
    ui.saveButton->setText(QString::fromUtf8("保存"));
    ui.quickSaveButton->setText(QString::fromUtf8("即时存档"));
    ui.undoButton->setText(QString::fromUtf8("撤销"));
    ui.loadButton->setText(QString::fromUtf8("读取"));
    ui.settingsButton->setText(QString::fromUtf8("设置"));
    ui.modButton->setText(QString::fromUtf8("修改器"));
    ui.saveButton->setToolTip(QString::fromUtf8("保存游戏"));
    ui.quickSaveButton->setToolTip(QString::fromUtf8("即时存档 (F5)"));
    ui.undoButton->setToolTip(QString::fromUtf8("撤销 (Ctrl+Z)"));
    ui.loadButton->setToolTip(QString::fromUtf8("读取存档"));
    ui.settingsButton->setToolTip(QString::fromUtf8("设置"));
    ui.modButton->setToolTip(QString::fromUtf8("修改器"));
    ui.monsterScroll->setStyleSheet(
        "QScrollArea { background-color: rgba(17,19,34,220); background-image: url(:/images/runtime/ui/panel_texture.png); border: 2px solid #777080; }"
        "QScrollBar:vertical { background: #171824; width: 9px; }"
        "QScrollBar::handle:vertical { background: #777080; min-height: 24px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");
    ui.monsterPanel->setStyleSheet(
        "background-color: rgba(17,19,34,220); background-image: url(:/images/runtime/ui/panel_texture.png);");

    ui.mapWidget->setGame(m_game);
    ui.mapWidget->setFocusPolicy(Qt::NoFocus);
    QSettings settings(QStringLiteral("MyGO-Mota"), QStringLiteral("MyGO-Mota"));
    m_battleFeedbackEnabled = settings.value(QStringLiteral("battleFeedback"), true).toBool();
    ui.mapWidget->setMovementAnimationEnabled(
        settings.value(QStringLiteral("movementAnimation"), true).toBool());
    connect(ui.mapWidget, &MapWidget::tileClicked, this, [this](int x, int y) {
        if (m_game->player().hp <= 0 || ui.mapWidget->isSceneAnimating() ||
            m_pendingTeleport.active) return;
        if (m_game->floor3PrisonStoryPending()) {
            showFloor3PrisonVisualNovel();
            ui.mapWidget->update();
            updateHUD();
            return;
        }
        const int floorBefore = m_game->currentFloor();
        const Item* clickedItem = m_game->itemAt(x, y);
        const QString pickedName = clickedItem
            ? QString::fromStdString(Game::canonicalItemName(clickedItem->GetName()))
            : QString();
        const QString pickedDescription = clickedItem ? getItemDescription(clickedItem) : QString();
        if (!m_game->beginTeleportPlayerTo(x, y)) {
            ui.mapWidget->snapPlayerToGame();
            return;
        }
        // 只有路径规划成功才记录撤销点；点击墙体或不可达位置不应制造“假操作”。
        captureUndoSnapshot();
        const bool animateTeleport = m_game->lastTeleportNeedsAnimation();
        const auto teleportPath = m_game->takeLastTeleportPath();
        if (animateTeleport && teleportPath.size() > 1 &&
            ui.mapWidget->movementAnimationEnabled()) {
            m_pendingTeleport.active = true;
            m_pendingTeleport.x = x;
            m_pendingTeleport.y = y;
            m_pendingTeleport.floorBefore = floorBefore;
            m_pendingTeleport.pickedName = pickedName;
            m_pendingTeleport.pickedDescription = pickedDescription;
            ui.mapWidget->playPlayerPath(teleportPath);
            return;
        }

        const auto result = m_game->completeTeleportPlayerTo();
        ui.mapWidget->snapPlayerToGame();
        startMonsterMovementAnimation();
        handleTeleportResult(x, y, floorBefore, pickedName, pickedDescription, result);
    });
    updateHUD();
    m_movementQueueTimer.setInterval(16);
    connect(&m_movementQueueTimer, &QTimer::timeout, this, &MainWindow::flushPendingMove);
    // 动画结束的同一帧先提交鼠标瞬移的目标交互，再衔接键盘移动队列。
    connect(ui.mapWidget, &MapWidget::playerMotionFinished, this, [this]() {
        if (m_pendingTeleport.active) {
            completePendingTeleport();
            return;
        }
        flushPendingMove();
    });
    connect(ui.mapWidget, &MapWidget::monsterMotionFinished, this, [this]() {
        if (m_floor32KnightStoryFloor < 0)
            return;
        const int floorBefore = m_floor32KnightStoryFloor;
        m_floor32KnightStoryFloor = -1;
        showFloor32KnightStoryIfNeeded(floorBefore);
        ui.mapWidget->update();
        updateHUD();
    });
    m_movementQueueTimer.start();
    connect(&m_battleFeedbackTimer, &QTimer::timeout, this, [this]() {
        if (ui.battleLabel) ui.battleLabel->setVisible(false);
    });

    connect(ui.saveButton, &QPushButton::clicked, this, [this]() {
        showSaveLoadDialog(true);
    });

    connect(ui.loadButton, &QPushButton::clicked, this, [this]() {
        showSaveLoadDialog(false);
    });

    connect(ui.modButton, &QPushButton::clicked, this, &MainWindow::showModifier);
}

void MainWindow::handleTeleportResult(int x, int y, int floorBefore,
                                      const QString& pickedName,
                                      const QString& pickedDescription,
                                      Game::MoveResult result)
{
    switch (result) {
        case Game::Move_Pickup:
        case Game::Move_Ok:
            if (result == Game::Move_Pickup && !pickedName.isEmpty())
                showBattleFeedback(QString::fromUtf8("获得 %1：%2").arg(pickedName).arg(
                    QString(pickedDescription).replace(QString::fromUtf8("（未生效）"), QString::fromUtf8("（已生效）"))));
            if (m_game->floor3PrisonStoryPending())
                showPrisonTrapPrompt();
            else if (floorBefore == 3 && m_game->currentFloor() == 2)
                showOpeningPrisonStory();
            showFloor20VampireStoryIfNeeded(floorBefore);
            showFloor33TrapStoryIfNeeded(floorBefore);
            showFloor32KnightStoryAfterMovement(floorBefore);
            ui.mapWidget->update();
            updateHUD();
            break;
        case Game::Move_NPC:
            showNPCDialog(x, y);
            ui.mapWidget->update();
            updateHUD();
            break;
        case Game::Move_Shop:
            showShopDialog(x, y);
            ui.mapWidget->update();
            updateHUD();
            break;
        case Game::Move_Encounter: {
            // 点击怪物格也走统一战斗入口；瞬移后玩家坐标已在目标格，
            // 因而战斗结束后可以继续从该格移动或拾取战利品。
            const bool knightFight = floorBefore == 32 && m_game->monsterAt(x, y) &&
                m_game->monsterAt(x, y)->GetName() == "幼年长崎素世·骑士队长";
            std::vector<std::string> log;
            const auto fightResult = m_game->fightAt(x, y, log);
            showBattleFeedback(QString::fromStdString(summarizeBattleLog(log)));
            if (knightFight && fightResult == Game::Fight_PlayerWin)
                showVisualNovelDialogue({
                    {QString::fromUtf8("骑士队长"), QString::fromUtf8("哼！这次算你赢了，我先逃回右上角的后台入口！"),
                     QStringLiteral(":/images/characters/portraits/variants/soyo_child_sad.png"), QStringLiteral("#b58cff")},
                    {QString::fromUtf8("千早爱音"), QString::fromUtf8("别想逃走……他已经逃回 (12,2) 的后台入口了。"),
                     QStringLiteral(":/images/characters/portraits/variants/anon_confident.png"), QStringLiteral("#ff8fc7")}
                });
            // 战败对白结束后才开始撤退动画；普通战斗仍立即播放其他脚本移动。
            startMonsterMovementAnimation();
            ui.mapWidget->update();
            updateHUD();
            if (fightResult == Game::Fight_GameWin) {
                gameWin();
            } else if (fightResult == Game::Fight_PlayerDead) {
                gameOver();
            }
            break;
        }
        case Game::Move_StairsUp:
            m_game->goUpFloor(x, y);
            showOpeningFloorStory(floorBefore, m_game->currentFloor());
            ui.mapWidget->update();
            updateHUD();
            break;
        case Game::Move_StairsDown:
            m_game->goDownFloor(x, y);
            showOpeningFloorStory(floorBefore, m_game->currentFloor());
            ui.mapWidget->update();
            updateHUD();
            break;
        default:
            break;
    }
}

void MainWindow::completePendingTeleport()
{
    if (!m_pendingTeleport.active) return;
    const PendingTeleport pending = m_pendingTeleport;
    m_pendingTeleport = {};
    const auto result = m_game->completeTeleportPlayerTo();
    startMonsterMovementAnimation();
    handleTeleportResult(pending.x, pending.y, pending.floorBefore,
                         pending.pickedName, pending.pickedDescription, result);
    ui.mapWidget->update();
}

void MainWindow::flushPendingMove()
{
    if (!m_hasPendingMove || m_pendingTeleport.active || ui.mapWidget->isSceneAnimating()) return;
    const int dx = m_pendingMoveDx;
    const int dy = m_pendingMoveDy;
    m_hasPendingMove = false;
    const int key = dx < 0 ? Qt::Key_Left : dx > 0 ? Qt::Key_Right :
                    dy < 0 ? Qt::Key_Up : Qt::Key_Down;
    QKeyEvent queued(QEvent::KeyPress, key, Qt::NoModifier);
    keyPressEvent(&queued);
}

void MainWindow::startMonsterMovementAnimation()
{
    auto movements = m_game->takeFloor10AmbushMovementAnimations();
    const auto scripted = m_game->takeScriptedMonsterMovementAnimations();
    movements.insert(movements.end(), scripted.begin(), scripted.end());
    const auto knight = m_game->takeFloor32KnightMovementAnimations();
    movements.insert(movements.end(), knight.begin(), knight.end());
    if (!movements.empty())
        ui.mapWidget->playMonsterMovement(movements);
}

void MainWindow::showBattleFeedback(const QString& message)
{
    if (!m_battleFeedbackEnabled || !ui.battleLabel) return;
    ui.battleLabel->setText(message);
    ui.battleLabel->setVisible(true);
    m_battleFeedbackTimer.stop();
    m_battleFeedbackTimer.setSingleShot(true);
    m_battleFeedbackTimer.start(3500);
}

void MainWindow::captureUndoSnapshot()
{
    if (!m_game) return;
    auto snapshot = std::make_unique<QTemporaryFile>();
    snapshot->setAutoRemove(true);
    if (!snapshot->open()) return;
    const QString path = snapshot->fileName();
    snapshot->close();
    if (!m_game->saveToFile(path.toStdString())) return;
    m_undoHistory.emplace_back(std::move(snapshot));
    // 保留足够长的操作历史，支持连续撤销；最旧快照超出上限后自动淘汰。
    constexpr size_t kMaxUndoSteps = 128;
    if (m_undoHistory.size() > kMaxUndoSteps)
        m_undoHistory.erase(m_undoHistory.begin());
    ui.undoButton->setEnabled(!m_undoHistory.empty());
}

void MainWindow::clearUndoHistory()
{
    m_undoHistory.clear();
    ui.undoButton->setEnabled(false);
}

void MainWindow::undoLastAction()
{
    if (m_undoHistory.empty()) {
        QMessageBox::information(this, QString::fromUtf8("撤销"),
            QString::fromUtf8("当前没有可撤销的操作。"));
        return;
    }
    const QString path = m_undoHistory.back()->fileName();
    if (!m_game->loadFromFile(path.toStdString())) {
        QMessageBox::warning(this, QString::fromUtf8("撤销"),
            QString::fromUtf8("撤销存档读取失败。"));
        return;
    }
    m_undoHistory.pop_back();
    ui.undoButton->setEnabled(!m_undoHistory.empty());
    m_hasPendingMove = false;
    m_pendingTeleport = {};
    m_floor32KnightStoryFloor = -1;
    ui.mapWidget->snapPlayerToGame();
    ui.mapWidget->update();
    updateHUD();
}

void MainWindow::showSaveLoadDialog(bool initialSave)
{
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("魔塔存档"));
    dlg.setFixedSize(520, 460);
    applyRuntimeArtSkin(dlg);
    auto* layout = new QVBoxLayout(&dlg);
    layout->addWidget(new QLabel(QString::fromUtf8(
        "选择一个存档槽位。普通存档最多 10 个槽位，保存不会离开当前游戏。"), &dlg));
    auto* slotList = new QListWidget(&dlg);
    layout->addWidget(slotList, 1);

    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) dir = QDir::currentPath();
    const QString saveDir = QDir(dir).filePath(QStringLiteral("saves"));
    QDir().mkpath(saveDir);

    const auto slotPath = [&saveDir](int slot) {
        return QDir(saveDir).filePath(QStringLiteral("slot_%1.sav").arg(slot));
    };
    const auto refreshSlots = [&]() {
        slotList->clear();
        for (int i = 1; i <= 10; ++i) {
            const QString path = slotPath(i);
            const QFileInfo info(path);
            const QString state = info.exists()
                ? QString::fromUtf8("已有存档（%1 KB）").arg(info.size() / 1024)
                : QString::fromUtf8("空槽位");
            auto* row = new QListWidgetItem(QString::fromUtf8("存档 %1    %2").arg(i).arg(state), slotList);
            row->setData(Qt::UserRole, i);
        }
        int defaultRow = 0;
        if (!initialSave) {
            for (int i = 1; i <= 10; ++i) {
                if (QFileInfo::exists(slotPath(i))) {
                    defaultRow = i - 1;
                    break;
                }
            }
        }
        slotList->setCurrentRow(defaultRow);
    };
    refreshSlots();

    auto* status = new QLabel(&dlg);
    status->setStyleSheet(QStringLiteral("color:#9fd8ff;"));
    layout->addWidget(status);
    auto* buttons = new QHBoxLayout();
    auto* save = new QPushButton(QString::fromUtf8("保存到选中槽位"), &dlg);
    auto* load = new QPushButton(QString::fromUtf8("读取选中槽位"), &dlg);
    auto* close = new QPushButton(QString::fromUtf8("返回游戏"), &dlg);
    buttons->addWidget(save);
    buttons->addWidget(load);
    buttons->addWidget(close);
    layout->addLayout(buttons);

    QObject::connect(save, &QPushButton::clicked, &dlg, [&]() {
        auto* row = slotList->currentItem();
        if (!row) return;
        const int slot = row->data(Qt::UserRole).toInt();
        if (m_game->saveToFile(slotPath(slot).toStdString())) {
            status->setText(QString::fromUtf8("已保存到存档 %1").arg(slot));
            refreshSlots();
            slotList->setCurrentRow(slot - 1);
        } else {
            status->setText(QString::fromUtf8("保存失败，请重试。"));
        }
    });
    QObject::connect(load, &QPushButton::clicked, &dlg, [&]() {
        auto* row = slotList->currentItem();
        if (!row) return;
        const int slot = row->data(Qt::UserRole).toInt();
        const QString path = slotPath(slot);
        if (!QFileInfo::exists(path)) {
            status->setText(QString::fromUtf8("存档 %1 为空。").arg(slot));
            return;
        }
        if (!m_game->loadFromFile(path.toStdString())) {
            status->setText(QString::fromUtf8("读取失败，请重试。"));
            return;
        }
        clearUndoHistory();
        m_hasPendingMove = false;
        m_pendingTeleport = {};
        m_floor32KnightStoryFloor = -1;
        ui.mapWidget->snapPlayerToGame();
        ui.mapWidget->update();
        updateHUD();
        dlg.accept();
    });
    QObject::connect(close, &QPushButton::clicked, &dlg, &QDialog::reject);
    dlg.exec();
}

void MainWindow::quickSave()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) dir = QDir::currentPath();
    QDir().mkpath(dir);
    const QString path = QDir(dir).filePath(QStringLiteral("quicksave.sav"));
    const bool ok = m_game->saveToFile(path.toStdString());
    if (!ok)
        QMessageBox::warning(this, QString::fromUtf8("即时存档"), QString::fromUtf8("即时存档失败。"));
}

void MainWindow::quickLoad()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) dir = QDir::currentPath();
    const QString path = QDir(dir).filePath(QStringLiteral("quicksave.sav"));
    if (!QFileInfo::exists(path)) {
        QMessageBox::information(this, QString::fromUtf8("即时读档"),
            QString::fromUtf8("还没有即时存档。"));
        return;
    }
    if (!m_game->loadFromFile(path.toStdString())) {
        QMessageBox::warning(this, QString::fromUtf8("即时读档"),
            QString::fromUtf8("即时存档读取失败。"));
        return;
    }
    // 读档后丢弃当前撤销点和所有尚未完成的动画/剧情回调，避免旧状态回写新存档。
    clearUndoHistory();
    m_hasPendingMove = false;
    m_pendingTeleport = {};
    m_floor32KnightStoryFloor = -1;
    ui.mapWidget->snapPlayerToGame();
    ui.mapWidget->update();
    updateHUD();
    QMessageBox::information(this, QString::fromUtf8("即时读档"),
        QString::fromUtf8("即时存档已读取。"));
}

void MainWindow::showSettings()
{
    QSettings settings(QStringLiteral("MyGO-Mota"), QStringLiteral("MyGO-Mota"));
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("设置"));
    dlg.setFixedSize(420, 250);
    auto* layout = new QVBoxLayout(&dlg);
    auto* animation = new QCheckBox(QString::fromUtf8("启用连续移动动画"), &dlg);
    animation->setChecked(ui.mapWidget->movementAnimationEnabled());
    auto* battle = new QCheckBox(QString::fromUtf8("显示战斗结果提示"), &dlg);
    battle->setChecked(m_battleFeedbackEnabled);
    layout->addWidget(animation);
    layout->addWidget(battle);
    layout->addWidget(new QLabel(QString::fromUtf8(
        "方向键：移动\n"
        "F5 即时存档，F9 即时读档，Ctrl+Z 连续撤销。\n"
        "背包中的消耗品按原版规则使用；楼层传送器可重复使用。"), &dlg));
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    buttons->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8("应用"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("取消"));
    auto* exitButton = new QPushButton(QString::fromUtf8("退出游戏"), &dlg);
    exitButton->setStyleSheet(QStringLiteral("QPushButton { color: #ff9a9a; }"));
    layout->addStretch();
    layout->addWidget(buttons);
    layout->addWidget(exitButton);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, [&]() {
        ui.mapWidget->setMovementAnimationEnabled(animation->isChecked());
        m_battleFeedbackEnabled = battle->isChecked();
        settings.setValue(QStringLiteral("movementAnimation"), animation->isChecked());
        settings.setValue(QStringLiteral("battleFeedback"), battle->isChecked());
        dlg.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(exitButton, &QPushButton::clicked, &dlg, [&dlg]() {
        if (QMessageBox::question(&dlg, QString::fromUtf8("退出游戏"),
                QString::fromUtf8("确定要退出游戏吗？"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
            QApplication::quit();
        }
    });
    applyRuntimeArtSkin(dlg);
    dlg.exec();
}

void MainWindow::showStoryMessage(const QString& message)
{
    showVisualNovelDialogue({
        {QString::fromUtf8("旁白"), message,
         QStringLiteral(":/images/characters/portraits/marina.png"), QStringLiteral("#ffd66b")}
    });

    connect(ui.quickSaveButton, &QPushButton::clicked, this, &MainWindow::quickSave);
    connect(ui.undoButton, &QPushButton::clicked, this, &MainWindow::undoLastAction);
    connect(ui.settingsButton, &QPushButton::clicked, this, &MainWindow::showSettings);
}

void MainWindow::loadAssets()
{
    auto* mw = ui.mapWidget;

    // Galgame 使用高分辨率立绘，地图行走使用用户指定的最新 8×8 参考素材。
    mw->loadPlayerImage(":/images/characters/portraits/anon.png");
    mw->loadPlayerOutfitSpriteSheet("reference_walk", ":/images/characters/player_outfits/anon_reference_walk_8x8.png");
    // 爱音多套 8×8 精细行走图；其余套装可通过 setPlayerOutfit 切换。
    mw->loadPlayerOutfitSpriteSheet("school", ":/images/characters/player_outfits/anon_school_8x8.png");
    mw->loadPlayerOutfitSpriteSheet("casual", ":/images/characters/player_outfits/anon_casual_8x8.png");
    mw->loadPlayerOutfitSpriteSheet("mygo_stage", ":/images/characters/player_outfits/anon_mygo_stage_8x8.png");
    mw->loadPlayerOutfitSpriteSheet("mujica_stage", ":/images/characters/player_outfits/anon_mujica_stage_8x8.png");
    mw->loadPlayerOutfitSpriteSheet("summer", ":/images/characters/player_outfits/anon_summer_8x8.png");
    mw->setPlayerOutfit("reference_walk");
    mw->loadBackgroundImage(":/images/backgrounds/bangdream_gbp_cover.jpg");

    // 运行时地图图块全部来自已生成图集的裁切素材。
    mw->loadTileImage(Tile_Floor,       ":/images/runtime/tiles/floor.png");
    mw->loadTileImage(Tile_Wall,        ":/images/runtime/tiles/wall.png");
    mw->loadTileImage(Tile_DarkWall,    ":/images/runtime/tiles/dark_wall.png");
    mw->loadDarkWallRevealedImage(":/images/runtime/tiles/dark_wall_revealed.png");
    mw->loadTileImage(Tile_DoorRed,     ":/images/runtime/tiles/door_red.png");
    mw->loadTileImage(Tile_DoorBlue,    ":/images/runtime/tiles/door_blue.png");
    mw->loadTileImage(Tile_DoorGreen,   ":/images/runtime/tiles/door_yellow.png");
    mw->loadTileImage(Tile_DoorMagic,   ":/images/runtime/tiles/door_magic.png");
    mw->loadTileImage(Tile_DoorIron,    ":/images/runtime/tiles/door_iron.png");
    mw->loadTileImage(Tile_StairsUp,    ":/images/runtime/tiles/stairs_up.png");
    mw->loadTileImage(Tile_StairsDown,  ":/images/runtime/tiles/stairs_down.png");
    mw->loadTileImage(Tile_Lava,        ":/images/runtime/tiles/lava.png");
    mw->loadTileImage(Tile_StarRiver,   ":/images/runtime/tiles/star_river.png");
    // 普通 NPC 使用麻里奈，商店使用凛凛子（均为 60×60 RGBA 角色小人）。
    mw->loadTileImage(Tile_Shop,        ":/images/characters/npcs/ririko_icon.png");
    mw->loadTileImage(Tile_NPC,         ":/images/characters/npcs/marina_icon.png");
    mw->loadNPCImage("小偷",             ":/images/characters/npcs/michelle_icon.png");

    const std::vector<std::pair<const char*, const char*>> itemImages = {
        {"Red Key",       ":/images/runtime/items/key_red.png"},
        {"红钥匙",        ":/images/runtime/items/key_red.png"},
        {"Blue Key",      ":/images/runtime/items/key_blue.png"},
        {"蓝钥匙",        ":/images/runtime/items/key_blue.png"},
        {"Green Key",     ":/images/runtime/items/key_yellow.png"},
        {"Yellow Key",    ":/images/runtime/items/key_yellow.png"},
        {"黄钥匙",        ":/images/runtime/items/key_yellow.png"},
        {"万能钥匙",      ":/images/runtime/items/key_magic.png"},
        {"大黄门钥匙",    ":/images/runtime/items/key_magic.png"},
        {"Potion",        ":/images/runtime/items/potion.png"},
        {"生命药",        ":/images/runtime/items/potion.png"},
        {"小血瓶",        ":/images/runtime/items/potion_small.png"},
        {"大血瓶",        ":/images/runtime/items/potion_large.png"},
        {"Small Potion",  ":/images/runtime/items/potion_small.png"},
        {"Large Potion",  ":/images/runtime/items/potion_large.png"},
        {"红宝石",        ":/images/runtime/items/ruby_gem.png"},
        {"蓝宝石",        ":/images/runtime/items/sapphire_gem.png"},
        {"Ruby Gem",      ":/images/runtime/items/ruby_gem.png"},
        {"Sapphire Gem",  ":/images/runtime/items/sapphire_gem.png"},
        {"Weapon",        ":/images/runtime/items/weapon.png"},
        {"武器",          ":/images/runtime/items/weapon.png"},
        {"Armor",         ":/images/runtime/items/armor.png"},
        {"防具",          ":/images/runtime/items/armor.png"},
        {"Treasure",      ":/images/runtime/items/treasure.png"},
        {"金币",          ":/images/runtime/items/treasure.png"},
        {"匿名眼镜",      ":/images/runtime/items/glasses.png"},
        {"破墙锤",        ":/images/runtime/items/wall_breaker.png"},
        {"上楼器",        ":/images/runtime/items/stairs_up.png"},
        {"下楼器",        ":/images/runtime/items/stairs_down.png"},
        {"临时护盾",      ":/images/runtime/items/temp_shield.png"},
        {"企鹅玩偶",      ":/images/runtime/items/penguin_doll.png"},
        {"抹茶芭菲",      ":/images/runtime/items/matcha_parfait.png"},
        {"幸运金币",      ":/images/runtime/items/lucky_coin.png"},
        {"圣水",          ":/images/runtime/items/holy_water.png"},
        {"红色Live票",    ":/images/runtime/items/mygo/live_ticket_red.png"},
        {"蓝色Live票",    ":/images/runtime/items/mygo/live_ticket_blue.png"},
        {"黄色Live票",    ":/images/runtime/items/mygo/live_ticket_yellow.png"},
        {"后台万能通行证", ":/images/runtime/items/mygo/backstage_pass.png"},
        {"现场补给",      ":/images/runtime/items/mygo/mygo_support_badge_red.png"},
        {"灯的热牛奶",    ":/images/runtime/items/mygo/tomori_warm_milk.png"},
        {"爱音能量饮",    ":/images/runtime/items/mygo/anon_energy_drink.png"},
        {"MyGO应援红章",  ":/images/runtime/items/mygo/mygo_support_badge_red.png"},
        {"Mujica应援蓝章", ":/images/runtime/items/mygo/mujica_support_badge_blue.png"},
        {"爱音拨片",      ":/images/runtime/items/mygo/anon_guitar_pick.png"},
        {"立希鼓棒",      ":/images/runtime/items/mygo/taki_drumsticks.png"},
        {"乐奈猫爪",      ":/images/runtime/items/mygo/rana_cat_claw.png"},
        {"灯的麦克风",    ":/images/runtime/items/mygo/tomori_microphone.png"},
        {"睦的贝斯",      ":/images/runtime/items/mygo/mutsumi_bass.png"},
        {"素世谱架",      ":/images/runtime/items/mygo/soyo_music_stand.png"},
        {"海铃节拍器",    ":/images/runtime/items/mygo/umiri_metronome.png"},
        {"初华舞台耳返",  ":/images/runtime/items/mygo/uika_in_ear.png"},
        {"祥子黑色乐谱",  ":/images/runtime/items/mygo/sakiko_sheet_music.png"},
        {"Mujica终幕面具", ":/images/runtime/items/mygo/mujica_finale_mask.png"},
        {"爱音手机",      ":/images/runtime/items/mygo/anon_smartphone.png"},
        {"楼层传送器",    ":/images/runtime/items/mygo/anon_smartphone.png"},
        {"Mujica镜面舞台票", ":/images/runtime/items/mygo/mujica_mirror_ticket.png"},
        {"高松灯的单词本", ":/images/runtime/items/mygo/tomori_lyric_notebook.png"},
        {"怪物手册",      ":/images/runtime/items/glasses.png"},
        {"立希水壶",      ":/images/runtime/items/mygo/rikki_water_kettle.png"},
        {"爱音自拍眼镜",  ":/images/runtime/items/mygo/anon_selfie_glasses.png"},
        {"睦的镐子",      ":/images/runtime/items/mygo/mutsumi_pickaxe_toolbox.png"},
        {"Mujica烟雾弹",  ":/images/runtime/items/mygo/mujica_smoke_bomb.png"},
        {"Mujica舞台震响卷", ":/images/runtime/items/mygo/mujica_stage_quake_scroll.png"},
        {"MyGO和解徽章",  ":/images/runtime/items/mygo/mygo_reconciliation_badge.png"},
        {"MyGO团结徽章",  ":/images/runtime/items/mygo/mygo_reconciliation_badge.png"},
        {"祥子指挥棒",    ":/images/runtime/items/mygo/sakiko_conductor_baton.png"},
        {"海铃冷静指令",  ":/images/runtime/items/mygo/umiri_calm_command.png"},
        {"乐队护盾贴",    ":/images/runtime/items/mygo/band_shield_sticker.png"},
        {"立希企鹅挂件",  ":/images/runtime/items/mygo/rikki_penguin_keychain.png"},
        {"乐奈抹茶芭菲",  ":/images/runtime/items/mygo/rana_matcha_parfait.png"},
        {"乐奈幸运硬币",  ":/images/runtime/items/mygo/rana_lucky_coin.png"},
        {"舞台升降卡",    ":/images/runtime/items/mygo/stage_lift_card.png"},
        {"撤场通行卡",    ":/images/runtime/items/mygo/exit_pass.png"},
        {"ClassicArtifact", ":/images/runtime/items/artifact.png"}
    };
    for (const auto& [name, path] : itemImages)
        mw->loadItemImage(name, QString::fromUtf8(path));

    // 加载怪物图片：优先使用逐怪物 60x60 chibi 精灵，缺失时回退到立绘。
    auto monsters = MonsterDB::all();
    for (size_t i = 0; i < monsters.size(); ++i) {
        QString path = QString(":/images/characters/monsters/monster_%1.png")
                           .arg(i + 1, 2, 10, QChar('0'));
        if (QPixmap(path).isNull()) {
            path = monsterPortraitPath(monsters[i].GetName());
            if (path.isEmpty())
                path = QString(":/images/monster_%1.png").arg(i + 1, 2, 10, QChar('0'));
        }
        mw->loadMonsterImage(monsters[i].GetName(), path);
    }

    mw->update();
}

QString MainWindow::getItemDescription(const Item* item) const
{
    if (!item) return QString::fromUtf8("(空)");

    if (const auto* unknown = dynamic_cast<const UnknownItem*>(item)) {
        const QString source = QString::fromStdString(unknown->SourceName());
        return source.isEmpty()
            ? QString::fromUtf8("未知道具（名称无效）")
            : QString::fromUtf8("未知道具（原名：%1，暂无效果）").arg(source);
    }

    const std::string canonical = Game::canonicalItemName(item->GetName());
    if (canonical.empty())
        return QString::fromUtf8("未知道具（名称无效）");
    QString name = QString::fromStdString(canonical);
    int val = item->GetValue();

    if (name == "Potion" || name == QString::fromUtf8("现场补给"))
        return QString::fromUtf8("恢复 %1 点生命值").arg(val);
    if (name == QString::fromUtf8("灯的热牛奶"))
        return QString::fromUtf8("恢复 %1 点生命值").arg(val);
    if (name == QString::fromUtf8("爱音能量饮"))
        return QString::fromUtf8("恢复 %1 点生命值").arg(val);
    if (name == QString::fromUtf8("MyGO应援红章"))
        return QString::fromUtf8("攻击力 +%1（拾取即生效）").arg(val);
    if (name == QString::fromUtf8("Mujica应援蓝章"))
        return QString::fromUtf8("防御力 +%1（拾取即生效）").arg(val);
    if (name == "Ruby Gem") return QString::fromUtf8("攻击力 +%1（拾取即生效）").arg(val);
    if (name == "Sapphire Gem") return QString::fromUtf8("防御力 +%1（拾取即生效）").arg(val);
    if (name == "Weapon" || name == QString::fromUtf8("武器") ||
        name == QString::fromUtf8("爱音拨片") || name == QString::fromUtf8("立希鼓棒") ||
        name == QString::fromUtf8("乐奈猫爪") || name == QString::fromUtf8("灯的麦克风") ||
        name == QString::fromUtf8("睦的贝斯"))
        return QString::fromUtf8("攻击力 +%1").arg(val);
    if (name == "Armor" || name == QString::fromUtf8("防具") ||
        name == QString::fromUtf8("素世谱架") || name == QString::fromUtf8("海铃节拍器") ||
        name == QString::fromUtf8("初华舞台耳返"))
        return QString::fromUtf8("防御力 +%1").arg(val);
    if (name == QString::fromUtf8("祥子黑色乐谱") || name == QString::fromUtf8("Mujica终幕面具"))
        return QString::fromUtf8("防御力 +%1，并免疫魔法攻击").arg(val);
    if (name == "Treasure" || name == QString::fromUtf8("金币"))
        return QString::fromUtf8("获得 %1 金币").arg(val);
    if (name == "Red Key" || name == QString::fromUtf8("红色Live票"))
        return QString::fromUtf8("红色Live票 ×1");
    if (name == "Blue Key" || name == QString::fromUtf8("蓝色Live票"))
        return QString::fromUtf8("蓝色Live票 ×1");
    if (name == "Green Key" || name == "Yellow Key" || name == QString::fromUtf8("黄色Live票"))
        return QString::fromUtf8("黄色Live票 ×1");
    if (name == QString::fromUtf8("立希水壶"))
        return QString::fromUtf8("生命值增加当前攻击力与防御力之和");
    if (name == QString::fromUtf8("后台万能通行证") || name == QString::fromUtf8("大黄门钥匙"))
        return QString::fromUtf8("使用后可打开当前楼层全部黄门");
    if (name == QString::fromUtf8("爱音自拍眼镜"))
        return QString::fromUtf8("可以查看怪物属性");
    if (name == QString::fromUtf8("破墙锤") || name == QString::fromUtf8("睦的镐子"))
        return QString::fromUtf8("点击使用，下一次移动可摧毁墙壁");
    if (name == QString::fromUtf8("舞台升降卡"))
        return QString::fromUtf8("点击使用，从当前位置上楼");
    if (name == QString::fromUtf8("撤场通行卡"))
        return QString::fromUtf8("点击使用，从当前位置下楼");
    if (name == QString::fromUtf8("乐队护盾贴"))
        return QString::fromUtf8("点击使用，下次战斗防御 +50");
    if (name == QString::fromUtf8("立希企鹅挂件"))
        return QString::fromUtf8("面对高松灯和企鹅时伤害减半");
    if (name == QString::fromUtf8("乐奈抹茶芭菲"))
        return QString::fromUtf8("面对要乐奈和小猫时伤害减半");
    if (name == QString::fromUtf8("乐奈幸运硬币"))
        return QString::fromUtf8("打怪和拾取金币翻倍");
    if (name == QString::fromUtf8("睦的镐子"))
        return QString::fromUtf8("点击使用，下一次移动可摧毁墙壁");
    if (name == QString::fromUtf8("Mujica烟雾弹") || name == QString::fromUtf8("Mujica舞台震响卷"))
        return QString::fromUtf8("点击使用，摧毁墙壁");
    if (name == QString::fromUtf8("MyGO和解徽章") || name == QString::fromUtf8("MyGO团结徽章"))
        return QString::fromUtf8("对吸血鬼和兽人攻击翻倍（%1）")
            .arg(m_game->player().hasCross ? QString::fromUtf8("已生效") : QString::fromUtf8("未生效"));
    if (name == QString::fromUtf8("祥子指挥棒"))
        return QString::fromUtf8("对魔龙攻击翻倍（%1）")
            .arg(m_game->player().hasDragonSlayer ? QString::fromUtf8("已生效") : QString::fromUtf8("未生效"));
    if (name == QString::fromUtf8("海铃冷静指令") || name == QString::fromUtf8("冰冻徽章"))
        return QString::fromUtf8("点击使用，将当前楼层全部岩浆冻结为道路（可重复使用）");
    if (name == QString::fromUtf8("爱音手机"))
        return QString::fromUtf8("点击使用，传送到指定楼层");
    if (name == QString::fromUtf8("楼层传送器"))
        return QString::fromUtf8("点击使用，传送到指定楼层");
    if (name == QString::fromUtf8("Mujica镜面舞台票"))
        return QString::fromUtf8("点击使用，剩余 %1 次").arg(item->GetValue());
    if (name == QString::fromUtf8("高松灯的单词本"))
        return QString::fromUtf8("高松灯整理的单词与线索");
    if (name == QString::fromUtf8("怪物手册"))
        return QString::fromUtf8("查看地图上怪物的生命、攻击、防御与预计掉血");

    return name;
}

void MainWindow::activateItem(int index)
{
    if (index < 0 || index >= m_game->player().InventoryCount()) return;
    const Item* item = m_game->player().GetItem(index);
    if (!item) return;

    const QString itemName = QString::fromStdString(Game::canonicalItemName(item->GetName()));
    const QString itemDescription = getItemDescription(item);
    if (item->IsPassiveEffect()) {
        showBattleFeedback(itemName + QStringLiteral("\n") + itemDescription);
        return;
    }

    const bool isFlyingWand = dynamic_cast<const FlyingWand*>(item) != nullptr;
    const bool isFloorTeleporter = dynamic_cast<const FloorTeleporter*>(item) != nullptr;
    const bool isStairUpper = dynamic_cast<const StairUpper*>(item) != nullptr;
    const bool isStairLower = dynamic_cast<const StairLower*>(item) != nullptr;
    const bool isSymmetryFlyer = dynamic_cast<const SymmetryFlyer*>(item) != nullptr;
    const bool isBomb = dynamic_cast<const Bomb*>(item) != nullptr;
    const bool isEarthquake = dynamic_cast<const EarthquakeScroll*>(item) != nullptr;
    const bool isFreezeMagic = dynamic_cast<const FreezeMagic*>(item) != nullptr;
    const bool isMagicKey = dynamic_cast<const MagicKey*>(item) != nullptr;

    QString msg = QString::fromUtf8("使用了 %1").arg(itemName);
    if (isStairUpper) {
        if (!m_game->canTeleportByStairItem(true, m_game->player().x, m_game->player().y)) {
            showBattleFeedback(QString::fromUtf8("上楼器失败：目标楼层对应位置不是地板。"));
            return;
        }
        const int floorBefore = m_game->currentFloor();
        captureUndoSnapshot();
        m_game->player().UseItem(index);
        m_game->goUpFloor(m_game->player().x, m_game->player().y, false);
        m_game->player().stairUpUsed = false;
        showOpeningFloorStory(floorBefore, m_game->currentFloor());
    } else if (isStairLower) {
        if (m_game->currentFloor() <= 0) {
            showBattleFeedback(QString::fromUtf8("当前已经在最底层，无法继续下楼。"));
            return;
        }
        if (!m_game->canTeleportByStairItem(false, m_game->player().x, m_game->player().y)) {
            showBattleFeedback(QString::fromUtf8("下楼器失败：目标楼层对应位置不是地板。"));
            return;
        }
        const int floorBefore = m_game->currentFloor();
        captureUndoSnapshot();
        m_game->player().UseItem(index);
        m_game->goDownFloor(m_game->player().x, m_game->player().y, false);
        m_game->player().stairDownUsed = false;
        showOpeningFloorStory(floorBefore, m_game->currentFloor());
    } else if (isFlyingWand) {
        if (!m_game->canUsePhone()) {
            showBattleFeedback(QString::fromUtf8("爱音手机只能在与紫色或黄色楼梯连通的楼层使用。"));
            return;
        }
        bool ok = false;
        const int target = QInputDialog::getInt(this, QString::fromUtf8("爱音手机"),
            QString::fromUtf8("选择已到达的目标楼层（不可前往0、44、50层）："),
            m_game->currentFloor(), 1, 49, 1, &ok);
        if (!ok) return;
        if (target == 44 || target == 50 || !m_game->hasVisitedFloor(target)) {
            showBattleFeedback(QString::fromUtf8("手机只能传送到已经到过的楼层，且不能前往0、44、50层。"));
            return;
        }
        captureUndoSnapshot();
        if (!m_game->phoneTeleportToFloor(target)) {
            if (!m_undoHistory.empty()) m_undoHistory.pop_back();
            ui.undoButton->setEnabled(!m_undoHistory.empty());
            showBattleFeedback(QString::fromUtf8("当前楼梯未连通，或目标楼层没有可用楼梯。"));
            return;
        }
        m_game->player().UseItem(index);
    } else if (isFloorTeleporter) {
        bool ok = false;
        const int target = QInputDialog::getInt(this, QString::fromUtf8("楼层传送器"),
            QString::fromUtf8("选择目标楼层（0-50）："), m_game->currentFloor(), 0, 50, 1, &ok);
        if (!ok) return;
        captureUndoSnapshot();
        m_game->player().UseItem(index);
        while (m_game->currentFloor() < target) {
            const int floorBefore = m_game->currentFloor();
            m_game->goUpFloor(m_game->player().x, m_game->player().y, false);
            if (m_game->currentFloor() == 42 && m_game->floor42KnightStoryPending())
                showOpeningFloorStory(floorBefore, m_game->currentFloor());
        }
        while (m_game->currentFloor() > target)
            m_game->goDownFloor(m_game->player().x, m_game->player().y, false);
    } else if (isSymmetryFlyer) {
        const int mirroredX = m_game->width() - 1 - m_game->player().x;
        const int mirroredY = m_game->height() - 1 - m_game->player().y;
        const int targetTile = m_game->tileAt(mirroredX, mirroredY);
        if (targetTile == Tile_Floor) {
            captureUndoSnapshot();
            m_game->player().x = mirroredX;
            m_game->player().y = mirroredY;
            m_game->player().UseItem(index);
        } else {
            showBattleFeedback(QString::fromUtf8("对称位置不可到达。"));
            return;
        }
    } else if (isFreezeMagic) {
        captureUndoSnapshot();
        m_game->player().UseItem(index);
        msg += QString::fromUtf8("（冻结 %1 格岩浆）").arg(m_game->useFreezeMagic());
    } else if (isBomb || isEarthquake) {
        captureUndoSnapshot();
        const int affected = isBomb ? m_game->useBomb() : m_game->useEarthquakeScroll();
        m_game->player().UseItem(index);
        msg += QString::fromUtf8("（影响 %1 个图块/敌人）").arg(affected);
    } else if (isMagicKey) {
        captureUndoSnapshot();
        const int opened = m_game->useMagicKey();
        m_game->player().UseItem(index);
        msg += QString::fromUtf8("（开启 %1 扇黄门）").arg(opened);
    } else {
        captureUndoSnapshot();
        const QString description = itemDescription;
        m_game->player().UseItem(index);
        msg += QStringLiteral(": ") + description;
    }
    showBattleFeedback(msg);
    ui.mapWidget->update();
    updateHUD();
}

void MainWindow::showInventory(int focusIndex)
{
    auto& inv = m_game->player().Inventory();
    int count = m_game->player().InventoryCount();

    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("背包"));
    dlg.resize(400, 400);
    dlg.setStyleSheet("QDialog { background-color: #1a1a2e; color: #d0d0d0; }");

    auto* layout = new QVBoxLayout(&dlg);

    auto* label = new QLabel(QString::fromUtf8("背包 (共 %1 件物品)").arg(count), &dlg);
    label->setStyleSheet("color: #c8a23b; font-size: 16px; font-weight: bold; padding: 8px;");
    layout->addWidget(label);

    auto* list = new QListWidget(&dlg);
    list->setStyleSheet(
        "QListWidget { background: #0d0d1a; color: #d0d0d0; border: 1px solid #555; "
        "font-size: 14px; }"
        "QListWidget::item { padding: 6px; border-bottom: 1px solid #333; }"
        "QListWidget::item:selected { background: #3a3a6a; }"
        "QListWidget::item:hover { background: #2a2a4a; }"
    );

    if (count == 0) {
        auto* emptyItem = new QListWidgetItem(QString::fromUtf8("背包是空的"), list);
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsSelectable);
        emptyItem->setForeground(QColor("#666688"));
    } else {
        for (int i = 0; i < count; ++i) {
            auto* item = m_game->player().GetItem(i);
            if (item) {
                const std::string canonical = Game::canonicalItemName(item->GetName());
                const QString displayName = canonical.empty()
                    ? QString::fromUtf8("未知道具")
                    : QString::fromStdString(canonical);
                QString text = displayName + " — " + getItemDescription(item);
                auto* listItem = new QListWidgetItem(text, list);
                listItem->setData(Qt::UserRole, i);
                listItem->setToolTip(item->IsPassiveEffect()
                    ? QString::fromUtf8("被动效果，无需使用")
                    : QString::fromUtf8("双击使用"));
            }
        }
    }
    if (focusIndex >= 0 && focusIndex < count)
        list->setCurrentRow(focusIndex);
    layout->addWidget(list);

    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    btnBox->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8("使用"));
    btnBox->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("关闭"));
    btnBox->setStyleSheet(
        "QPushButton { background: #3a3a5a; color: #d0d0d0; border: 1px solid #66a; "
        "border-radius: 4px; padding: 6px 16px; font-size: 14px; }"
        "QPushButton:hover { background: #4a4a7a; }"
    );
    layout->addWidget(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, [&]() {
        auto* cur = list->currentItem();
        if (!cur || count == 0) return;
        int idx = cur->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < count) {
            auto* item = m_game->player().GetItem(idx);
            if (item) {
                if (item->IsPassiveEffect()) {
                    QMessageBox::information(&dlg, QString::fromUtf8("查看物品"),
                        QString::fromUtf8("%1\n%2").arg(
                            QString::fromStdString(item->GetName()),
                            getItemDescription(item)));
                } else {
                    const bool isFlyingWand = dynamic_cast<const FlyingWand*>(item) != nullptr;
                    const bool isFloorTeleporter = dynamic_cast<const FloorTeleporter*>(item) != nullptr;
                    const bool isStairUpper = dynamic_cast<const StairUpper*>(item) != nullptr;
                    const bool isStairLower = dynamic_cast<const StairLower*>(item) != nullptr;
                    const bool isSymmetryFlyer = dynamic_cast<const SymmetryFlyer*>(item) != nullptr;
                    const bool isBomb = dynamic_cast<const Bomb*>(item) != nullptr;
                    const bool isEarthquake = dynamic_cast<const EarthquakeScroll*>(item) != nullptr;
                    const bool isFreezeMagic = dynamic_cast<const FreezeMagic*>(item) != nullptr;
                    const bool isMagicKey = dynamic_cast<const MagicKey*>(item) != nullptr;
                    QString msg = QString::fromUtf8("使用了 %1: %2")
                        .arg(QString::fromStdString(item->GetName()))
                        .arg(getItemDescription(item));
                    if (isStairUpper) {
                        // 上楼器点击后立即执行传送，不留到下一次移动。
                        const int floorBefore = m_game->currentFloor();
                        captureUndoSnapshot();
                        m_game->player().UseItem(idx);
                        m_game->goUpFloor(m_game->player().x, m_game->player().y, false);
                        m_game->player().stairUpUsed = false;
                        showOpeningFloorStory(floorBefore, m_game->currentFloor());
                    } else if (isStairLower) {
                        if (m_game->currentFloor() <= 0) {
                            QMessageBox::information(&dlg, QString::fromUtf8("无法使用"),
                                QString::fromUtf8("当前已经在最底层，无法继续下楼。"));
                            return;
                        }
                        const int floorBefore = m_game->currentFloor();
                        captureUndoSnapshot();
                        m_game->player().UseItem(idx);
                        m_game->goDownFloor(m_game->player().x, m_game->player().y, false);
                        // 下楼器在使用界面中立即执行传送，不把一次性状态留给下一次按键。
                        m_game->player().stairDownUsed = false;
                        showOpeningFloorStory(floorBefore, m_game->currentFloor());
                    } else if (isFlyingWand) {
                        if (!m_game->canUsePhone()) {
                            QMessageBox::information(&dlg, QString::fromUtf8("手机无法使用"),
                                QString::fromUtf8("爱音手机只能在与紫色或黄色楼梯连通的楼层使用。"));
                            return;
                        }
                        bool ok = false;
                        const int target = QInputDialog::getInt(&dlg, QString::fromUtf8("爱音手机"),
                            QString::fromUtf8("选择已到达的目标楼层（不可前往0、44、50层）:"),
                            m_game->currentFloor(), 1, 49, 1, &ok);
                        if (!ok) return;
                        if (target == 44 || target == 50 || !m_game->hasVisitedFloor(target)) {
                            QMessageBox::information(&dlg, QString::fromUtf8("无法传送"),
                                QString::fromUtf8("手机只能传送到已经到过的楼层，且不能前往0、44、50层。"));
                            return;
                        }
                        captureUndoSnapshot();
                        if (!m_game->phoneTeleportToFloor(target)) {
                            if (!m_undoHistory.empty()) m_undoHistory.pop_back();
                            ui.undoButton->setEnabled(!m_undoHistory.empty());
                            QMessageBox::information(&dlg, QString::fromUtf8("无法传送"),
                                QString::fromUtf8("当前楼梯未连通，或目标楼层没有可用楼梯。"));
                            return;
                        }
                        m_game->player().UseItem(idx);
                    } else if (isFloorTeleporter) {
                        bool ok = false;
                        const int target = QInputDialog::getInt(&dlg,
                            QString::fromUtf8("楼层传送器"),
                            QString::fromUtf8("选择目标楼层（0-50）:"), m_game->currentFloor(), 0, 50, 1, &ok);
                        if (!ok) return;
                        captureUndoSnapshot();
                        m_game->player().UseItem(idx);
                        while (m_game->currentFloor() < target) {
                            const int floorBefore = m_game->currentFloor();
                            m_game->goUpFloor(m_game->player().x, m_game->player().y, false);
                            if (m_game->currentFloor() == 42 && m_game->floor42KnightStoryPending())
                                showOpeningFloorStory(floorBefore, m_game->currentFloor());
                        }
                        while (m_game->currentFloor() > target)
                            m_game->goDownFloor(m_game->player().x, m_game->player().y, false);
                    } else if (isSymmetryFlyer) {
                        const int mirroredX = m_game->width() - 1 - m_game->player().x;
                        const int mirroredY = m_game->height() - 1 - m_game->player().y;
                        const int targetTile = m_game->tileAt(mirroredX, mirroredY);
                        if (targetTile == Tile_Floor || targetTile == Tile_Item ||
                            targetTile == Tile_StairsUp || targetTile == Tile_StairsDown) {
                            captureUndoSnapshot();
                            m_game->player().x = mirroredX;
                            m_game->player().y = mirroredY;
                            m_game->player().UseItem(idx);
                        }
                    } else if (isFreezeMagic) {
                        captureUndoSnapshot();
                        m_game->player().UseItem(idx);
                        const int affected = m_game->useFreezeMagic();
                        msg += QString::fromUtf8("（冻结 %1 格岩浆）").arg(affected);
                    } else if (isBomb || isEarthquake) {
                        captureUndoSnapshot();
                        const int affected = isBomb ? m_game->useBomb() : m_game->useEarthquakeScroll();
                        m_game->player().UseItem(idx);
                        msg += QString::fromUtf8("（影响 %1 个图块/敌人）").arg(affected);
                    } else if (isMagicKey) {
                        captureUndoSnapshot();
                        const int opened = m_game->useMagicKey();
                        m_game->player().UseItem(idx);
                        msg += QString::fromUtf8("（开启 %1 扇黄门）").arg(opened);
                    } else {
                        captureUndoSnapshot();
                        m_game->player().UseItem(idx);
                    }
                    updateHUD();
                    QMessageBox::information(&dlg, QString::fromUtf8("使用物品"), msg);
                    dlg.accept();
                }
            }
        }
    });
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(list, &QListWidget::itemDoubleClicked, btnBox, &QDialogButtonBox::accepted);

    applyRuntimeArtSkin(dlg);
    dlg.exec();
}

void MainWindow::showPrisonTrapPrompt()
{
    if (m_game->storyShown("floor3_prison_prompt")) return;
    m_game->markStoryShown("floor3_prison_prompt");
    showStoryMessage(QString::fromUtf8(
        "五个身影已经显现：长崎素世与四名魔法警卫将你围住！\n"
        "点击地图画面继续，查看事件对话。"));
}

void MainWindow::showFloor3PrisonVisualNovel()
{
    if (m_game->storyShown("floor3_prison_dialogue")) return;
    const std::vector<VisualNovelPage> pages = {
        {QString::fromUtf8("千早爱音"),
         QString::fromUtf8("咦……这面墙后面，刚才明明什么都没有……"),
         QStringLiteral(":/images/characters/portraits/variants/anon_surprised.png"), QStringLiteral("#ff8fc7")},
        {QString::fromUtf8("长崎素世"),
         QString::fromUtf8("终于追上你了，爱音。你以为自己能平安通过这里吗？"),
         QStringLiteral(":/images/characters/portraits/variants/soyo_witch_angry.png"), QStringLiteral("#d9b6ff")},
        {QString::fromUtf8("千早爱音"),
         QString::fromUtf8("素世？！你为什么会在这里？我们不是约好要一起离开的吗？"),
         QStringLiteral(":/images/characters/portraits/variants/anon_more_shocked.png"), QStringLiteral("#ff8fc7")},
        {QString::fromUtf8("长崎素世"),
         QString::fromUtf8("别再往前了。把她围起来——这一次，我不会再让你逃走。"),
         QStringLiteral(":/images/characters/portraits/variants/soyo_witch_more_battle.png"), QStringLiteral("#d9b6ff")},
        {QString::fromUtf8("系统"),
         QString::fromUtf8("四名魔法警卫显现！夹击陷阱启动。\n点击“继续”承受冲击并返回二层。"),
         QStringLiteral(":/images/characters/portraits/marina.png"), QStringLiteral("#ffd66b")}
    };

    showVisualNovelDialogue(pages);
    m_game->markStoryShown("floor3_prison_dialogue");

    // 对话全部播放完后才结算伤害并传送回二层，保留原版剧情节奏。
    if (m_game->floor3PrisonStoryPending()) {
        m_game->resolveFloor3PrisonStory();
        showOpeningPrisonStory();
        ui.mapWidget->update();
        updateHUD();
    }
}

void MainWindow::showVisualNovelDialogue(const std::vector<VisualNovelPage>& pages)
{
    if (pages.empty()) return;
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("剧情"));
    dlg.setModal(true);
    dlg.setFixedSize(1240, 620);
    dlg.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setStyleSheet(
        "QDialog { background: rgba(14, 12, 28, 248); border: 2px solid #8c6ba8; }"
        "QLabel#vnName { font-size: 20px; font-weight: 800; padding: 3px 0; }"
        "QLabel#vnText { color: #fff4e6; font-size: 18px; }"
        "QLabel#vnPortrait { background: transparent; border: none; }"
        "QFrame#vnBox { background: rgba(18, 15, 35, 245); border: 2px solid #a57cc2; border-radius: 8px; }"
        "QPushButton { color: #fff7d0; background: #5f3d79; border: 1px solid #d5a8f0;"
        " border-radius: 6px; padding: 8px 24px; font-size: 15px; font-weight: 700; }"
        "QPushButton:hover { background: #79509a; }"
    );
    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(18, 12, 18, 14);
    root->setSpacing(6);
    auto* stage = new QHBoxLayout();
    stage->setContentsMargins(18, 0, 18, 0);
    auto* leftPortrait = new QLabel(&dlg);
    leftPortrait->setObjectName("vnPortrait");
    leftPortrait->setAttribute(Qt::WA_TranslucentBackground);
    leftPortrait->setAutoFillBackground(false);
    leftPortrait->setFixedSize(300, 400);
    leftPortrait->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    auto* rightPortrait = new QLabel(&dlg);
    rightPortrait->setObjectName("vnPortrait");
    rightPortrait->setAttribute(Qt::WA_TranslucentBackground);
    rightPortrait->setAutoFillBackground(false);
    rightPortrait->setFixedSize(300, 400);
    rightPortrait->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    stage->addWidget(leftPortrait, 0, Qt::AlignLeft | Qt::AlignBottom);
    stage->addStretch(1);
    stage->addWidget(rightPortrait, 0, Qt::AlignRight | Qt::AlignBottom);
    root->addLayout(stage, 1);

    auto* box = new QFrame(&dlg);
    box->setObjectName("vnBox");
    auto* boxLayout = new QVBoxLayout(box);
    boxLayout->setContentsMargins(18, 10, 14, 10);
    boxLayout->setSpacing(4);
    auto* textColumn = new QVBoxLayout();
    auto* name = new QLabel(&dlg);
    name->setObjectName("vnName");
    auto* text = new QLabel(&dlg);
    text->setObjectName("vnText");
    text->setWordWrap(true);
    text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    textColumn->addWidget(name);
    textColumn->addWidget(text, 1);
    textColumn->addStretch();
    boxLayout->addLayout(textColumn, 1);
    auto* next = new QPushButton(QString::fromUtf8("继续 ▶"), &dlg);
    next->setDefault(true);
    next->setFixedWidth(140);
    auto* buttonRow = new QHBoxLayout();
    auto* pageCounter = new QLabel(&dlg);
    pageCounter->setStyleSheet(QStringLiteral("color: #cdb4e8; font-size: 14px; font-weight: 700;"));
    buttonRow->addWidget(pageCounter);
    buttonRow->addStretch();
    buttonRow->addWidget(next);
    boxLayout->addLayout(buttonRow);
    root->addWidget(box, 0);
    int pageIndex = 0;
    const auto renderPage = [&]() {
        const VisualNovelPage& page = pages[static_cast<size_t>(pageIndex)];
        name->setText(page.speaker);
        name->setStyleSheet(QStringLiteral("color: %1;").arg(page.accent));
        text->setText(page.text);
        pageCounter->setText(QStringLiteral("%1 / %2").arg(pageIndex + 1).arg(pages.size()));
        const bool playerSpeaking = page.speaker.contains(QString::fromUtf8("爱音"));
        const QString leftPath = dialoguePortraitPath(playerSpeaking ? page.portrait
                                                : QStringLiteral(":/images/characters/portraits/anon.png"));
        const QString rightPath = dialoguePortraitPath(playerSpeaking
            ? QStringLiteral(":/images/characters/portraits/soyo.png") : page.portrait);
        const auto setPortrait = [&dlg](QLabel* target, const QString& path, bool active) {
            const QPixmap image(path);
            if (!image.isNull()) {
                target->setPixmap(image.scaled(target->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                // 当前说话者突出显示，另一侧降低亮度；每页切换时淡入，避免立绘突变。
                auto* effect = new QGraphicsOpacityEffect(target);
                target->setGraphicsEffect(effect);
                effect->setOpacity(0.0);
                auto* fade = new QPropertyAnimation(effect, "opacity", effect);
                fade->setDuration(180);
                fade->setStartValue(0.0);
                fade->setEndValue(active ? 1.0 : 0.42);
                fade->start(QAbstractAnimation::DeleteWhenStopped);
            } else {
                target->clear();
                target->setGraphicsEffect(nullptr);
            }
        };
        setPortrait(leftPortrait, leftPath, playerSpeaking);
        setPortrait(rightPortrait, rightPath, !playerSpeaking);
        next->setText(pageIndex + 1 == static_cast<int>(pages.size())
                          ? QString::fromUtf8("继续 ▶") : QString::fromUtf8("继续 ▶"));
    };
    connect(next, &QPushButton::clicked, &dlg, [&]() {
        if (pageIndex + 1 < static_cast<int>(pages.size())) {
            ++pageIndex;
            renderPage();
        } else {
            dlg.accept();
        }
    });
    renderPage();
    dlg.move((width() - dlg.width()) / 2, height() - dlg.height() - 24);
    dlg.exec();
}

bool MainWindow::showVisualNovelChoice(const QString& speaker, const QString& message,
                                       const QString& portraitPath, const QString& yesText,
                                       const QString& noText)
{
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("剧情选择"));
    dlg.setModal(true);
    dlg.setFixedSize(1240, 620);
    dlg.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setStyleSheet(
        "QDialog { background: rgba(14, 12, 28, 248); border: 2px solid #8c6ba8; }"
        "QLabel#vnName { font-size: 20px; font-weight: 800; color: #ffb5d7; }"
        "QLabel#vnText { color: #fff4e6; font-size: 18px; }"
        "QLabel#vnPortrait { background: transparent; border: none; }"
        "QFrame#vnBox { background: rgba(18, 15, 35, 245); border: 2px solid #a57cc2; border-radius: 8px; }"
        "QPushButton { color: #fff7d0; background: #5f3d79; border: 1px solid #d5a8f0;"
        " border-radius: 6px; padding: 8px 24px; font-size: 15px; font-weight: 700; }"
        "QPushButton:hover { background: #79509a; }"
    );
    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(18, 12, 18, 14);
    root->setSpacing(6);
    auto* stage = new QHBoxLayout();
    stage->setContentsMargins(18, 0, 18, 0);
    auto* leftPortrait = new QLabel(&dlg);
    leftPortrait->setObjectName("vnPortrait");
    leftPortrait->setAttribute(Qt::WA_TranslucentBackground);
    leftPortrait->setAutoFillBackground(false);
    leftPortrait->setFixedSize(300, 400);
    leftPortrait->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    auto* rightPortrait = new QLabel(&dlg);
    rightPortrait->setObjectName("vnPortrait");
    rightPortrait->setAttribute(Qt::WA_TranslucentBackground);
    rightPortrait->setAutoFillBackground(false);
    rightPortrait->setFixedSize(300, 400);
    rightPortrait->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    const bool playerSpeaking = speaker.contains(QString::fromUtf8("爱音"));
    const QString leftPath = dialoguePortraitPath(playerSpeaking ? portraitPath
                                            : QStringLiteral(":/images/characters/portraits/anon.png"));
    const QString rightPath = dialoguePortraitPath(playerSpeaking
        ? QStringLiteral(":/images/characters/portraits/soyo.png") : portraitPath);
    const auto setPortrait = [&dlg](QLabel* target, const QString& path, bool active) {
        const QPixmap image(path);
        if (!image.isNull()) {
            target->setPixmap(image.scaled(target->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            auto* effect = new QGraphicsOpacityEffect(target);
            target->setGraphicsEffect(effect);
            effect->setOpacity(active ? 1.0 : 0.42);
        } else {
            target->clear();
            target->setGraphicsEffect(nullptr);
        }
    };
    setPortrait(leftPortrait, leftPath, playerSpeaking);
    setPortrait(rightPortrait, rightPath, !playerSpeaking);
    stage->addWidget(leftPortrait, 0, Qt::AlignLeft | Qt::AlignBottom);
    stage->addStretch(1);
    stage->addWidget(rightPortrait, 0, Qt::AlignRight | Qt::AlignBottom);
    root->addLayout(stage, 1);

    auto* box = new QFrame(&dlg);
    box->setObjectName("vnBox");
    auto* boxLayout = new QVBoxLayout(box);
    boxLayout->setContentsMargins(18, 10, 14, 10);
    auto* column = new QVBoxLayout();
    auto* name = new QLabel(speaker, &dlg);
    name->setObjectName("vnName");
    auto* text = new QLabel(message, &dlg);
    text->setObjectName("vnText");
    text->setWordWrap(true);
    text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    column->addWidget(name);
    column->addWidget(text, 1);
    boxLayout->addLayout(column, 1);
    auto* buttons = new QHBoxLayout();
    buttons->addStretch();
    auto* no = new QPushButton(noText, &dlg);
    auto* yes = new QPushButton(yesText, &dlg);
    no->setAutoDefault(false);
    yes->setDefault(true);
    buttons->addWidget(no);
    buttons->addWidget(yes);
    boxLayout->addLayout(buttons);
    root->addWidget(box, 0);
    bool accepted = false;
    connect(yes, &QPushButton::clicked, &dlg, [&]() { accepted = true; dlg.accept(); });
    connect(no, &QPushButton::clicked, &dlg, &QDialog::reject);
    dlg.move((width() - dlg.width()) / 2, height() - dlg.height() - 24);
    dlg.exec();
    return accepted;
}

void MainWindow::showOpeningPrisonStory()
{
    if (m_prisonReturnStoryShown || m_game->storyShown("floor3_prison_return")) return;
    m_prisonReturnStoryShown = true;
    m_game->markStoryShown("floor3_prison_return");
    showVisualNovelDialogue({
        {QString::fromUtf8("旁白"),
         QString::fromUtf8("夹击发生了！你受到600点伤害，攻击和防御被压到10。"),
         QStringLiteral(":/images/characters/portraits/anon.png"), QStringLiteral("#ffd66b")},
        {QString::fromUtf8("千早爱音"),
         QString::fromUtf8("呜……这里是二层牢房？三层的怪物也都不见了……"),
         QStringLiteral(":/images/characters/portraits/variants/anon_sad.png"), QStringLiteral("#ff8fc7")},
        {QString::fromUtf8("米歇尔"),
         QString::fromUtf8("想恢复状态，就来和我对话两次。铁剑在5层，铁盾在9层。"),
         QStringLiteral(":/images/characters/portraits/variants/michelle_caring.png"), QStringLiteral("#ff9fbc")}
    });
}

void MainWindow::showFloor20VampireStoryIfNeeded(int floorBefore)
{
    if (floorBefore != 20 || m_game->currentFloor() != 20 ||
        m_game->tileAt(7, 10) != Tile_DoorMagic ||
        m_game->floor20VampireStoryShown())
        return;
    const Monster* vampire = m_game->monsterAt(7, 7);
    if (!vampire || vampire->GetName() != "凑友希那·吸血鬼") return;
    m_game->markFloor20VampireStoryShown();
    showVisualNovelDialogue({
        {QString::fromUtf8("旁白"),
         QString::fromUtf8("红门刚刚闭合，九只大蝙蝠在舞台中央聚成一个身影。"),
         QStringLiteral(":/images/characters/portraits/marina.png"), QStringLiteral("#ffd66b")},
        {QString::fromUtf8("千早爱音"),
         QString::fromUtf8("这不是普通的蝙蝠……是凑友希那的吸血鬼形态！"),
         QStringLiteral(":/images/characters/portraits/variants/anon_surprised.png"), QStringLiteral("#ff8fc7")},
        {QString::fromUtf8("旁白"),
         QString::fromUtf8("击败吸血鬼后，花门才会解除封锁。十字架可以让对吸血鬼的攻击翻倍。"),
         QStringLiteral(":/images/characters/portraits/variants/yukina_new_stage_surprised.png"), QStringLiteral("#b5c8ff")}
    });
}

void MainWindow::showFloor33TrapStoryIfNeeded(int floorBefore)
{
    if (floorBefore != 33 || m_game->currentFloor() != 33 ||
        !m_game->floor33TrapTriggered() ||
        m_game->player().x != 11 || m_game->player().y != 7 ||
        m_floor33TrapStoryShown || m_game->storyShown("floor33_trap_dialogue"))
        return;
    m_floor33TrapStoryShown = true;
    m_game->markStoryShown("floor33_trap_dialogue");
    showVisualNovelDialogue({
        {QString::fromUtf8("旁白"),
         QString::fromUtf8("你踏入了舞台中央，左右两侧的花门同时降下。"),
         QStringLiteral(":/images/characters/portraits/marina.png"), QStringLiteral("#ffd66b")},
        {QString::fromUtf8("千早爱音"),
         QString::fromUtf8("四个角落的怪物……必须全部击败，花门才会打开！"),
         QStringLiteral(":/images/characters/portraits/variants/anon_extra_worried.png"), QStringLiteral("#ff8fc7")}
    });
}

void MainWindow::showFloor32KnightStoryIfNeeded(int floorBefore)
{
    if (floorBefore != 32 || m_game->currentFloor() != 32 ||
        !m_game->floor32KnightStoryPending())
        return;
    if (!m_game->storyShown("floor32_knight_dialogue")) {
        m_game->markStoryShown("floor32_knight_dialogue");
        showVisualNovelDialogue({
            {QString::fromUtf8("旁白"),
             QString::fromUtf8("小长崎素世从右上角 (12,2) 的黄色楼梯出现，沿地板一步步走来，直接撞向爱音。"),
             QStringLiteral(":/images/characters/portraits/marina.png"), QStringLiteral("#ffd66b")},
            {QString::fromUtf8("骑士队长"),
             QString::fromUtf8("米歇尔已经被抓回二楼了！你别想再追上她！"),
             QStringLiteral(":/images/characters/portraits/variants/soyo_child_angry.png"), QStringLiteral("#b58cff")},
            {QString::fromUtf8("千早爱音"),
             QString::fromUtf8("来吧！就算你先攻，我也不会让开！"),
             QStringLiteral(":/images/characters/portraits/variants/anon_extra_determined.png"), QStringLiteral("#ff8fc7")}
        });
    }
    const int damage = m_game->resolveFloor32KnightStory();
    if (damage > 0)
        showBattleFeedback(QString::fromUtf8("骑士队长先攻，造成 %1 点伤害。").arg(damage));
    if (m_game->player().hp <= 0) {
        gameOver();
        return;
    }

    std::vector<std::string> log;
    const auto result = m_game->fightAt(7, 10, log);
    showBattleFeedback(QString::fromStdString(summarizeBattleLog(log)));
    if (result == Game::Fight_PlayerWin) {
        showVisualNovelDialogue({
            {QString::fromUtf8("骑士队长"),
             QString::fromUtf8("哼！这次算你赢了！"),
             QStringLiteral(":/images/characters/portraits/variants/soyo_child_sad.png"), QStringLiteral("#b58cff")},
            {QString::fromUtf8("旁白"),
             QString::fromUtf8("小素世转身沿来时的地板通道逃回右上黄色楼梯，楼梯始终没有消失。"),
             QStringLiteral(":/images/characters/portraits/marina.png"), QStringLiteral("#ffd66b")}
        });
        startMonsterMovementAnimation();
    } else if (result == Game::Fight_PlayerDead) {
        gameOver();
    }
    ui.mapWidget->update();
    updateHUD();
}

void MainWindow::showFloor32KnightStoryAfterMovement(int floorBefore)
{
    if (floorBefore != 32 || m_game->currentFloor() != 32 ||
        !m_game->floor32KnightStoryPending())
        return;
    if (ui.mapWidget->isMonsterMoving()) {
        m_floor32KnightStoryFloor = floorBefore;
        return;
    }
    showFloor32KnightStoryIfNeeded(floorBefore);
}

void MainWindow::showOpeningFloorStory(int fromFloor, int toFloor)
{
    (void)fromFloor;
    if (toFloor < 2 || toFloor > 50)
        return;

    // 42层首次抵达时，骑士队长逃跑后被魔王抓住，四名魔法警卫完成夹击；
    // 魔王离开后玩家仍留在42层。该剧情由 Game 状态保证只触发一次。
    if (toFloor == 42 && m_game->floor42KnightStoryPending() &&
        !m_game->storyShown("floor42_knight_capture")) {
        m_game->markStoryShown("floor42_knight_capture");
        m_game->markStoryShown("floor_opening_42");
        m_floorStoriesShown.insert(toFloor);
        showVisualNovelDialogue({
            {QString::fromUtf8("旁白"),
             QString::fromUtf8("骑士队长转身逃回黄色楼梯，王座后的阴影却突然压了下来。"),
             QStringLiteral(":/images/characters/portraits/marina.png"), QStringLiteral("#ffd66b")},
            {QString::fromUtf8("魔王"),
             QString::fromUtf8("抓住她。四名魔法警卫，从左右两侧夹击！"),
             QStringLiteral(":/images/characters/portraits/variants/soyo_witch_extra_commanding.png"), QStringLiteral("#b58cff")},
            {QString::fromUtf8("千早爱音"),
             QString::fromUtf8("骑士队长被抓住了……魔法警卫已经把我围住！"),
             QStringLiteral(":/images/characters/portraits/variants/anon_worried.png"), QStringLiteral("#ff8fc7")},
            {QString::fromUtf8("旁白"),
             QString::fromUtf8("魔王转身离开王座，夹击暂时解除。你仍站在第四十二层，前方的道路重新显现。"),
             QStringLiteral(":/images/characters/portraits/marina.png"), QStringLiteral("#ffd66b")}
        });
        m_game->resolveFloor42KnightStory();
        ui.mapWidget->update();
        updateHUD();
        return;
    }

    const std::string openingStoryKey = "floor_opening_" + std::to_string(toFloor);
    if (m_floorStoriesShown.count(toFloor) != 0 || m_game->storyShown(openingStoryKey))
        return;

    std::vector<VisualNovelPage> pages;
    if (toFloor == 2 && !m_floor2OpeningShown) {
        m_floor2OpeningShown = true;
        pages = {
            {QString::fromUtf8("千早爱音"),
             QString::fromUtf8("这里就是魔塔二层……听说被夺走的铁剑和铁盾分别藏在更高的楼层。"),
             QStringLiteral(":/images/characters/portraits/variants/anon_calm.png"), QStringLiteral("#ff8fc7")},
        {QString::fromUtf8("旁白"),
         QString::fromUtf8("先去找牢房里的米歇尔，她知道通往暗道的方法。"),
             QStringLiteral(":/images/characters/portraits/variants/michelle_wave.png"), QStringLiteral("#ffd66b")}
        };
    } else if (toFloor == 3 && !m_floor3OpeningShown) {
        m_floor3OpeningShown = true;
        pages = {
            {QString::fromUtf8("千早爱音"),
             QString::fromUtf8("三层的空气好沉重……前方似乎有守卫巡逻。"),
             QStringLiteral(":/images/characters/portraits/variants/anon_worried.png"), QStringLiteral("#ff8fc7")},
            {QString::fromUtf8("旁白"),
             QString::fromUtf8("小心前进，别被他们发现。"),
             QStringLiteral(":/images/characters/portraits/variants/soyo_school_serious.png"), QStringLiteral("#ffd66b")}
        };
    } else {
        QString location;
        QString warning;
        QString portrait = QStringLiteral(":/images/characters/portraits/marina.png");
        QString playerPortrait = QStringLiteral(":/images/characters/portraits/variants/anon_calm.png");
        switch (toFloor) {
        case 4: location = QStringLiteral("商店层的灯牌在黑暗里亮起。凛凛子似乎正在准备新的交易。"); warning = QStringLiteral("先确认钥匙和金币，再决定要不要购买强化。"); portrait = QStringLiteral(":/images/characters/portraits/ririko.png"); break;
        case 5: location = QStringLiteral("第五层传来熟悉的金属声——铁剑就在这附近。"); warning = QStringLiteral("拿回装备，才能继续追上素世的脚步。"); playerPortrait = QStringLiteral(":/images/characters/portraits/variants/anon_confident.png"); break;
        case 9: location = QStringLiteral("第九层的冷风穿过长廊，铁盾的气息就在前方。"); warning = QStringLiteral("机关门不会白白打开，留意周围的守卫。"); playerPortrait = QStringLiteral(":/images/characters/portraits/variants/anon_worried.png"); break;
        case 10: location = QStringLiteral("第十层的花门紧闭，舞台中央传来椎名立希的脚步声。"); warning = QStringLiteral("击败侧翼怪物，才能解开上下花门。"); portrait = QStringLiteral(":/images/characters/portraits/taki_stage.png"); break;
        case 20: location = QStringLiteral("第二十层的石壁刻着古老的乐谱，魔法守卫在暗处等待。"); warning = QStringLiteral("不要忽视每一扇机关门，它们都对应着守卫。"); playerPortrait = QStringLiteral(":/images/characters/portraits/variants/anon_worried.png"); break;
        case 24:
            location = QStringLiteral("第二十四层的红门像一道被封存的舞台幕墙，门后没有普通楼梯。");
            warning = m_game->princessDollPassageUnlocked()
                ? QStringLiteral("隐藏楼梯已经显现，穿过红门后登上正中的楼梯即可直达终幕。")
                : QStringLiteral("先去二十六层面对公主娃娃，回来后红门上方才会显现通往终幕的道路。");
            playerPortrait = QStringLiteral(":/images/characters/portraits/variants/anon_surprised.png");
            break;
        case 25: location = QStringLiteral("第二十五层的大厅回荡着大法师的低语。"); warning = QStringLiteral("这里开始，敌人的防御会明显提升。"); portrait = QStringLiteral(":/images/characters/portraits/sakiko_stage.png"); break;
        case 30: location = QStringLiteral("第三十层的牢门后传来求救声，像是有人被困在舞台后台。"); warning = QStringLiteral("找到开门的钥匙，再决定是否深入。"); playerPortrait = QStringLiteral(":/images/characters/portraits/variants/anon_worried.png"); break;
        case 40: location = QStringLiteral("第四十层通往异界的入口终于显现，星河在墙后缓慢流动。"); warning = QStringLiteral("秘宝是开启后续道路的关键。"); playerPortrait = QStringLiteral(":/images/characters/portraits/variants/anon_confident.png"); break;
        case 44: location = QStringLiteral("异界深处的第四十四层没有熟悉的方向感，只有不断变化的舞台。"); warning = QStringLiteral("这里不与普通楼梯相连，只能使用上楼器或下楼器进入和离开。"); playerPortrait = QStringLiteral(":/images/characters/portraits/variants/anon_surprised.png"); break;
        case 49: location = QStringLiteral("第四十九层的王座前，魔龙守卫挡住了最后的道路。"); warning = QStringLiteral("神圣剑、神圣盾或屠龙匕首，至少准备一样。"); playerPortrait = QStringLiteral(":/images/characters/portraits/variants/anon_extra_determined.png"); portrait = QStringLiteral(":/images/characters/portraits/soyo_stage.png"); break;
        case 50: location = QStringLiteral("第五十层的顶灯全部亮起，长崎素世正在王座尽头等你。"); warning = QStringLiteral("走完最后一段路，完成这场属于 MyGO!!!!! 与 Ave Mujica 的演出。"); portrait = QStringLiteral(":/images/characters/portraits/variants/soyo_witch_calm.png"); break;
        default:
            // 未配置专属剧情的楼层保持原版静默，不显示通用旁白。
            return;
        }
        pages = {
            {QString::fromUtf8("千早爱音"), location,
             playerPortrait, QStringLiteral("#ff8fc7")},
            {QString::fromUtf8("旁白"), warning, portrait, QStringLiteral("#ffd66b")}
        };
    }

    m_game->markStoryShown(openingStoryKey);
    m_floorStoriesShown.insert(toFloor);
    if (!pages.empty()) showVisualNovelDialogue(pages);
}

void MainWindow::showNPCDialog(int x, int y)
{
    NPC* npc = m_game->npcAt(x, y);
    if (!npc) return;

    Player& p = m_game->player();

    // NPC 对话使用与地图图块相同的角色头像，保持角色身份连续。
    QString npcPortrait;
    if (npc->IsTrader()) {
        npcPortrait = QStringLiteral(":/images/characters/portraits/ririko.png");
    } else if (npc->GetName() == "小偷" || npc->GetName() == "米歇尔") {
        // 米歇尔在牢笼、离场和终幕分别使用不同动作，保持剧情状态连续。
        if (m_game->currentFloor() == 2 && x == 12 && y == 12)
            npcPortrait = QStringLiteral(":/images/characters/portraits/variants/michelle_caring.png");
        else if (!npc->HasGivenReward())
            npcPortrait = QStringLiteral(":/images/characters/portraits/variants/michelle_wave.png");
        else
            npcPortrait = QStringLiteral(":/images/characters/portraits/variants/michelle_confident.png");
    } else {
        npcPortrait = QStringLiteral(":/images/characters/portraits/marina.png");
    }
    auto showNpcInfo = [&](const QString& title, const QString& text) {
        showVisualNovelDialogue({
            {title, text, npcPortrait, QStringLiteral("#ffb5d7")}
        });
    };

    // 原版关键 NPC 事件（保留一次性状态）。
    const int classicId = npc->ClassicId();
    if (!npc->HasGivenReward() && classicId == 3) {
        npc->Interact(p); // 领取三层专属道具“怪物手册”
        showNpcInfo(QString::fromUtf8("怪物手册"),
            QString::fromUtf8("这本怪物手册交给你。\n它能查看本层怪物的能力。"));
        updateHUD();
        return;
    }
    if (!npc->HasGivenReward() && classicId == 33) {
        p.atk = (p.atk * 103 + 99) / 100;
        p.def = (p.def * 103 + 99) / 100;
        npc->SetGiven(true);
        showNpcInfo(QString::fromUtf8("商人"), QString::fromUtf8("你的攻击力和防御力提升了 3%！"));
        updateHUD();
        return;
    }
    if (!npc->HasGivenReward() && classicId == 22 && m_game->currentFloor() == 26) {
        // 原版26层并不存在真正的公主：这里是洋娃娃。完成对话后，
        // 24层红门上方显现直通50层的隐藏楼梯，而不是直接传送。
        npc->SetGiven(true);
        m_game->unlockPrincessDollPassage();
        showVisualNovelDialogue({
            {QString::fromUtf8("公主娃娃"),
             QString::fromUtf8("谢谢你打开牢门……我终于自由了。"),
             npcPortrait, QStringLiteral("#ffb5d7")},
            {QString::fromUtf8("千早爱音"),
             QString::fromUtf8("咦？等等……这只是一个洋娃娃！"),
             QStringLiteral(":/images/characters/portraits/variants/anon_surprised.png"), QStringLiteral("#ff8fc7")},
            {QString::fromUtf8("旁白"),
             QString::fromUtf8("24层的墙体发生变化，红门后的隐藏通道已经开启。"),
             QStringLiteral(":/images/characters/portraits/marina.png"), QStringLiteral("#ffd66b")}
        });
        updateHUD();
        return;
    }
    if (!npc->HasGivenReward() && classicId == 10) {
        const bool accepted = showVisualNovelChoice(QString::fromUtf8("不正经的商人"),
            QString::fromUtf8("给我 1 金币，试试你的运气？（1% 获得 88 金币）"),
            npcPortrait, QString::fromUtf8("试试运气"));
        if (accepted && p.gold >= 1) {
            --p.gold;
            npc->SetGiven(true);
            if (QRandomGenerator::global()->bounded(100) == 0) p.gold += 88;
        } else if (accepted) {
            showNpcInfo(QString::fromUtf8("不正经的商人"), QString::fromUtf8("金币不够，等你准备好再来吧。"));
        }
        updateHUD();
        return;
    }
    if (!npc->HasGivenReward() && classicId == 24) {
        if (showVisualNovelChoice(QString::fromUtf8("神秘的商人"),
            QString::fromUtf8("我会随机提升一项属性，同时扣除另一项属性，确定交易吗？"),
            npcPortrait, QString::fromUtf8("接受交易"))) {
            int gain = QRandomGenerator::global()->bounded(3);
            int loss = QRandomGenerator::global()->bounded(3);
            if (gain == 0) p.hp += 100; else if (gain == 1) p.atk += 10; else p.def += 10;
            if (loss == 0) p.hp = std::max(1, p.hp - 100); else if (loss == 1) p.atk = std::max(0, p.atk - 10); else p.def = std::max(0, p.def - 10);
            npc->SetGiven(true);
        }
        updateHUD();
        return;
    }
    if (!npc->HasGivenReward() && classicId == 26 && m_game->currentFloor() == 35) {
        // 35层米歇尔剧情：对话完成后她离开魔龙层，前往50层终幕，
        // 同时打开魔龙房间暗道。所有文字统一使用 Galgame 对话界面。
        npc->SetGiven(true);
        showVisualNovelDialogue({
            {QString::fromUtf8("米歇尔"),
             QString::fromUtf8("我来帮你打开魔龙房间的暗道。等我离开后，去50层终幕找我。"),
             npcPortrait, QStringLiteral("#ffb5d7")},
            {QString::fromUtf8("千早爱音"),
             QString::fromUtf8("米歇尔，你要先去50层吗？终幕见！"),
             QStringLiteral(":/images/characters/portraits/variants/anon_confident.png"), QStringLiteral("#ff8fc7")}
        });
        m_game->completeFloor35MichelleStory();
        ui.mapWidget->update();
        updateHUD();
        return;
    }
    if (!npc->HasGivenReward() && classicId == 47 && m_game->currentFloor() == 50) {
        // 终幕揭开伪装：50层的米歇尔其实是长崎素世。
        npc->SetGiven(true);
        showVisualNovelDialogue({
            {QString::fromUtf8("米歇尔"),
             QString::fromUtf8("一路辛苦了，爱音。米歇尔只是我戴上的伪装。"),
             npcPortrait, QStringLiteral("#ffb5d7")},
            {QString::fromUtf8("长崎素世"),
             QString::fromUtf8("现在，欢迎来到真正的终幕——我是长崎素世。"),
             QStringLiteral(":/images/characters/portraits/variants/soyo_witch_calm.png"), QStringLiteral("#b58cff")},
            {QString::fromUtf8("千早爱音"),
             QString::fromUtf8("素世……原来你一直就在我身边！"),
             QStringLiteral(":/images/characters/portraits/variants/anon_more_shocked.png"), QStringLiteral("#ff8fc7")}
        });
        m_game->revealFloor50MichelleIdentity();
        ui.mapWidget->update();
        updateHUD();
        return;
    }
    if (classicId == 13) {
        if (npc->GetName() == "米歇尔" && !m_game->canReleaseMichelleFromCage()) {
            showVisualNovelDialogue({
                {QString::fromUtf8("米歇尔"),
                 QString::fromUtf8("牢门外还有中级守卫……先击败守卫，再来救我出去。"),
                 npcPortrait, QStringLiteral("#ffb5d7")},
                {QString::fromUtf8("千早爱音"),
                 QString::fromUtf8("我明白了。等我清理完守卫就回来。"),
                 QStringLiteral(":/images/characters/portraits/variants/anon_worried.png"), QStringLiteral("#ff8fc7")}
            });
            updateHUD();
            return;
        }
        // 29层剧情后米歇尔会在二层右下角 (12,12) 返场；这段对话只负责
        // 提示队伍前往35层开启暗道，不应复用开局(4,8)小偷的两次对话流程。
        if (npc->GetName() == "米歇尔" && m_game->currentFloor() == 2 &&
            (x == 12 && y == 12)) {
            npc->SetGiven(true);
            m_game->activateFloor35Michelle();
            showVisualNovelDialogue({
                {QString::fromUtf8("米歇尔"),
                 QString::fromUtf8("大家到35层集合吧，我会在那里打开魔龙房间的暗道。"),
                 npcPortrait, QStringLiteral("#ffb5d7")},
                {QString::fromUtf8("千早爱音"),
                 QString::fromUtf8("收到！我们现在就去35层，打开通往更深处的道路。"),
                 QStringLiteral(":/images/characters/portraits/variants/anon_confident.png"), QStringLiteral("#ff8fc7")}
            });
            ui.mapWidget->update();
            updateHUD();
            return;
        }
        // 原版2层小偷必须对话两次：第一次给出铁剑/铁盾楼层，
        // 第二次才解除三层陷阱造成的虚弱状态。
        if (!npc->HasGivenReward()) {
            // 对话完成后只打开小偷左侧的原版暗墙。
            m_game->setTile(3, 8, Tile_Floor);
            npc->SetGiven(true);
            // 只有15层剧情后回来的“米歇尔”才会在此处被救出并前往35层；
            // 开局二层的普通小偷对话不会提前触发魔龙剧情。
            if (npc->GetName() == "米歇尔")
                m_game->activateFloor35Michelle();
            showStoryMessage(QString::fromUtf8("铁剑在5层，铁盾在9层。先去把它们找回来。"));

            // 米歇尔完成第一次情报对话后走到二层下楼梯处，短暂停留后离场。
            // 楼梯底图保留，NPC 消失后玩家仍可正常使用楼梯。
            FloorData& floor2 = m_game->currentFloorData();
            const int sourceKey = m_game->posKey(x, y);
            int stairKey = -1;
            for (int sy = 0; sy < m_game->height() && stairKey < 0; ++sy) {
                for (int sx = 0; sx < m_game->width(); ++sx) {
                    if (floor2.map[sy * m_game->width() + sx] == Tile_StairsUp) {
                        stairKey = m_game->posKey(sx, sy);
                        break;
                    }
                }
            }
            auto movedIt = floor2.npcs.find(sourceKey);
            if (stairKey >= 0 && movedIt != floor2.npcs.end()) {
                const int originalStairTile = floor2.map[stairKey];
                NPC moved = std::move(movedIt->second);
                floor2.npcs.erase(movedIt);
                floor2.map[sourceKey] = floor2.items.count(sourceKey) ? Tile_Item : Tile_Floor;
                floor2.npcs.emplace(stairKey, std::move(moved));
                floor2.map[stairKey] = Tile_NPC;
                ui.mapWidget->update();
                QTimer::singleShot(900, this, [this, stairKey, originalStairTile]() {
                    if (m_game->currentFloor() != 2) return;
                    FloorData& current = m_game->currentFloorData();
                    auto it = current.npcs.find(stairKey);
                    if (it == current.npcs.end() || it->second.ClassicId() != 13) return;
                    current.npcs.erase(it);
                    current.map[stairKey] = originalStairTile;
                    ui.mapWidget->update();
                });
            }
        } else {
            if (m_game->floor3TrapActive())
                m_game->clearFloor3Trap();
            showStoryMessage(QString::fromUtf8("快去找剑和盾吧。现在可以继续前进了。"));
        }
        ui.mapWidget->update();
        updateHUD();
        return;
    }
    if (!npc->HasGivenReward() && (classicId == 14 || classicId == 25 || classicId == 31)) {
        // 小偷事件：打开身边的隐藏墙，等价于原版暗道剧情。
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                if (std::abs(dx) + std::abs(dy) == 1 && m_game->tileAt(x + dx, y + dy) == Tile_DarkWall)
                    m_game->setTile(x + dx, y + dy, Tile_Floor);
        npc->SetGiven(true);

        // 大章鱼所在的15层：米歇尔完成剧情后打开左侧暗墙并前往29层。
        if (classicId == 14 && m_game->currentFloor() == 15) {
            showVisualNovelDialogue({
                {QString::fromUtf8("米歇尔"),
                 QString::fromUtf8("大章鱼已经被击败了。我先去29层等你，之后再一起前往35层吧。"),
                 npcPortrait, QStringLiteral("#ffb5d7")},
                {QString::fromUtf8("千早爱音"),
                 QString::fromUtf8("一路小心，等我在35层和你会合！"),
                 QStringLiteral(":/images/characters/portraits/variants/anon_confident.png"), QStringLiteral("#ff8fc7")}
            });
            m_game->sendMichelleToFloor29();
            FloorData& floor = m_game->currentFloorData();
            const int npcKey = m_game->posKey(x, y);
            floor.npcs.erase(npcKey);
            floor.map[npcKey] = floor.items.count(npcKey) ? Tile_Item : Tile_Floor;
            ui.mapWidget->update();
            updateHUD();
            return;
        }

        // 29层的米歇尔是一次性剧情 NPC：完成对话后离场，并移除她正下方
        // 的墙（原版暗道入口）。该事件必须在对话窗口关闭后才清除 NPC，
        // 避免立绘在剧情尚未结束时突然消失。
        if (classicId == 25 && m_game->currentFloor() == 29) {
            if (m_game->tileAt(x, y + 1) == Tile_Wall ||
                m_game->tileAt(x, y + 1) == Tile_DarkWall) {
                m_game->setTile(x, y + 1, Tile_Floor);
            }

            std::vector<VisualNovelPage> pages;
            for (const auto& line : npc->Dialog()) {
                pages.push_back({QString::fromStdString(npc->GetName()),
                                 QString::fromStdString(line), npcPortrait,
                                 QStringLiteral("#ffb5d7")});
            }
            if (pages.empty()) {
                pages.push_back({QString::fromStdString(npc->GetName()),
                                 QString::fromUtf8("暗道已经打开了。"), npcPortrait,
                                 QStringLiteral("#ffb5d7")});
            }
            showVisualNovelDialogue(pages);

            // 29层剧情结束后，米歇尔回到二层右下角 (12,12)，等待玩家再次救出。
            m_game->returnMichelleToFloor2Cage();

            FloorData& floor = m_game->currentFloorData();
            const int npcKey = m_game->posKey(x, y);
            auto it = floor.npcs.find(npcKey);
            if (it != floor.npcs.end() && it->second.ClassicId() == classicId) {
                floor.npcs.erase(it);
                floor.map[npcKey] = floor.items.count(npcKey) ? Tile_Item : Tile_Floor;
            }
            ui.mapWidget->update();
            updateHUD();
            return;
        }
    }

    // 交易 NPC：每个 NPC 只允许完成一次交易，状态随存档保存。
    if (npc->IsTrader()) {
        if (npc->IsTradeDone()) {
            showNpcInfo(QString::fromStdString(npc->GetName()),
                QString::fromUtf8("这笔交易已经完成了。下次再见。"));
            updateHUD();
            return;
        }
        const Item* tradeReward = npc->GetTradeReward();
        QString rewardDesc;
        if (tradeReward)
            rewardDesc = getItemDescription(tradeReward);
        else
            rewardDesc = QString::fromUtf8("(无)");

        const QString info = QString::fromUtf8(
            "我这里有 %1。\n\n"
            "需要金币：%2\n"
            "你当前有：%3\n\n"
            "这位商人的交易只能完成一次。")
            .arg(rewardDesc)
            .arg(npc->GetTradeGoldCost())
            .arg(p.gold);

        bool canAfford = (npc->GetTradeGoldCost() <= p.gold);
        const bool accepted = showVisualNovelChoice(QString::fromStdString(npc->GetName()), info,
            npcPortrait, canAfford ? QString::fromUtf8("完成交易") : QString::fromUtf8("金币不足"));

        if (accepted && canAfford) {
            p.gold -= npc->GetTradeGoldCost();
            if (tradeReward) {
                // 创建可应用的物品副本
                auto item = Game::createItemByName(tradeReward->GetName(), tradeReward->GetValue());
                if (item) item->Apply(p);
            }
            npc->SetTradeDone(true);

            showNpcInfo(QString::fromStdString(npc->GetName()),
                QString::fromUtf8("交易成功！获得了 %1。").arg(rewardDesc));
        } else if (accepted) {
            showNpcInfo(QString::fromStdString(npc->GetName()),
                QString::fromUtf8("金币不够，交易暂时无法完成。"));
        }

        ui.mapWidget->update();
        updateHUD();
        return;
    }

    // 普通NPC（非交易或交易已完成）
    const std::string npcStoryKey = "npc_dialogue_" + std::to_string(m_game->currentFloor())
        + "_" + std::to_string(x) + "_" + std::to_string(y);
    if (m_game->storyShown(npcStoryKey)) return;
    bool hadReward = !npc->HasGivenReward();
    std::string reply = npc->Interact(p);

    if (hadReward && npc->HasGivenReward()) {
        showNpcInfo(QString::fromStdString(npc->GetName()), QString::fromStdString(reply));
        m_game->markStoryShown(npcStoryKey);
    } else {
        const auto& dialog = npc->Dialog();
        std::vector<VisualNovelPage> pages;
        for (size_t i = 0; i < dialog.size(); ++i) {
            pages.push_back({QString::fromStdString(npc->GetName()),
                             QString::fromStdString(dialog[i]), npcPortrait,
                             QStringLiteral("#ffb5d7")});
        }
        if (pages.empty())
            pages.push_back({QString::fromStdString(npc->GetName()),
                             QString::fromStdString(reply), npcPortrait,
                             QStringLiteral("#ffb5d7")});
        showVisualNovelDialogue(pages);
        m_game->markStoryShown(npcStoryKey);
    }
}

void MainWindow::showShopDialog(int x, int y)
{
    ShopData* shop = m_game->shopAt(x, y);
    if (!shop) return;

    Player& p = m_game->player();

    // 4/12/32/46 层是可重复购买的原版属性商店，必须优先于一次性兑换商人判断。
    if (shop->classicShopFloor > 0 && shop->classicShopFloor != 28) {
        const ClassicShopOffer offer = classicShopOfferForFloor(shop->classicShopFloor, p.shopUseCount);
        QDialog dlg(this);
        dlg.setWindowTitle(QString::fromUtf8("属性商店"));
        dlg.setFixedSize(460, 420);
        auto* layout = new QVBoxLayout(&dlg);
        layout->addWidget(new QLabel(QString::fromUtf8("原版商店：全局第 %1 次购买价格 %2 金币").arg(p.shopUseCount + 1).arg(offer.price), &dlg));
        struct Offer { QString name; QString effect; std::function<void()> apply; };
        const Offer offers[] = {
            {QString::fromUtf8("生命值"), QString::fromUtf8("+%1").arg(offer.hp), [&]{ p.hp += offer.hp; }},
            {QString::fromUtf8("攻击力"), QString::fromUtf8("+%1").arg(offer.atk), [&]{ p.atk += offer.atk; }},
            {QString::fromUtf8("防御力"), QString::fromUtf8("+%1").arg(offer.def), [&]{ p.def += offer.def; }}
        };
        bool purchased = false;
        for (const auto& item : offers) {
            // 价格直接显示在每个购买按钮上，避免玩家还要回看顶部说明。
            const QString buttonText = QString::fromUtf8("购买 %1（%2）\n当前价格：%3 金币")
                .arg(item.name, item.effect).arg(offer.price);
            auto* button = new QPushButton(buttonText, &dlg);
            button->setMinimumHeight(52);
            button->setEnabled(p.gold >= offer.price);
            QObject::connect(button, &QPushButton::clicked, &dlg, [this, &dlg, &p, &purchased, offer, item] {
                captureUndoSnapshot();
                p.gold -= offer.price;
                item.apply();
                ++p.shopUseCount;
                purchased = true;
                dlg.accept();
            });
            layout->addWidget(button);
        }
        auto* leave = new QPushButton(QString::fromUtf8("离开"), &dlg);
        QObject::connect(leave, &QPushButton::clicked, &dlg, &QDialog::reject);
        layout->addWidget(leave);
        applyRuntimeArtSkin(dlg);
        dlg.exec();
        // classicPurchaseCount 仅兼容旧存档，不参与属性商店是否可再次购买的判定。
        (void)purchased;
        updateHUD();
        return;
    }

    // 原版固定兑换商人：钥匙数量与价格保持 50 层魔塔配置。
    const int classicId = shop->classicNpcId;
    const QString shopPortrait = QStringLiteral(":/images/characters/portraits/ririko.png");
    if (classicId == 24) {
        // 28层商人是收购商：黄色钥匙可无限次出售，每把100金币。
        const int yellowKeys = p.KeyCount(KeyType::Green);
        if (yellowKeys <= 0) {
            showVisualNovelDialogue({
                {QString::fromUtf8("凛凛子"),
                 QString::fromUtf8("带黄色Live票来，我会按每把100金币无限收购。"),
                 shopPortrait, QStringLiteral("#ffb5d7")}
            });
            updateHUD();
            return;
        }
        const bool accepted = showVisualNovelChoice(QString::fromUtf8("凛凛子"),
            QString::fromUtf8("出售黄色Live票 ×1\n获得：100 金币\n当前黄色Live票：%1\n无上限")
                .arg(yellowKeys), shopPortrait, QString::fromUtf8("出售"));
        if (accepted) {
            captureUndoSnapshot();
            p.UseKey(KeyType::Green);
            p.gold += 100;
            showVisualNovelDialogue({
                {QString::fromUtf8("凛凛子"),
                 QString::fromUtf8("收购完成，获得100金币。下次还可以继续出售。"),
                 shopPortrait, QStringLiteral("#ffb5d7")}
            });
        }
        updateHUD();
        return;
    }
    if (shop->classicPurchaseCount > 0) {
        showVisualNovelDialogue({
            {QString::fromUtf8("凛凛子"),
             QString::fromUtf8("这个摊位的交易已经完成了，下次再来看看吧。"),
             shopPortrait, QStringLiteral("#ffb5d7")}
        });
        updateHUD();
        return;
    }
    showVisualNovelDialogue({
        {QString::fromUtf8("凛凛子"),
         QString::fromUtf8("欢迎来到现场补给站！按照魔塔规则，每个摊位只能交易一次。"),
         shopPortrait, QStringLiteral("#ffb5d7")}
    });
    struct FixedOffer { QString text; int cost; std::function<void()> grant; };
    FixedOffer offer;
    bool fixed = true;
    switch (classicId) {
    case 7:  offer = {QString::fromUtf8("蓝色Live票 ×1"), 50, [&] { p.AddKey(KeyType::Blue); }}; break;
    case 8:  offer = {QString::fromUtf8("黄色Live票 ×5"), 50, [&] { p.AddKey(KeyType::Green, 5); }}; break;
    case 9:  offer = {QString::fromUtf8("红色Live票 ×1"), 800, [&] { p.AddKey(KeyType::Red); }}; break;
    case 16: offer = {QString::fromUtf8("红色Live票 ×1"), 800, [&] { p.AddKey(KeyType::Red); }}; break;
    case 11: offer = {QString::fromUtf8("蓝色Live票 ×1"), 200, [&] { p.AddKey(KeyType::Blue); }}; break;
    case 27: offer = {QString::fromUtf8("黄色Live票 ×4、蓝色Live票 ×1"), 1000, [&] { p.AddKey(KeyType::Green, 4); p.AddKey(KeyType::Blue); }}; break;
    case 36: offer = {QString::fromUtf8("黄色Live票 ×3"), 200, [&] { p.AddKey(KeyType::Green, 3); }}; break;
    case 38: offer = {QString::fromUtf8("蓝色Live票 ×3"), 2000, [&] { p.AddKey(KeyType::Blue, 3); }}; break;
    case 41: offer = {QString::fromUtf8("生命值 +2000"), 1000, [&] { p.hp += 2000; }}; break;
    case 44: offer = {QString::fromUtf8("地震卷轴"), 4000, [&] { p.AddItem(std::make_unique<EarthquakeScroll>()); }}; break;
    default: fixed = false; break;
    }
    if (fixed) {
        const bool canAfford = p.gold >= offer.cost;
        const bool accepted = showVisualNovelChoice(QString::fromUtf8("凛凛子"),
            QString::fromUtf8("%1\n价格：%2 金币\n当前金币：%3\n每个摊位只能购买一次。")
                .arg(offer.text).arg(offer.cost).arg(p.gold),
            shopPortrait, canAfford ? QString::fromUtf8("购买") : QString::fromUtf8("金币不足"));
        if (accepted && canAfford) {
            captureUndoSnapshot();
            p.gold -= offer.cost;
            offer.grant();
            shop->classicPurchaseCount = 1;
            showVisualNovelDialogue({
                {QString::fromUtf8("凛凛子"),
                 QString::fromUtf8("交易完成！这是你的 %1。这个摊位不会再次出售。")
                     .arg(offer.text), shopPortrait, QStringLiteral("#ffb5d7")}
            });
        } else if (accepted) {
            showVisualNovelDialogue({
                {QString::fromUtf8("凛凛子"), QString::fromUtf8("金币不够，等你准备好再来吧。"),
                 shopPortrait, QStringLiteral("#ffb5d7")}
            });
        }
        updateHUD();
        return;
    }
    int surcharge = p.shopUseCount * 60;

    struct ShopItem {
        QString name;
        int basePrice;
        int actualPrice;
        QString effectDesc;
        std::function<void()> apply;
    };

    std::vector<ShopItem> items;
    items.push_back({QString::fromUtf8("生命药"), shop->potionPrice,
        shop->potionPrice > 0 ? shop->potionPrice + surcharge : 0,
        QString::fromUtf8("生命 +%1").arg(shop->potionValue),
        [&, hpVal = shop->potionValue]() { p.hp += hpVal; p.gold -= shop->potionPrice + p.shopUseCount * 60; p.shopUseCount++; }});
    items.push_back({QString::fromUtf8("武器"), shop->weaponPrice,
        shop->weaponPrice > 0 ? shop->weaponPrice + surcharge : 0,
        QString::fromUtf8("攻击 +%1").arg(shop->weaponValue),
        [&, atkVal = shop->weaponValue]() { p.atk += atkVal; p.gold -= shop->weaponPrice + p.shopUseCount * 60; p.shopUseCount++; }});
    items.push_back({QString::fromUtf8("防具"), shop->armorPrice,
        shop->armorPrice > 0 ? shop->armorPrice + surcharge : 0,
        QString::fromUtf8("防御 +%1").arg(shop->armorValue),
        [&, defVal = shop->armorValue]() { p.def += defVal; p.gold -= shop->armorPrice + p.shopUseCount * 60; p.shopUseCount++; }});

    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("商店"));
    dlg.setFixedSize(360, 360);
    dlg.setStyleSheet("QDialog { background-color: #1a1a2e; color: #d0d0d0; }");

    auto* layout = new QVBoxLayout(&dlg);
    layout->setSpacing(10);
    layout->setContentsMargins(16, 12, 16, 12);

    auto* infoLabel = new QLabel(
        QString::fromUtf8("💰 金币: %1  |  已购 %2 次  (+%3 G/次)")
            .arg(p.gold).arg(p.shopUseCount).arg(surcharge), &dlg);
    infoLabel->setStyleSheet("color: #c8a23b; font-size: 14px; font-weight: bold;");
    layout->addWidget(infoLabel);

    auto* sep = new QFrame(&dlg);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #444;");
    layout->addWidget(sep);

    bool purchased = false;
    QString purchasedName;
    QString purchasedEffect;
    for (auto& item : items) {
        auto* row = new QHBoxLayout();
        row->setSpacing(8);

        QString desc = item.basePrice > 0
            ? QString::fromUtf8("%1 (%2 G) — %3").arg(item.name).arg(item.actualPrice).arg(item.effectDesc)
            : QString::fromUtf8("%1 — 不售卖").arg(item.name);

        auto* label = new QLabel(desc, &dlg);
        label->setStyleSheet(item.basePrice > 0 ? "font-size: 13px;" : "color: #666; font-size: 13px;");
        row->addWidget(label, 1);

        auto* btn = new QPushButton(item.basePrice > 0
            ? QString::fromUtf8("购买（%1 G）").arg(item.actualPrice)
            : QString::fromUtf8("不可购买"), &dlg);
        btn->setFixedWidth(60);
        btn->setStyleSheet(
            "QPushButton { background: #3a5a3a; color: #d0d0d0; border: 1px solid #6a6; "
            "border-radius: 4px; padding: 4px 10px; font-size: 13px; }"
            "QPushButton:hover { background: #4a7a4a; }"
            "QPushButton:disabled { background: #333; color: #666; border-color: #444; }"
        );
        btn->setEnabled(item.basePrice > 0 && p.gold >= item.actualPrice);

        connect(btn, &QPushButton::clicked, &dlg, [this, &dlg, &item, &purchased, &purchasedName, &purchasedEffect]() {
            captureUndoSnapshot();
            item.apply();
            purchased = true;
            purchasedName = item.name;
            purchasedEffect = item.effectDesc;
            dlg.accept();
        });
        row->addWidget(btn);

        layout->addLayout(row);
    }

    layout->addStretch();

    auto* leaveBtn = new QPushButton(QString::fromUtf8("离开"), &dlg);
    leaveBtn->setFixedHeight(36);
    leaveBtn->setStyleSheet(
        "QPushButton { background: #3a3a5a; color: #d0d0d0; border: 1px solid #66a; "
        "border-radius: 4px; padding: 6px 16px; font-size: 14px; }"
        "QPushButton:hover { background: #4a4a7a; }"
    );
    connect(leaveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(leaveBtn);

    applyRuntimeArtSkin(dlg);
    dlg.exec();
    if (purchased) {
        shop->classicPurchaseCount = 1;
        showVisualNovelDialogue({
            {QString::fromUtf8("凛凛子"),
             QString::fromUtf8("购买了 %1！%2\n本摊位交易已完成。")
                 .arg(purchasedName).arg(purchasedEffect),
             shopPortrait, QStringLiteral("#ffb5d7")}
        });
    }
    updateHUD();
}

void MainWindow::showModifier()
{
    Player& p = m_game->player();

    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("修改器"));
    dlg.setFixedSize(420, 600);
    dlg.setStyleSheet("QDialog { background-color: #1a1a2e; color: #d0d0d0; }");

    auto* layout = new QVBoxLayout(&dlg);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 12, 12, 12);

    auto* tab = new QTabWidget(&dlg);
    tab->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #444; background: #1e1e32; }"
        "QTabBar::tab { background: #2a2a3e; color: #aaa; padding: 6px 16px; "
        "border: 1px solid #444; border-bottom: none; }"
        "QTabBar::tab:selected { background: #1e1e32; color: #fff; }"
    );

    // === Tab 1: 属性修改 ===
    auto* statTab = new QWidget();
    auto* statLayout = new QVBoxLayout(statTab);
    statLayout->setSpacing(6);
    statLayout->setContentsMargins(8, 8, 8, 8);

    struct StatRow {
        QString label;
        int* ptr;
        int min, max;
    };
    StatRow stats[] = {
        {QString::fromUtf8("生命 (HP)"), &p.hp, 1, 99999},
        {QString::fromUtf8("攻击 (ATK)"), &p.atk, 0, 99999},
        {QString::fromUtf8("防御 (DEF)"), &p.def, 0, 99999},
        {QString::fromUtf8("金币 (Gold)"), &p.gold, 0, 999999},
        {QString::fromUtf8("红钥匙"), (int*)&p, -1, 0},   // special
        {QString::fromUtf8("蓝钥匙"), (int*)&p, -2, 0},   // special
        {QString::fromUtf8("黄钥匙"), (int*)&p, -3, 0},   // special
    };

    // Key spinboxes need special handling since they use AddKey/HasKey
    auto* redKeySpin = new QSpinBox(statTab);
    redKeySpin->setRange(0, 999);
    redKeySpin->setValue(p.KeyCount(KeyType::Red));
    auto* blueKeySpin = new QSpinBox(statTab);
    blueKeySpin->setRange(0, 999);
    blueKeySpin->setValue(p.KeyCount(KeyType::Blue));
    auto* greenKeySpin = new QSpinBox(statTab);
    greenKeySpin->setRange(0, 999);
    greenKeySpin->setValue(p.KeyCount(KeyType::Green));

    auto makeStatRow = [&](const QString& label, QSpinBox* spin) {
        auto* row = new QHBoxLayout();
        auto* lbl = new QLabel(label, statTab);
        lbl->setFixedWidth(120);
        lbl->setStyleSheet("color: #aaa; font-size: 13px;");
        row->addWidget(lbl);
        spin->setStyleSheet(
            "QSpinBox { background: #222; color: #fff; border: 1px solid #555; "
            "padding: 4px; font-size: 13px; }");
        spin->setFixedWidth(140);
        row->addWidget(spin);
        row->addStretch();
        statLayout->addLayout(row);
        return spin;
    };

    auto* hpSpin = makeStatRow(QString::fromUtf8("生命 (HP)"), new QSpinBox(statTab));
    hpSpin->setRange(1, 99999);
    hpSpin->setValue(p.hp);
    auto* atkSpin = makeStatRow(QString::fromUtf8("攻击 (ATK)"), new QSpinBox(statTab));
    atkSpin->setRange(0, 99999);
    atkSpin->setValue(p.atk);
    auto* defSpin = makeStatRow(QString::fromUtf8("防御 (DEF)"), new QSpinBox(statTab));
    defSpin->setRange(0, 99999);
    defSpin->setValue(p.def);
    auto* goldSpin = makeStatRow(QString::fromUtf8("金币 (Gold)"), new QSpinBox(statTab));
    goldSpin->setRange(0, 999999);
    goldSpin->setValue(p.gold);

    statLayout->addSpacing(6);
    auto* keyLabel = new QLabel(QString::fromUtf8("钥匙数量:"), statTab);
    keyLabel->setStyleSheet("color: #aaccaa; font-size: 13px; font-weight: bold;");
    statLayout->addWidget(keyLabel);

    makeStatRow(QString::fromUtf8("红色Live票"), redKeySpin);
    makeStatRow(QString::fromUtf8("蓝色Live票"), blueKeySpin);
    makeStatRow(QString::fromUtf8("黄色Live票"), greenKeySpin);

    statLayout->addStretch();
    tab->addTab(statTab, QString::fromUtf8("属性"));

    // === Tab 2: 道具添加 ===
    auto* itemTab = new QWidget();
    auto* itemLayout = new QVBoxLayout(itemTab);
    itemLayout->setSpacing(4);
    itemLayout->setContentsMargins(8, 8, 8, 8);

    auto* itemHint = new QLabel(QString::fromUtf8("点击按钮直接添加到背包:"), itemTab);
    itemHint->setStyleSheet("color: #aaa; font-size: 12px; margin-bottom: 4px;");
    itemLayout->addWidget(itemHint);

    struct TestItem {
        QString name;
        int val;
        QString color;
    };
    TestItem testItems[] = {
        {QString::fromUtf8("灯的热牛奶"), 200, "#d44"},
        {QString::fromUtf8("爱音拨片"), 10, "#d82"},
        {QString::fromUtf8("素世谱架"), 5, "#48d"},
        {QString::fromUtf8("金币"), 100, "#da0"},
        {QString::fromUtf8("红色Live票"), 1, "#d33"},
        {QString::fromUtf8("蓝色Live票"), 1, "#33d"},
        {QString::fromUtf8("黄色Live票"), 1, "#db3"},
        {QString::fromUtf8("大黄门钥匙"), 0, "#84d"},
        {QString::fromUtf8("舞台升降卡"), 0, "#aa0"},
        {QString::fromUtf8("撤场通行卡"), 0, "#a6a"},
        {QString::fromUtf8("睦的镐子"), 0, "#864"},
        {QString::fromUtf8("乐队护盾贴"), 3, "#68d"},
        {QString::fromUtf8("爱音自拍眼镜"), 0, "#4aa"},
        {QString::fromUtf8("乐奈幸运硬币"), 0, "#da0"},
        {QString::fromUtf8("MyGO和解徽章"), 0, "#ff8"},
        {QString::fromUtf8("祥子指挥棒"), 0, "#f88"},
        {QString::fromUtf8("Mujica终幕面具"), 100, "#aff"},
    };

    auto* itemGrid = new QGridLayout();
    itemGrid->setSpacing(3);
    for (int i = 0; i < (int)(sizeof(testItems) / sizeof(testItems[0])); ++i) {
        auto* btn = new QPushButton(testItems[i].name, itemTab);
        btn->setFixedHeight(32);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QString(
            "QPushButton { background-color: %1; color: #fff; border: 1px solid #666; "
            "border-radius: 3px; font-size: 12px; font-weight: bold; }"
            "QPushButton:hover { border-color: #fff; }"
        ).arg(testItems[i].color));
        itemGrid->addWidget(btn, i / 4, i % 4);

        QString iname = testItems[i].name;
        int ival = testItems[i].val;
        connect(btn, &QPushButton::clicked, this, [this, iname, ival]() {
            auto item = Game::createItemByName(iname.toStdString(), ival);
            if (item) {
                m_game->player().AddItem(std::move(item));
            }
            updateHUD();
        });
    }
    itemLayout->addLayout(itemGrid);

    // 特殊道具效果（直接切换）
    itemLayout->addSpacing(6);
    auto* effectLabel = new QLabel(QString::fromUtf8("直接切换效果:"), itemTab);
    effectLabel->setStyleSheet("color: #aaa; font-size: 12px;");
    itemLayout->addWidget(effectLabel);

    auto addEffectBtn = [&](const QString& name, bool* flag) {
        auto updateText = [name, flag]() {
            return QString::fromUtf8("%1: %2").arg(name)
                .arg(*flag ? QString::fromUtf8("开") : QString::fromUtf8("关"));
        };
        auto* btn = new QPushButton(updateText(), itemTab);
        btn->setFixedHeight(30);
        btn->setStyleSheet(
            "QPushButton { background: #3a4a3a; color: #d0d0d0; border: 1px solid #5a5; "
            "border-radius: 3px; font-size: 12px; }"
            "QPushButton:hover { background: #4a6a4a; }"
        );
        connect(btn, &QPushButton::clicked, this, [this, flag, btn, updateText]() {
            *flag = !(*flag);
            btn->setText(updateText());
            updateHUD();
        });
        itemLayout->addWidget(btn);
        return btn;
    };
    addEffectBtn(QString::fromUtf8("匿名眼镜"), &p.hasGlasses);
    addEffectBtn(QString::fromUtf8("幸运金币"), &p.hasLuckyCoin);

    itemLayout->addStretch();
    tab->addTab(itemTab, QString::fromUtf8("道具"));

    // === Tab 3: 管理员调试 ===
    auto* debugTab = new QWidget();
    auto* debugLayout = new QVBoxLayout(debugTab);
    debugLayout->setSpacing(8);
    debugLayout->setContentsMargins(8, 8, 8, 8);

    auto* adminCheck = new QCheckBox(QString::fromUtf8("启用管理员模式"), debugTab);
    adminCheck->setChecked(m_adminMode);
    adminCheck->setStyleSheet("QCheckBox { color: #ffd66b; font-size: 14px; font-weight: bold; }");
    debugLayout->addWidget(adminCheck);

    auto* debugHint = new QLabel(QString::fromUtf8(
        "开启后可绕过门、钥匙和剧情触发，直接传送到任意楼层坐标。\n"
        "坐标范围：X/Y 0-14；楼层 0-50（含幸运金币层）。"), debugTab);
    debugHint->setWordWrap(true);
    debugHint->setStyleSheet("color: #aaa; font-size: 12px;");
    debugLayout->addWidget(debugHint);

    auto* debugForm = new QGridLayout();
    auto* floorSpin = new QSpinBox(debugTab);
    floorSpin->setRange(0, 50);
    floorSpin->setValue(m_game->currentFloor());
    auto* xSpin = new QSpinBox(debugTab);
    xSpin->setRange(0, m_game->width() - 1);
    xSpin->setValue(m_game->player().x);
    auto* ySpin = new QSpinBox(debugTab);
    ySpin->setRange(0, m_game->height() - 1);
    ySpin->setValue(m_game->player().y);
    for (QSpinBox* spin : {floorSpin, xSpin, ySpin}) {
        spin->setStyleSheet("QSpinBox { background: #222; color: #fff; border: 1px solid #555; padding: 4px; }");
        spin->setEnabled(m_adminMode);
    }
    debugForm->addWidget(new QLabel(QString::fromUtf8("楼层"), debugTab), 0, 0);
    debugForm->addWidget(floorSpin, 0, 1);
    debugForm->addWidget(new QLabel(QString::fromUtf8("X"), debugTab), 1, 0);
    debugForm->addWidget(xSpin, 1, 1);
    debugForm->addWidget(new QLabel(QString::fromUtf8("Y"), debugTab), 2, 0);
    debugForm->addWidget(ySpin, 2, 1);
    debugLayout->addLayout(debugForm);

    auto* debugTeleportBtn = new QPushButton(QString::fromUtf8("传送"), debugTab);
    debugTeleportBtn->setEnabled(m_adminMode);
    debugTeleportBtn->setStyleSheet(
        "QPushButton { background: #624c8a; color: #fff; border: 1px solid #b99bea; "
        "border-radius: 4px; padding: 7px 16px; font-weight: bold; }"
        "QPushButton:disabled { background: #333; color: #777; border-color: #555; }");
    debugLayout->addWidget(debugTeleportBtn);

    auto* debugStatus = new QLabel(QString::fromUtf8("管理员模式已关闭"), debugTab);
    debugStatus->setStyleSheet("color: #999; font-size: 12px;");
    debugLayout->addWidget(debugStatus);
    debugLayout->addStretch();
    tab->addTab(debugTab, QString::fromUtf8("调试"));

    auto setDebugEnabled = [this, adminCheck, floorSpin, xSpin, ySpin, debugTeleportBtn, debugStatus](bool enabled) {
        m_adminMode = enabled;
        floorSpin->setEnabled(enabled);
        xSpin->setEnabled(enabled);
        ySpin->setEnabled(enabled);
        debugTeleportBtn->setEnabled(enabled);
        debugStatus->setText(enabled ? QString::fromUtf8("管理员模式已开启")
                                     : QString::fromUtf8("管理员模式已关闭"));
        debugStatus->setStyleSheet(enabled ? "color: #ffd66b; font-size: 12px;"
                                           : "color: #999; font-size: 12px;");
    };
    connect(adminCheck, &QCheckBox::toggled, this, setDebugEnabled);
    connect(debugTeleportBtn, &QPushButton::clicked, this,
            [this, floorSpin, xSpin, ySpin, debugStatus]() {
        if (!m_adminMode) return;
        if (m_game->debugTeleport(floorSpin->value(), xSpin->value(), ySpin->value())) {
            debugStatus->setText(QString::fromUtf8("已传送到 %1 层 (%2,%3)")
                .arg(m_game->currentFloor()).arg(m_game->player().x).arg(m_game->player().y));
            ui.mapWidget->update();
            updateHUD();
        } else {
            debugStatus->setText(QString::fromUtf8("传送失败：坐标或楼层越界"));
        }
    });

    layout->addWidget(tab);

    // 底部按钮
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto* applyBtn = new QPushButton(QString::fromUtf8("应用"), &dlg);
    applyBtn->setFixedHeight(36);
    applyBtn->setStyleSheet(
        "QPushButton { background: #3a5a3a; color: #d0d0d0; border: 1px solid #5a5; "
        "border-radius: 4px; padding: 6px 20px; font-size: 14px; }"
        "QPushButton:hover { background: #4a7a4a; }"
    );
    btnRow->addWidget(applyBtn);
    layout->addLayout(btnRow);

    connect(applyBtn, &QPushButton::clicked, this, [&]() {
        p.hp = hpSpin->value();
        p.atk = atkSpin->value();
        p.def = defSpin->value();
        p.gold = goldSpin->value();
        // Sync keys: set the difference
        int diff;
        diff = redKeySpin->value() - p.KeyCount(KeyType::Red);
        if (diff > 0) p.AddKey(KeyType::Red, diff);
        else if (diff < 0) { while (diff++ < 0 && p.HasKey(KeyType::Red)) p.UseKey(KeyType::Red); }
        diff = blueKeySpin->value() - p.KeyCount(KeyType::Blue);
        if (diff > 0) p.AddKey(KeyType::Blue, diff);
        else if (diff < 0) { while (diff++ < 0 && p.HasKey(KeyType::Blue)) p.UseKey(KeyType::Blue); }
        diff = greenKeySpin->value() - p.KeyCount(KeyType::Green);
        if (diff > 0) p.AddKey(KeyType::Green, diff);
        else if (diff < 0) { while (diff++ < 0 && p.HasKey(KeyType::Green)) p.UseKey(KeyType::Green); }
        updateHUD();
        dlg.accept();
    });

    applyRuntimeArtSkin(dlg);
    dlg.exec();
    updateHUD();
}

void MainWindow::updateMonsterPanel()
{
    // 清除现有怪物条目（保留标题和分隔线）
    QLayoutItem* child;
    while ((child = ui.monsterLayout->takeAt(ui.monsterLayout->count() - 1)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
        // 只删到底部的 stretch 和分隔线之上的条目
        if (ui.monsterLayout->count() <= 2) break;
    }
    // 删除 stretch、分隔线、标题之外的所有
    while (ui.monsterLayout->count() > 3) {
        QLayoutItem* item = ui.monsterLayout->takeAt(2);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    // 添加 stretch（如果被删了）
    if (ui.monsterLayout->count() <= 3) {
        // 确保底部有 stretch
    }
    // 重新添加 stretch
    auto* existingStretch = ui.monsterLayout->itemAt(ui.monsterLayout->count() - 1);
    if (!existingStretch || existingStretch->spacerItem() == nullptr) {
        ui.monsterLayout->addStretch();
    }

    // 收集当前楼层所有怪物
    const auto& monsters = m_game->currentFloorData().monsters;
    if (monsters.empty()) {
        auto* emptyLabel = new QLabel(QString::fromUtf8("本层无怪物"), ui.monsterPanel);
        emptyLabel->setStyleSheet("color: #666; font-size: 13px; padding: 12px;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        // 插入到 stretch 之前
        ui.monsterLayout->insertWidget(ui.monsterLayout->count() - 1, emptyLabel);
        return;
    }

    // 按位置排序（从上到下，从左到右）
    int mapW = m_game->width();
    std::vector<std::pair<int, Monster>> sorted(monsters.begin(), monsters.end());
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;  // key = y*width + x
    });

    for (const auto& [key, mon] : sorted) {
        int x = key % mapW;
        int y = key / mapW;

        auto* row = new QWidget(ui.monsterPanel);
        row->setStyleSheet("background-color: #1e1e36; border-radius: 4px;");
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(4, 4, 4, 4);
        rowLayout->setSpacing(8);

        // 怪物小图
        auto* imgLabel = new QLabel(row);
        imgLabel->setFixedSize(52, 52);
        imgLabel->setAlignment(Qt::AlignCenter);
        imgLabel->setStyleSheet("border: 1px solid #665f78; border-radius: 4px; background: rgba(24,25,43,210);");

        QString imgPath = monsterPortraitPath(mon.GetName());
        if (imgPath.isEmpty())
            imgPath = QString(":/images/monster_%1.png")
                .arg(MonsterDB::indexOf(mon.GetName()) + 1, 2, 10, QChar('0'));
        QPixmap px(imgPath);
        if (!px.isNull()) {
            imgLabel->setPixmap(px.scaled(48, 48, Qt::KeepAspectRatio, Qt::FastTransformation));
        }
        rowLayout->addWidget(imgLabel);

        // 怪物信息
        QString info = QString::fromUtf8(
            "<b style='color:#f1cf7a;'>%1</b><br>"
            "<span style='color:#ff8b9d;'>生命 %2</span>  "
            "<span style='color:#ffb86b;'>攻击 %3</span><br>"
            "<span style='color:#9fc5ff;'>防御 %4</span>  "
            "<span style='color:#b8df9b;'>金币 %5</span><br>"
            "<span style='color:#8e91ab; font-size:10px;'>位置 (%6,%7)</span>")
            .arg(QString::fromStdString(mon.GetName()))
            .arg(formatNumber(mon.GetHP())).arg(formatNumber(mon.GetATK()))
            .arg(formatNumber(mon.GetDEF())).arg(formatNumber(mon.GetGold()))
            .arg(x).arg(y);

        auto* infoLabel = new QLabel(info, row);
        infoLabel->setStyleSheet("color: #d0d0d0; font-size: 12px;");
        infoLabel->setTextFormat(Qt::RichText);
        rowLayout->addWidget(infoLabel, 1);

        // 插入到 stretch 之前
        ui.monsterLayout->insertWidget(ui.monsterLayout->count() - 1, row);
    }
}

void MainWindow::updateItemPanel()
{
    if (!ui.itemLayout || !ui.itemPanel) return;
    while (QLayoutItem* item = ui.itemLayout->takeAt(0)) {
        if (QWidget* widget = item->widget()) delete widget;
        delete item;
    }

    struct ItemSlot {
        QString name;
        int firstIndex = -1;
        int count = 0;
        bool equipment = false;
    };
    std::vector<ItemSlot> itemSlots;
    const int count = m_game->player().InventoryCount();
    const auto isTowerSpecial = [](const QString& name) {
        static const char* const names[] = {
            "爱音手机", "大黄门钥匙", "怪物手册", "高松灯的单词本", "爱音自拍眼镜", "睦的镐子",
            "Mujica烟雾弹", "海铃冷静指令", "MyGO和解徽章", "祥子指挥棒",
            "立希水壶", "乐奈幸运硬币", "舞台升降卡", "撤场通行卡",
            "Mujica镜面舞台票"
        };
        for (const char* candidate : names)
            if (name == QString::fromUtf8(candidate)) return true;
        return false;
    };
    static const char* const visibleOrder[] = {
        // 固定前两排：武器、防具
        "爱音拨片", "立希鼓棒", "乐奈猫爪", "灯的麦克风", "睦的贝斯",
        "素世谱架", "海铃节拍器", "初华舞台耳返", "祥子黑色乐谱", "Mujica终幕面具",
        // 后续固定位置：塔内实际出现的特殊道具
        "爱音手机", "大黄门钥匙", "怪物手册", "高松灯的单词本", "爱音自拍眼镜", "睦的镐子",
        "Mujica烟雾弹", "海铃冷静指令", "MyGO和解徽章", "祥子指挥棒", "立希水壶",
        "乐奈幸运硬币", "舞台升降卡", "撤场通行卡", "Mujica镜面舞台票"
    };
    constexpr int visibleCount = static_cast<int>(sizeof(visibleOrder) / sizeof(visibleOrder[0]));
    for (int i = 0; i < count; ++i) {
        const Item* item = m_game->player().GetItem(i);
        if (!item) continue;
        const QString name = QString::fromStdString(Game::canonicalItemName(item->GetName()));
        const QString displayName = name.isEmpty() ? QString::fromUtf8("未知道具") : name;
        const bool equipment = dynamic_cast<const Weapon*>(item) != nullptr ||
                               dynamic_cast<const Armor*>(item) != nullptr;
        if (!equipment && !isTowerSpecial(displayName)) continue;
        int slotIndex = -1;
        for (int j = 0; j < static_cast<int>(itemSlots.size()); ++j) {
            if (itemSlots[j].name == displayName) {
                slotIndex = j;
                break;
            }
        }
        if (slotIndex < 0) {
            itemSlots.push_back({displayName, i, 1, equipment});
        } else {
            ++itemSlots[slotIndex].count;
        }
    }

    if (itemSlots.empty()) {
        auto* empty = new QLabel(QString::fromUtf8("暂无道具"), ui.itemPanel);
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet(QStringLiteral("color:#747b9d; font-size:11px;"));
        ui.itemLayout->addWidget(empty);
        return;
    }

    constexpr int columns = 5;
    for (int i = 0; i < visibleCount; ++i) {
        const QString fixedName = QString::fromUtf8(visibleOrder[i]);
        const auto slotIt = std::find_if(itemSlots.begin(), itemSlots.end(),
            [&fixedName](const ItemSlot& slot) { return slot.name == fixedName; });
        auto* emptySlot = new QWidget(ui.itemPanel);
        emptySlot->setFixedSize(48, 48);
        if (slotIt == itemSlots.end()) {
            ui.itemLayout->addWidget(emptySlot, i / columns, i % columns, Qt::AlignCenter);
            continue;
        }
        delete emptySlot;
        const ItemSlot& slot = *slotIt;
        const Item* item = m_game->player().GetItem(slot.firstIndex);
        if (!item) continue;
        auto* button = new QPushButton(ui.itemPanel);
        button->setFixedSize(48, 48);
        button->setIcon(QIcon(itemIconPath(item->GetName())));
        button->setIconSize(QSize(34, 34));
        button->setText(slot.count > 1 ? QString::fromUtf8("×%1").arg(slot.count) : QString());
        button->setToolTip(slot.name + QStringLiteral("\n") + getItemDescription(item) +
                           (slot.count > 1 ? QString::fromUtf8("\n持有 %1 件").arg(slot.count) : QString()) +
                           QStringLiteral("\n点击直接查看或使用"));
        button->setStyleSheet(
            QStringLiteral("QPushButton { background: transparent; border: 1px solid transparent; border-image: none; "
                           "border-radius: 4px; padding: 0; color: #ffd66b; font-size: 9px; font-weight: 700; }"
                           "QPushButton:hover { background: rgba(90,70,120,90); border-color: #d7a9ff; }"));
        ui.itemLayout->addWidget(button, i / columns, i % columns, Qt::AlignCenter);
        const int itemIndex = slot.firstIndex;
        connect(button, &QPushButton::clicked, this, [this, itemIndex]() {
            activateItem(itemIndex);
        });
    }
}

void MainWindow::updateHUD()
{
    int floor = m_game->currentFloor();
    ui.floorLabel->setText(QString::fromUtf8("第 %1 层").arg(floor));
    ui.hpLabel->setText(QString::fromUtf8("生命值    %1").arg(formatNumber(m_game->player().hp)));
    ui.atkLabel->setText(QString::fromUtf8("攻击力    %1").arg(formatNumber(m_game->player().atk)));
    ui.defLabel->setText(QString::fromUtf8("防御力    %1").arg(formatNumber(m_game->player().def)));
    ui.goldLabel->setText(QString::fromUtf8("金币      %1").arg(formatNumber(m_game->player().gold)));
    QString keyText = QString::fromUtf8("钥匙  红 %1   蓝 %2   黄 %3")
        .arg(formatNumber(m_game->player().KeyCount(KeyType::Red)))
        .arg(formatNumber(m_game->player().KeyCount(KeyType::Blue)))
        .arg(formatNumber(m_game->player().KeyCount(KeyType::Green)));
    if (m_game->player().magicKeyUses > 0)
        keyText += QString::fromUtf8("   万能 ×%1").arg(formatNumber(m_game->player().magicKeyUses));
    ui.keysLabel->setText(keyText);

    // 道具图标栏位于属性区域下方；旧文字栏位保留兼容但不再占用界面空间。
    ui.invItemsLabel->setVisible(false);
    updateItemPanel();

    updateMonsterPanel();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_Z) {
        undoLastAction();
        return;
    }
    if (event->key() == Qt::Key_F5) {
        quickSave();
        return;
    }
    if (event->key() == Qt::Key_F9) {
        quickLoad();
        return;
    }
    // 玩家已死亡则不响应
    if (m_game->player().hp <= 0) {
        QWidget::keyPressEvent(event);
        return;
    }

    int dx = 0, dy = 0;
    switch (event->key()) {
    case Qt::Key_Left:  dx = -1; break;
    case Qt::Key_Right: dx =  1; break;
    case Qt::Key_Up:    dy = -1; break;
    case Qt::Key_Down:  dy =  1; break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }

    // 游戏坐标已经按格更新；动画未结束前只缓存最后一次方向，动画结束后
    // 自动请求下一格，避免按住方向键时出现停顿或跨格插值。
    if (m_pendingTeleport.active) {
        return;
    }
    if (ui.mapWidget->isMonsterMoving()) {
        return;
    }
    if (ui.mapWidget->isPlayerMoving()) {
        m_pendingMoveDx = dx;
        m_pendingMoveDy = dy;
        m_hasPendingMove = true;
        return;
    }

    // 检查上楼器/下楼器（传送到当前坐标，不找楼梯）
    if (m_game->player().stairUpUsed) {
        captureUndoSnapshot();
        m_game->player().stairUpUsed = false;
        m_game->goUpFloor(m_game->player().x, m_game->player().y, false);
        ui.mapWidget->update();
        updateHUD();
        return;
    }
    if (m_game->player().stairDownUsed) {
        captureUndoSnapshot();
        m_game->player().stairDownUsed = false;
        m_game->goDownFloor(m_game->player().x, m_game->player().y, false);
        ui.mapWidget->update();
        updateHUD();
        return;
    }

    int nx = m_game->player().x + dx;
    int ny = m_game->player().y + dy;
    ui.mapWidget->setPlayerDirection(dx, dy);
    const int floorBefore = m_game->currentFloor();
    const Item* steppedItem = m_game->itemAt(nx, ny);
    const QString pickedName = steppedItem
        ? QString::fromStdString(Game::canonicalItemName(steppedItem->GetName()))
        : QString();
    const QString pickedDescription = steppedItem ? getItemDescription(steppedItem) : QString();
    // 在执行移动/战斗前保存一步撤销点，确保战斗、拾取和楼梯切换都能回退。
    captureUndoSnapshot();
    auto result = m_game->tryMovePlayer(nx, ny);
    startMonsterMovementAnimation();

    switch (result) {
    case Game::Move_Block:
        break;
    case Game::Move_DoorLocked: {
        if (floorBefore == 24 && nx == 7 && ny == 9 &&
            !m_game->princessDollPassageUnlocked()) {
            QMessageBox::information(this, QString::fromUtf8("通道未开启"),
                QString::fromUtf8("请先完成26层的公主娃娃剧情，24层隐藏通道才会出现。"));
            break;
        }
        int tile = m_game->tileAt(nx, ny);
        QString keyName;
        if (tile == Tile_DoorRed) keyName = QString::fromUtf8("红色Live票");
        else if (tile == Tile_DoorBlue) keyName = QString::fromUtf8("蓝色Live票");
        else if (tile == Tile_DoorGreen) keyName = QString::fromUtf8("黄色Live票");
        QMessageBox::information(this, QString::fromUtf8("门已锁"),
            QString::fromUtf8("需要 %1 才能打开这扇门。").arg(keyName));
        break;
    }
    case Game::Move_Ok:
        if (m_game->floor3PrisonStoryPending())
            showPrisonTrapPrompt();
        else if (floorBefore == 3 && m_game->currentFloor() == 2)
            showOpeningPrisonStory();
        showFloor20VampireStoryIfNeeded(floorBefore);
        showFloor33TrapStoryIfNeeded(floorBefore);
        showFloor32KnightStoryAfterMovement(floorBefore);
        ui.mapWidget->update();
        updateHUD();
        break;
    case Game::Move_Pickup:
        if (!pickedName.isEmpty())
            showBattleFeedback(QString::fromUtf8("获得 %1：%2").arg(pickedName).arg(
                QString(pickedDescription).replace(QString::fromUtf8("（未生效）"), QString::fromUtf8("（已生效）"))));
        ui.mapWidget->update();
        updateHUD();
        break;
    case Game::Move_Encounter: {
        Monster* m = m_game->monsterAt(nx, ny);
        if (!m) break;
        const bool knightFight = floorBefore == 32 &&
            m->GetName() == "幼年长崎素世·骑士队长";

        // 预判战斗结果
        int attackPower = m_game->player().atk;
        const bool vampireOrOrc = m->GetName().find("吸血") != std::string::npos ||
                                  m->GetName().find("兽人") != std::string::npos;
        const bool dragon = m->GetName().find("魔龙") != std::string::npos ||
                            m->GetName().find("龙") != std::string::npos;
        if (m_game->player().hasCross && vampireOrOrc) attackPower *= 2;
        if (m_game->player().hasDragonSlayer && dragon) attackPower *= 2;
        int dmgToMonster = std::max(0, attackPower - m->GetDEF());
        int shieldBonus = (m_game->player().tempShieldCharges > 0) ? 50 : 0;
        int dmgToPlayer = std::max(0, m->GetATK() - m_game->player().def - shieldBonus);
        const bool magicAttacker = m->GetName().find("法师") != std::string::npos ||
                                   m->GetName().find("巫师") != std::string::npos ||
                                   m->GetName().find("大法师") != std::string::npos ||
                                   m->GetName().find("魔法") != std::string::npos;
        if (m_game->player().hasHolyShield && magicAttacker) dmgToPlayer = 0;
        // 特殊道具减伤
        std::string mn = m->GetName();
        if (m_game->player().hasPenguinDoll &&
            (mn.find("高松灯") != std::string::npos || mn.find("企鹅") != std::string::npos))
            dmgToPlayer /= 2;
        if (m_game->player().hasMatchaParfait &&
            (mn.find("要乐奈") != std::string::npos || mn.find("小猫") != std::string::npos))
            dmgToPlayer /= 2;
        int roundsToKill = (dmgToMonster > 0) ? (m->GetHP() + dmgToMonster - 1) / dmgToMonster : -1;
        int totalDamage = (roundsToKill > 0 && dmgToPlayer > 0) ? (roundsToKill - 1) * dmgToPlayer : 0;
        bool canWin = (dmgToMonster > 0) && (totalDamage < m_game->player().hp);
        bool isStalemate = (dmgToMonster <= 0 && dmgToPlayer <= 0);

        QString battlePrefix;
        if (m_game->player().hasGlasses) {
            battlePrefix = QString::fromUtf8("%1  HP:%2 ATK:%3 DEF:%4  ")
                .arg(QString::fromStdString(m->GetName()))
                .arg(m->GetHP()).arg(m->GetATK()).arg(m->GetDEF());
        }
        if (isStalemate) {
            showBattleFeedback(battlePrefix + QString::fromUtf8("双方无法造成伤害，战斗停止。"));
            break;
        }
        if (!canWin) {
            battlePrefix += dmgToMonster <= 0
                ? QString::fromUtf8("⚠ 攻击力不足以穿透防御。")
                : QString::fromUtf8("⚠ 预计损失 %1 HP。 ").arg(totalDamage);
        }

        ui.mapWidget->update();
        std::vector<std::string> log;
        auto fightRes = m_game->fightAt(nx, ny, log);

        QString dlg = QString::fromStdString(summarizeBattleLog(log));
        if (!battlePrefix.isEmpty()) dlg = battlePrefix + "\n" + dlg;

        if (fightRes == Game::Fight_GameWin) {
            // 即使是最终战，也要先提交战斗产生的脚本移动队列。
            startMonsterMovementAnimation();
            ui.mapWidget->update();
            updateHUD();
            showBattleFeedback(dlg);
            gameWin();
            return;
        } else if (fightRes == Game::Fight_PlayerWin) {
            ui.mapWidget->update();
            updateHUD();
            showBattleFeedback(dlg);
            if (knightFight)
                showVisualNovelDialogue({
                     {QString::fromUtf8("骑士队长"), QString::fromUtf8("哼！这次算你赢了，我先逃回右上角的后台入口！"),
                     QStringLiteral(":/images/characters/portraits/variants/soyo_child_sad.png"), QStringLiteral("#b58cff")},
                    {QString::fromUtf8("千早爱音"), QString::fromUtf8("别想逃走……但楼梯已经打开了。"),
                     QStringLiteral(":/images/characters/portraits/variants/anon_extra_determined.png"), QStringLiteral("#ff8fc7")}
                });
            startMonsterMovementAnimation();
        } else if (fightRes == Game::Fight_Stalemate) {
            ui.mapWidget->update();
            updateHUD();
            showBattleFeedback(dlg);
        } else {
            showBattleFeedback(dlg);
            updateHUD();
            gameOver();
            return;
        }
        break;
    }
    case Game::Move_NPC: {
        showNPCDialog(nx, ny);
        updateHUD();
        break;
    }
    case Game::Move_Shop:
        showShopDialog(nx, ny);
        break;
        case Game::Move_StairsUp:
            m_game->goUpFloor(nx, ny);
            showOpeningFloorStory(floorBefore, m_game->currentFloor());
            ui.mapWidget->update();
            updateHUD();
            break;
        case Game::Move_StairsDown:
            m_game->goDownFloor(nx, ny);
            showOpeningFloorStory(floorBefore, m_game->currentFloor());
            ui.mapWidget->update();
            updateHUD();
            break;
    case Game::Move_PlayerDead:
        updateHUD();
        gameOver();
        break;
    }
}

void MainWindow::gameOver()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(QString::fromUtf8("游戏结束"));
    msgBox.setText(QString::fromUtf8("你被击败了！\n\n游戏结束。"));
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStyleSheet(
        "QMessageBox { background-color: #1a1a2e; color: #d0d0d0; }"
        "QLabel { color: #d0d0d0; font-size: 14px; }"
        "QPushButton { background: #3a3a5a; color: #d0d0d0; border: 1px solid #66a;"
        " border-radius: 4px; padding: 6px 16px; min-width: 80px; }"
        "QPushButton:hover { background: #4a4a7a; }"
    );

    QPushButton* restartBtn = msgBox.addButton(QString::fromUtf8("重新开始"), QMessageBox::ActionRole);
    QPushButton* menuBtn    = msgBox.addButton(QString::fromUtf8("返回主菜单"), QMessageBox::RejectRole);
    msgBox.setDefaultButton(restartBtn);

    applyRuntimeArtSkin(msgBox);
    msgBox.exec();

    if (msgBox.clickedButton() == restartBtn) {
        // 重新开始：创建新的 Game 并重新加载
        delete m_game;
        auto* newGame = new Game();
        newGame->loadDefaultMap();
        m_game = newGame;
        ui.mapWidget->setGame(m_game);
        ui.mapWidget->update();
        updateHUD();
        setFocus();
    } else {
        // 返回主菜单：关闭当前窗口，MenuWindow 会自动显示
        close();
    }
}

void MainWindow::gameWin()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(QString::fromUtf8("游戏通关"));
    msgBox.setText(QString::fromUtf8("恭喜！你击败了长崎素世！\n\n游戏通关！"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStyleSheet(
        "QMessageBox { background-color: #1a1a2e; color: #d0d0d0; }"
        "QLabel { color: #d0d0d0; font-size: 14px; }"
        "QPushButton { background: #3a3a5a; color: #d0d0d0; border: 1px solid #66a;"
        " border-radius: 4px; padding: 6px 16px; min-width: 80px; }"
        "QPushButton:hover { background: #4a4a7a; }"
    );

    QPushButton* restartBtn = msgBox.addButton(QString::fromUtf8("重新开始"), QMessageBox::ActionRole);
    QPushButton* menuBtn    = msgBox.addButton(QString::fromUtf8("返回主菜单"), QMessageBox::RejectRole);
    msgBox.setDefaultButton(menuBtn);

    applyRuntimeArtSkin(msgBox);
    msgBox.exec();

    if (msgBox.clickedButton() == restartBtn) {
        delete m_game;
        auto* newGame = new Game();
        newGame->loadDefaultMap();
        m_game = newGame;
        ui.mapWidget->setGame(m_game);
        ui.mapWidget->update();
        updateHUD();
        setFocus();
    } else {
        close();
    }
}
