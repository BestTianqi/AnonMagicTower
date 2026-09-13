#include "MainWindow.h"
#include "MapWidget.h"
#include "MapEditor.h"
#include "BattleFeedback.h"
#include "Entities/MonsterDB.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMessageBox>
#include <QString>
#include <QFileDialog>
#include <QDir>
#include <QDialog>
#include <QVBoxLayout>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFrame>
#include <QSpinBox>
#include <QInputDialog>
#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include <QTabWidget>
#include <QIcon>
#include <QRandomGenerator>
#include <QLocale>
#include <QMap>
#include <algorithm>
#include <iterator>

static QString formatNumber(int value)
{
    return QLocale(QLocale::Chinese, QLocale::China).toString(value);
}

static QString monsterPortraitPath(const std::string& name)
{
    // 与 MapWidget::loadAssets 中的原版怪物 ID 映射保持一致。
    // 怪物名称包含“原版形态”后缀，不能只靠角色名 contains 判断。
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
        "QPushButton { color: #fff7d0; border-image: url(:/images/runtime/ui/button_texture.png) 18 24 18 24 stretch stretch; padding: 8px 14px; font-size: 14px; font-weight: 700; }"
        "QPushButton:hover { color: white; }";
    for (QPushButton* button : {ui.invButton, ui.saveButton, ui.loadButton, ui.editorButton, ui.modButton})
        button->setStyleSheet(buttonArt);
    ui.invButton->setIcon(QIcon(":/images/runtime/items/artifact.png"));
    ui.saveButton->setIcon(QIcon(":/images/runtime/items/treasure.png"));
    ui.loadButton->setIcon(QIcon(":/images/runtime/items/stairs_down.png"));
    ui.editorButton->setIcon(QIcon(":/images/runtime/items/glasses.png"));
    ui.modButton->setIcon(QIcon(":/images/runtime/items/key_magic.png"));
    for (QPushButton* button : {ui.invButton, ui.saveButton, ui.loadButton, ui.editorButton, ui.modButton})
        button->setIconSize(QSize(30, 30));
    ui.invButton->setText(QString::fromUtf8("背包"));
    ui.saveButton->setText(QString::fromUtf8("保存"));
    ui.loadButton->setText(QString::fromUtf8("读取"));
    ui.editorButton->setText(QString::fromUtf8("地图编辑器"));
    ui.modButton->setText(QString::fromUtf8("修改器"));
    ui.monsterScroll->setStyleSheet(
        "QScrollArea { background-color: rgba(17,19,34,220); background-image: url(:/images/runtime/ui/panel_texture.png); border: 2px solid #777080; }"
        "QScrollBar:vertical { background: #171824; width: 9px; }"
        "QScrollBar::handle:vertical { background: #777080; min-height: 24px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");
    ui.monsterPanel->setStyleSheet(
        "background-color: rgba(17,19,34,220); background-image: url(:/images/runtime/ui/panel_texture.png);");

    ui.mapWidget->setGame(m_game);
    ui.mapWidget->setFocusPolicy(Qt::NoFocus);
    connect(ui.mapWidget, &MapWidget::tileClicked, this, [this](int x, int y) {
        if (m_game->player().hp <= 0 || ui.mapWidget->isPlayerMoving()) return;
        const int floorBefore = m_game->currentFloor();
        const auto result = m_game->teleportPlayerTo(x, y);
        switch (result) {
        case Game::Move_Pickup:
        case Game::Move_Ok:
            if (floorBefore == 3 && m_game->currentFloor() == 2)
                showOpeningPrisonStory();
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
            std::vector<std::string> log;
            const auto fightResult = m_game->fightAt(x, y, log);
            showBattleFeedback(QString::fromStdString(summarizeBattleLog(log)));
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
            ui.mapWidget->update();
            updateHUD();
            break;
        case Game::Move_StairsDown:
            m_game->goDownFloor(x, y);
            ui.mapWidget->update();
            updateHUD();
            break;
        default:
            break;
        }
    });
    updateHUD();
    m_movementQueueTimer.setInterval(16);
    connect(&m_movementQueueTimer, &QTimer::timeout, this, &MainWindow::flushPendingMove);
    // 动画结束的同一帧立即衔接下一格，定时器仅作为事件循环繁忙时的兜底。
    connect(ui.mapWidget, &MapWidget::playerMotionFinished,
            this, &MainWindow::flushPendingMove);
    m_movementQueueTimer.start();
    connect(&m_battleFeedbackTimer, &QTimer::timeout, this, [this]() {
        if (ui.battleLabel) ui.battleLabel->setVisible(false);
    });

    connect(ui.saveButton, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getSaveFileName(this, QString::fromUtf8("保存存档"),
            QDir::currentPath(), QString::fromUtf8("保存文件 (*.txt)"));
        if (!file.isEmpty()) {
            bool ok = m_game->saveToFile(file.toStdString());
            QMessageBox::information(this, QString::fromUtf8("保存"), ok ? QString::fromUtf8("保存成功") : QString::fromUtf8("保存失败"));
        }
    });

    connect(ui.loadButton, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, QString::fromUtf8("读取存档"),
            QDir::currentPath(), QString::fromUtf8("保存文件 (*.txt)"));
        if (!file.isEmpty()) {
            bool ok = m_game->loadFromFile(file.toStdString());
            if (ok) {
                ui.mapWidget->update();
                updateHUD();
                QMessageBox::information(this, QString::fromUtf8("读取"), QString::fromUtf8("读取成功"));
            } else {
                QMessageBox::warning(this, QString::fromUtf8("读取"), QString::fromUtf8("读取失败"));
            }
        }
    });

    connect(ui.editorButton, &QPushButton::clicked, this, [this]() {
        auto* editor = new MapEditor();
        editor->setAttribute(Qt::WA_DeleteOnClose);
        editor->show();
    });

    connect(ui.invButton, &QPushButton::clicked, this, &MainWindow::showInventory);

    connect(ui.modButton, &QPushButton::clicked, this, &MainWindow::showModifier);
}

void MainWindow::flushPendingMove()
{
    if (!m_hasPendingMove || ui.mapWidget->isPlayerMoving()) return;
    const int dx = m_pendingMoveDx;
    const int dy = m_pendingMoveDy;
    m_hasPendingMove = false;
    const int key = dx < 0 ? Qt::Key_Left : dx > 0 ? Qt::Key_Right :
                    dy < 0 ? Qt::Key_Up : Qt::Key_Down;
    QKeyEvent queued(QEvent::KeyPress, key, Qt::NoModifier);
    keyPressEvent(&queued);
}

void MainWindow::showBattleFeedback(const QString& message)
{
    if (!ui.battleLabel) return;
    ui.battleLabel->setText(message);
    ui.battleLabel->setVisible(true);
    m_battleFeedbackTimer.stop();
    m_battleFeedbackTimer.setSingleShot(true);
    m_battleFeedbackTimer.start(3500);
}

void MainWindow::loadAssets()
{
    auto* mw = ui.mapWidget;

    // 加载玩家图片
    mw->loadPlayerImage(":/images/characters/portraits/anon.png");
    mw->loadPlayerSpriteSheet(":/images/characters/pilot/anon_stage.png");
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
    mw->loadTileImage(Tile_Shop,        ":/images/characters/portraits/ririko.png");
    mw->loadTileImage(Tile_NPC,         ":/images/characters/portraits/marina.png");

    const std::vector<std::pair<const char*, const char*>> itemImages = {
        {"Red Key",       ":/images/runtime/items/key_red.png"},
        {"红钥匙",        ":/images/runtime/items/key_red.png"},
        {"Blue Key",      ":/images/runtime/items/key_blue.png"},
        {"蓝钥匙",        ":/images/runtime/items/key_blue.png"},
        {"Green Key",     ":/images/runtime/items/key_yellow.png"},
        {"Yellow Key",    ":/images/runtime/items/key_yellow.png"},
        {"黄钥匙",        ":/images/runtime/items/key_yellow.png"},
        {"万能钥匙",      ":/images/runtime/items/key_magic.png"},
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
        {"灯的歌词本",    ":/images/runtime/items/mygo/tomori_lyric_notebook.png"},
        {"立希水壶",      ":/images/runtime/items/mygo/rikki_water_kettle.png"},
        {"爱音自拍眼镜",  ":/images/runtime/items/mygo/anon_selfie_glasses.png"},
        {"睦的镐子",      ":/images/runtime/items/mygo/mutsumi_pickaxe_toolbox.png"},
        {"Mujica烟雾弹",  ":/images/runtime/items/mygo/mujica_smoke_bomb.png"},
        {"Mujica舞台震响卷", ":/images/runtime/items/mygo/mujica_stage_quake_scroll.png"},
        {"MyGO和解徽章",  ":/images/runtime/items/mygo/mygo_reconciliation_badge.png"},
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

    // 加载怪物图片：使用与边栏相同的本体/舞台头像映射。
    auto monsters = MonsterDB::all();
    for (size_t i = 0; i < monsters.size(); ++i) {
        QString path = monsterPortraitPath(monsters[i].GetName());
        if (path.isEmpty())
            path = QString(":/images/monster_%1.png").arg(i + 1, 2, 10, QChar('0'));
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
    if (name == QString::fromUtf8("后台万能通行证"))
        return QString::fromUtf8("可开任何门3次（优先使用普通钥匙）");
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
    if (name == QString::fromUtf8("MyGO和解徽章"))
        return QString::fromUtf8("对吸血鬼和兽人攻击翻倍");
    if (name == QString::fromUtf8("祥子指挥棒"))
        return QString::fromUtf8("对魔龙攻击翻倍");
    if (name == QString::fromUtf8("海铃冷静指令"))
        return QString::fromUtf8("点击使用，冻结下一格岩浆");
    if (name == QString::fromUtf8("爱音手机"))
        return QString::fromUtf8("点击使用，传送到指定楼层");
    if (name == QString::fromUtf8("楼层传送器"))
        return QString::fromUtf8("点击使用，传送到指定楼层");
    if (name == QString::fromUtf8("Mujica镜面舞台票"))
        return QString::fromUtf8("点击使用，剩余 %1 次").arg(item->GetValue());
    if (name == QString::fromUtf8("灯的歌词本"))
        return QString::fromUtf8("记录魔塔提示");

    return name;
}

void MainWindow::showInventory()
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
                    const bool isSymmetryFlyer = dynamic_cast<const SymmetryFlyer*>(item) != nullptr;
                    const bool isBomb = dynamic_cast<const Bomb*>(item) != nullptr;
                    const bool isEarthquake = dynamic_cast<const EarthquakeScroll*>(item) != nullptr;
                    const bool isFreezeMagic = dynamic_cast<const FreezeMagic*>(item) != nullptr;
                    QString msg = QString::fromUtf8("使用了 %1: %2")
                        .arg(QString::fromStdString(item->GetName()))
                        .arg(getItemDescription(item));
                    if (isFlyingWand || isFloorTeleporter) {
                        bool ok = false;
                        const int target = QInputDialog::getInt(&dlg,
                            item->GetName() == "楼层传送器" ? QString::fromUtf8("楼层传送器") : QString::fromUtf8("飞行魔杖"),
                            QString::fromUtf8("选择目标楼层（1-50）:"), m_game->currentFloor(), 1, 50, 1, &ok);
                        if (!ok) return;
                        m_game->player().UseItem(idx);
                        while (m_game->currentFloor() < target)
                            m_game->goUpFloor(m_game->player().x, m_game->player().y, false);
                        while (m_game->currentFloor() > target)
                            m_game->goDownFloor(m_game->player().x, m_game->player().y, false);
                    } else if (isSymmetryFlyer) {
                        const int mirroredX = m_game->width() - 1 - m_game->player().x;
                        const int mirroredY = m_game->player().y;
                        const int targetTile = m_game->tileAt(mirroredX, mirroredY);
                        if (targetTile == Tile_Floor || targetTile == Tile_Item ||
                            targetTile == Tile_StairsUp || targetTile == Tile_StairsDown) {
                            m_game->player().x = mirroredX;
                            m_game->player().y = mirroredY;
                            m_game->player().UseItem(idx);
                        }
                    } else if (isFreezeMagic) {
                        m_game->player().UseItem(idx);
                        const int affected = m_game->useFreezeMagic();
                        msg += QString::fromUtf8("（冻结 %1 格岩浆）").arg(affected);
                    } else if (isBomb || isEarthquake) {
                        const int affected = isBomb ? m_game->useBomb() : m_game->useEarthquakeScroll();
                        m_game->player().UseItem(idx);
                        msg += QString::fromUtf8("（影响 %1 个图块/敌人）").arg(affected);
                    } else {
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

void MainWindow::showOpeningPrisonStory()
{
    QMessageBox box(this);
    box.setWindowTitle(QString::fromUtf8("魔塔序章"));
    box.setText(QString::fromUtf8(
        "你在3层向前走时被守卫击晕，醒来后已经回到2层牢房。\n\n"
        "先去找小偷，他知道被夺走的铁剑和铁盾在哪里。"));
    box.exec();
}

void MainWindow::showNPCDialog(int x, int y)
{
    NPC* npc = m_game->npcAt(x, y);
    if (!npc) return;

    Player& p = m_game->player();

    // NPC 对话使用与地图图块相同的角色头像，保持角色身份连续。
    const QString npcPortrait = npc->IsTrader()
        ? QStringLiteral(":/images/characters/portraits/ririko.png")
        : QStringLiteral(":/images/characters/portraits/marina.png");
    auto showNpcInfo = [&](const QString& title, const QString& text) {
        QMessageBox box(this);
        box.setWindowTitle(title);
        box.setText(text);
        QPixmap avatar(npcPortrait);
        if (!avatar.isNull())
            box.setIconPixmap(avatar.scaled(60, 60, Qt::KeepAspectRatio, Qt::FastTransformation));
        box.exec();
    };

    // 原版关键 NPC 事件（保留一次性状态）。
    const int classicId = npc->ClassicId();
    if (!npc->HasGivenReward() && classicId == 3) {
        npc->Interact(p); // 领取怪物手册（主题名称：灯的歌词本）
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
    if (!npc->HasGivenReward() && classicId == 22) {
        auto reply = QMessageBox::question(this, QString::fromUtf8("公主"),
            QString::fromUtf8("谢谢你救了我！现在前往魔塔顶层吗？"), QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            npc->SetGiven(true);
            while (m_game->currentFloor() < 50) m_game->goUpFloor(p.x, p.y, false);
            showNpcInfo(QString::fromUtf8("公主"), QString::fromUtf8("我会在魔王身边等你。"));
        }
        updateHUD();
        return;
    }
    if (!npc->HasGivenReward() && classicId == 10) {
        auto reply = QMessageBox::question(this, QString::fromUtf8("不正经的商人"),
            QString::fromUtf8("给我 1 金币，试试你的运气？（1% 获得 88 金币）"), QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes && p.gold >= 1) {
            --p.gold;
            npc->SetGiven(true);
            if (QRandomGenerator::global()->bounded(100) == 0) p.gold += 88;
        }
        updateHUD();
        return;
    }
    if (!npc->HasGivenReward() && classicId == 24) {
        auto reply = QMessageBox::question(this, QString::fromUtf8("神秘的商人"),
            QString::fromUtf8("我会随机提升一项属性，同时扣除另一项属性，确定交易吗？"), QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            int gain = QRandomGenerator::global()->bounded(3);
            int loss = QRandomGenerator::global()->bounded(3);
            if (gain == 0) p.hp += 100; else if (gain == 1) p.atk += 10; else p.def += 10;
            if (loss == 0) p.hp = std::max(1, p.hp - 100); else if (loss == 1) p.atk = std::max(0, p.atk - 10); else p.def = std::max(0, p.def - 10);
            npc->SetGiven(true);
        }
        updateHUD();
        return;
    }
    if (classicId == 13) {
        // 原版2层小偷是两段式剧情：第一次确认主角醒来，第二次交代
        // 铁剑/铁盾所在楼层；第一次对话同时打开身边的暗道。
        if (!npc->HasGivenReward()) {
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    if (std::abs(dx) + std::abs(dy) == 1 && m_game->tileAt(x + dx, y + dy) == Tile_DarkWall)
                        m_game->setTile(x + dx, y + dy, Tile_Floor);
            npc->SetGiven(true);
            showNpcInfo(QString::fromUtf8("小偷"),
                QString::fromUtf8("你清醒了吗？这里是魔塔2层的牢房。"));
        } else {
            showNpcInfo(QString::fromUtf8("小偷"),
                QString::fromUtf8("你的剑和盾被警卫拿走了。铁剑在5层，铁盾在9层，先去找到它们。"));
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
    }

    // 交易NPC
    if (npc->IsTrader() && !npc->IsTradeDone()) {
        const Item* tradeReward = npc->GetTradeReward();
        QString rewardDesc;
        if (tradeReward)
            rewardDesc = getItemDescription(tradeReward);
        else
            rewardDesc = QString::fromUtf8("(无)");

        QString info = QString::fromUtf8(
            "【%1】\n\n"
            "交易物品: %2\n"
            "所需金币: %3\n"
            "你的金币: %4\n\n"
            "是否交易？")
            .arg(QString::fromStdString(npc->GetName()))
            .arg(rewardDesc)
            .arg(npc->GetTradeGoldCost())
            .arg(p.gold);

        bool canAfford = (npc->GetTradeGoldCost() <= p.gold);

        auto reply = QMessageBox::question(this, QString::fromUtf8("交易"),
            info,
            canAfford ? (QMessageBox::Yes | QMessageBox::No) : QMessageBox::No,
            QMessageBox::Yes);

        if (reply == QMessageBox::Yes && canAfford) {
            p.gold -= npc->GetTradeGoldCost();
            if (tradeReward) {
                // 创建可应用的物品副本
                auto item = Game::createItemByName(tradeReward->GetName(), tradeReward->GetValue());
                if (item) item->Apply(p);
            }
            npc->SetTradeDone(true);

            showNpcInfo(QString::fromStdString(npc->GetName()),
                QString::fromUtf8("交易成功！获得了 %1。").arg(rewardDesc));
        } else {
            // 显示NPC对话
            const auto& dialog = npc->Dialog();
            if (!dialog.empty()) {
                QString fullDialog;
                for (size_t i = 0; i < dialog.size(); ++i) {
                    fullDialog += QString::fromStdString(npc->GetName()) + ": " + QString::fromStdString(dialog[i]);
                    if (i + 1 < dialog.size()) fullDialog += "\n";
                }
                showNpcInfo(QString::fromStdString(npc->GetName()), fullDialog);
            }
        }

        ui.mapWidget->update();
        updateHUD();
        return;
    }

    // 普通NPC（非交易或交易已完成）
    bool hadReward = !npc->HasGivenReward();
    std::string reply = npc->Interact(p);

    if (hadReward && npc->HasGivenReward()) {
        showNpcInfo(QString::fromStdString(npc->GetName()), QString::fromStdString(reply));
    } else {
        const auto& dialog = npc->Dialog();
        QString fullDialog;
        for (size_t i = 0; i < dialog.size(); ++i) {
            fullDialog += QString::fromStdString(npc->GetName()) + ": " + QString::fromStdString(dialog[i]);
            if (i + 1 < dialog.size()) fullDialog += "\n";
        }
        showNpcInfo(QString::fromStdString(npc->GetName()), fullDialog);
    }
}

void MainWindow::showShopDialog(int x, int y)
{
    ShopData* shop = m_game->shopAt(x, y);
    if (!shop) return;

    Player& p = m_game->player();

    // 原版固定兑换商人：钥匙数量与价格保持 50 层魔塔配置。
    const int classicId = shop->classicNpcId;
    struct FixedOffer { QString text; int cost; std::function<void()> grant; };
    FixedOffer offer;
    bool fixed = true;
    switch (classicId) {
    case 7:  offer = {QString::fromUtf8("蓝色Live票 ×1"), 50, [&] { p.AddKey(KeyType::Blue); }}; break;
    case 8:  offer = {QString::fromUtf8("黄色Live票 ×5"), 50, [&] { p.AddKey(KeyType::Green, 5); }}; break;
    case 9:  offer = {QString::fromUtf8("红色Live票 ×5"), 800, [&] { p.AddKey(KeyType::Red, 5); }}; break;
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
        auto reply = QMessageBox::question(this, QString::fromUtf8("原版商人"),
            QString::fromUtf8("%1\n价格：%2 金币\n当前金币：%3\n购买吗？").arg(offer.text).arg(offer.cost).arg(p.gold),
            canAfford ? (QMessageBox::Yes | QMessageBox::No) : QMessageBox::No);
        if (reply == QMessageBox::Yes && canAfford) { p.gold -= offer.cost; offer.grant(); }
        updateHUD();
        return;
    }
    if (shop->classicShopFloor > 0) {
        const ClassicShopOffer offer = classicShopOfferForFloor(shop->classicShopFloor, p.shopUseCount);
        QDialog dlg(this);
        dlg.setWindowTitle(QString::fromUtf8("属性商店"));
        dlg.setFixedSize(400, 300);
        auto* layout = new QVBoxLayout(&dlg);
        layout->addWidget(new QLabel(QString::fromUtf8("原版商店：全局第 %1 次购买价格 %2 金币").arg(p.shopUseCount + 1).arg(offer.price), &dlg));
        struct Offer { QString name; QString effect; std::function<void()> apply; };
        const Offer offers[] = {
            {QString::fromUtf8("生命值"), QString::fromUtf8("+%1").arg(offer.hp), [&]{ p.hp += offer.hp; }},
            {QString::fromUtf8("攻击力"), QString::fromUtf8("+%1").arg(offer.atk), [&]{ p.atk += offer.atk; }},
            {QString::fromUtf8("防御力"), QString::fromUtf8("+%1").arg(offer.def), [&]{ p.def += offer.def; }}
        };
        for (const auto& item : offers) {
            auto* button = new QPushButton(QString::fromUtf8("购买 %1（%2）").arg(item.name, item.effect), &dlg);
            button->setEnabled(p.gold >= offer.price);
            QObject::connect(button, &QPushButton::clicked, &dlg, [&dlg, &p, offer, item] {
                p.gold -= offer.price;
                item.apply();
                ++p.shopUseCount;
                dlg.accept();
            });
            layout->addWidget(button);
        }
        auto* leave = new QPushButton(QString::fromUtf8("离开"), &dlg);
        QObject::connect(leave, &QPushButton::clicked, &dlg, &QDialog::reject);
        layout->addWidget(leave);
        applyRuntimeArtSkin(dlg);
        dlg.exec();
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

    for (auto& item : items) {
        auto* row = new QHBoxLayout();
        row->setSpacing(8);

        QString desc = item.basePrice > 0
            ? QString::fromUtf8("%1 (%2 G) — %3").arg(item.name).arg(item.actualPrice).arg(item.effectDesc)
            : QString::fromUtf8("%1 — 不售卖").arg(item.name);

        auto* label = new QLabel(desc, &dlg);
        label->setStyleSheet(item.basePrice > 0 ? "font-size: 13px;" : "color: #666; font-size: 13px;");
        row->addWidget(label, 1);

        auto* btn = new QPushButton(QString::fromUtf8("购买"), &dlg);
        btn->setFixedWidth(60);
        btn->setStyleSheet(
            "QPushButton { background: #3a5a3a; color: #d0d0d0; border: 1px solid #6a6; "
            "border-radius: 4px; padding: 4px 10px; font-size: 13px; }"
            "QPushButton:hover { background: #4a7a4a; }"
            "QPushButton:disabled { background: #333; color: #666; border-color: #444; }"
        );
        btn->setEnabled(item.basePrice > 0 && p.gold >= item.actualPrice);

        connect(btn, &QPushButton::clicked, &dlg, [&dlg, &item]() {
            item.apply();
            QMessageBox::information(&dlg, QString::fromUtf8("购买成功"),
                QString::fromUtf8("购买了 %1！%2（花费 %3 G）")
                    .arg(item.name).arg(item.effectDesc).arg(item.actualPrice));
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
    updateHUD();
}

void MainWindow::showModifier()
{
    Player& p = m_game->player();

    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("修改器"));
    dlg.setFixedSize(380, 480);
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
        {QString::fromUtf8("后台万能通行证"), 3, "#84d"},
        {QString::fromUtf8("舞台升降卡"), 0, "#aa0"},
        {QString::fromUtf8("撤场通行卡"), 0, "#a6a"},
        {QString::fromUtf8("破墙锤"), 0, "#864"},
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

    // 显示背包物品
    int invCount = m_game->player().InventoryCount();
    if (invCount > 0) {
        QString items;
        for (int i = 0; i < invCount; ++i) {
            auto* item = m_game->player().GetItem(i);
            if (item) {
                const std::string canonical = Game::canonicalItemName(item->GetName());
                const QString displayName = canonical.empty()
                    ? QString::fromUtf8("未知道具")
                    : QString::fromStdString(canonical);
                if (!items.isEmpty()) items += QString::fromUtf8("、");
                items += displayName;
            }
        }
        ui.invItemsLabel->setText(QString::fromUtf8("物品\n%1").arg(items));
        ui.invItemsLabel->setVisible(true);
    } else {
        ui.invItemsLabel->setVisible(false);
    }

    ui.invButton->setText(QString::fromUtf8("🎒 背包 (%1)").arg(invCount));

    updateMonsterPanel();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
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
    if (ui.mapWidget->isPlayerMoving()) {
        m_pendingMoveDx = dx;
        m_pendingMoveDy = dy;
        m_hasPendingMove = true;
        return;
    }

    // 检查上楼器/下楼器（传送到当前坐标，不找楼梯）
    if (m_game->player().stairUpUsed) {
        m_game->player().stairUpUsed = false;
        m_game->goUpFloor(m_game->player().x, m_game->player().y, false);
        ui.mapWidget->update();
        updateHUD();
        return;
    }
    if (m_game->player().stairDownUsed) {
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
    auto result = m_game->tryMovePlayer(nx, ny);

    switch (result) {
    case Game::Move_Block:
        break;
    case Game::Move_DoorLocked: {
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
        if (floorBefore == 3 && m_game->currentFloor() == 2)
            showOpeningPrisonStory();
        ui.mapWidget->update();
        updateHUD();
        break;
    case Game::Move_Pickup:
        ui.mapWidget->update();
        updateHUD();
        break;
    case Game::Move_Encounter: {
        Monster* m = m_game->monsterAt(nx, ny);
        if (!m) break;

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
            ui.mapWidget->update();
            updateHUD();
            showBattleFeedback(dlg);
            gameWin();
            return;
        } else if (fightRes == Game::Fight_PlayerWin) {
            ui.mapWidget->update();
            updateHUD();
            showBattleFeedback(dlg);
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
        ui.mapWidget->update();
        updateHUD();
        break;
    case Game::Move_StairsDown:
        m_game->goDownFloor(nx, ny);
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
