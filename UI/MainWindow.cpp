#include "MainWindow.h"
#include "MapWidget.h"
#include "BattleFeedback.h"
#include "StoryPresentation.h"
#include "StoryScript.h"
#include "BossBattlePresentation.h"
#include "BossEncounterFlow.h"
#include "MonsterVisual.h"
#include "ShopPresentation.h"
#include "Audio/GameAudio.h"
#include "Entities/MonsterDB.h"
#include <QPainter>
#include <QApplication>
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
#include <QSlider>
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
#include <QComboBox>
#include <QStandardPaths>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <QCloseEvent>
#include <algorithm>
#include <functional>
#include <iterator>

namespace {

class ClickableStoryDialog final : public QDialog {
public:
    explicit ClickableStoryDialog(bool mandatory, QWidget* parent = nullptr)
        : QDialog(parent), m_policy(mandatory) {}
    std::function<void()> advance;
    void markCompleted() { m_completed = true; }
    bool completed() const { return m_completed; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        Q_UNUSED(watched);
        if (event->type() == QEvent::KeyPress) {
            auto* key = static_cast<QKeyEvent*>(event);
            if (!key->isAutoRepeat() && advance) advance();
            return true;
        }
        if (event->type() == QEvent::MouseButtonRelease && advance &&
            !qobject_cast<QPushButton*>(watched)) {
            const auto* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton) {
                advance();
                return true;
            }
        }
        return QDialog::eventFilter(watched, event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && advance) {
            advance();
            event->accept();
            return;
        }
        QDialog::mouseReleaseEvent(event);
    }

    void reject() override
    {
        if (m_policy.canReject(m_completed)) QDialog::reject();
    }

    void closeEvent(QCloseEvent* event) override
    {
        if (m_policy.canReject(m_completed)) QDialog::closeEvent(event);
        else event->ignore();
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        if (!event->isAutoRepeat() && advance) advance();
        event->accept();
    }

private:
    StoryCompletionPolicy m_policy;
    bool m_completed = false;
};

QPixmap fillStoryCanvas(const QPixmap& source, const QSize& size, bool dimScene)
{
    if (source.isNull() || size.isEmpty()) return {};
    const QPixmap expanded = source.scaled(size, Qt::KeepAspectRatioByExpanding,
                                           Qt::SmoothTransformation);
    const int cropX = std::max(0, (expanded.width() - size.width()) / 2);
    const int cropY = std::max(0, (expanded.height() - size.height()) / 2);
    QPixmap result = expanded.copy(cropX, cropY, size.width(), size.height());
    QPainter painter(&result);
    painter.fillRect(result.rect(), dimScene ? QColor(9, 7, 21, 125)
                                             : QColor(9, 7, 21, 35));
    return result;
}

QString endDialogStyle()
{
    return QStringLiteral(
        "QMessageBox { background-color: #1a1a2e; color: #d0d0d0; }"
        "QLabel { color: #d0d0d0; font-size: 14px; }"
        "QPushButton { background: #3a3a5a; color: #d0d0d0; border: 1px solid #66a;"
        " border-radius: 4px; padding: 6px 16px; min-width: 80px; }"
        "QPushButton:hover { background: #4a4a7a; }");
}

QString oneTimeMerchantClue(int classicId)
{
    switch (classicId) {
    case 7:  return QString::fromUtf8("花门需要击败与它对应的守卫，钥匙无法直接打开。");
    case 8:  return QString::fromUtf8("红宝石提升攻击、蓝宝石提升防御；打不过时先核对怪物手册。");
    case 9:  return QString::fromUtf8("红色Live票很稀少，开红门前先观察门后的路线。");
    case 11: return QString::fromUtf8("15层击败大章鱼后，米歇尔会帮你打开暗墙。");
    case 16: return QString::fromUtf8("29层的米歇尔离开后，会回到2层右下角的牢笼。");
    case 27: return QString::fromUtf8("32层骑士队长会先攻，进入中央前请留足生命。");
    case 36: return QString::fromUtf8("36层藏着多段浅色暗墙，路被挡住时试着靠近检查。");
    case 38: return QString::fromUtf8("39层左上房间打开指定的两扇黄门后，会出现镜面舞台票。");
    case 41: return QString::fromUtf8("43层的普通楼梯会直达45层，44层要靠上楼器或下楼器进入。");
    case 44: return QString::fromUtf8("神圣盾能免疫巫师靠近伤害与魔法警卫的夹击。");
    default: return QString::fromUtf8("固定兑换摊位只能交易一次；收集的线索可在灯的单词本里回看。");
    }
}

} // namespace

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

static QString resolvedMonsterVisualPath(const std::string& name)
{
    const QString spritePath = QString::fromStdString(monsterSpriteResourcePath(name));
    if (!spritePath.isEmpty() && !QPixmap(spritePath).isNull()) return spritePath;

    const QString portraitPath = monsterPortraitPath(name);
    if (!portraitPath.isEmpty() && !QPixmap(portraitPath).isNull()) return portraitPath;

    const int index = MonsterDB::indexOf(name);
    return index >= 0
        ? QString(":/images/monster_%1.png").arg(index + 1, 2, 10, QChar('0'))
        : QString();
}

static void applyRuntimeArtSkin(QWidget& widget)
{
    widget.setStyleSheet(
        "QDialog, QMessageBox { background-color: #171320; background-image: url(:/images/runtime/ui/panel_texture.png); color: #f1e8d0; }"
        "QGroupBox, QTabWidget::pane, QListWidget, QScrollArea { background: rgba(14,15,25,210); color: #f1e8d0; border: 2px solid #777080; }"
        "QLineEdit, QTextEdit, QSpinBox, QComboBox { background: rgba(14,15,25,220); color: white; border: 1px solid #9b93a3; padding: 4px; }"
        "QLabel, QCheckBox { color: #f1e8d0; }"
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
    if (name == QString::fromUtf8("现场补给")) return QStringLiteral(":/images/runtime/items/potion.png");
    if (name == QString::fromUtf8("灯的热牛奶")) return QStringLiteral(":/images/runtime/items/mygo/tomori_warm_milk.png");
    if (name == QString::fromUtf8("爱音能量饮")) return QStringLiteral(":/images/runtime/items/mygo/anon_energy_drink.png");
    if (name == QString::fromUtf8("红宝石")) return QStringLiteral(":/images/runtime/items/ruby_gem.png");
    if (name == QString::fromUtf8("蓝宝石")) return QStringLiteral(":/images/runtime/items/sapphire_gem.png");
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

MainWindow::MainWindow(Game* game, QWidget* parent, bool playFirstFloorOpening, bool ownsGame)
    : QWidget(parent), m_game(game), m_ownsGame(ownsGame), m_playFirstFloorOpening(playFirstFloorOpening)
{
    GameAudio::prepare();
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
    // Re-enable animation once for the regenerated 8x8 walk sheet. The user
    // can still turn it off afterwards; this only migrates older saved settings
    // that may have disabled animation while the previous sheet was broken.
    constexpr int walkAnimationAssetVersion = 2;
    if (settings.value(QStringLiteral("walkAnimationAssetVersion"), 0).toInt() <
        walkAnimationAssetVersion) {
        settings.setValue(QStringLiteral("movementAnimation"), true);
        settings.setValue(QStringLiteral("walkAnimationAssetVersion"),
                          walkAnimationAssetVersion);
    }
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
        const int targetTileBefore = m_game->tileAt(x, y);
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
            m_pendingTeleport.targetTileBefore = targetTileBefore;
            m_pendingTeleport.pickedName = pickedName;
            m_pendingTeleport.pickedDescription = pickedDescription;
            ui.mapWidget->playPlayerPath(teleportPath);
            return;
        }

        const auto result = m_game->completeTeleportPlayerTo();
        ui.mapWidget->snapPlayerToGame();
        startMonsterMovementAnimation();
        handleTeleportResult(x, y, floorBefore, pickedName, pickedDescription,
                             result, targetTileBefore);
    });
    updateHUD();
    // Animation-off mode has no playerMotionFinished signal. Keep held input
    // at the same six-tiles-per-second cadence without OS key-repeat latency.
    m_movementQueueTimer.setInterval(167);
    connect(&m_movementQueueTimer, &QTimer::timeout, this, &MainWindow::flushPendingMove);
    m_movementQueueTimer.start();
    // 动画结束的同一帧先提交鼠标瞬移的目标交互，再衔接键盘移动队列。
    connect(ui.mapWidget, &MapWidget::playerMotionFinished, this, [this]() {
        if (m_pendingTeleport.active) {
            completePendingTeleport();
            return;
        }
        flushPendingMove();
    });
    connect(ui.mapWidget, &MapWidget::monsterMotionFinished, this, [this]() {
        if (m_floor32KnightStoryFloor >= 0) {
            const int floorBefore = m_floor32KnightStoryFloor;
            m_floor32KnightStoryFloor = -1;
            showFloor32KnightStoryIfNeeded(floorBefore);
        } else if (m_floor42CaptureStoryQueued) {
            m_floor42CaptureStoryQueued = false;
            showFloor42CaptureStory();
        }
        showPendingApproachHazardCgs();
        ui.mapWidget->update();
        updateHUD();
    });
    connect(&m_battleFeedbackTimer, &QTimer::timeout, this, [this]() {
        if (ui.battleLabel) ui.battleLabel->setVisible(false);
    });

    connect(ui.saveButton, &QPushButton::clicked, this, [this]() {
        showSaveLoadDialog(true);
    });

    connect(ui.loadButton, &QPushButton::clicked, this, [this]() {
        showSaveLoadDialog(false);
    });

    connect(ui.quickSaveButton, &QPushButton::clicked, this, &MainWindow::quickSave);
    connect(ui.undoButton, &QPushButton::clicked, this, &MainWindow::undoLastAction);
    connect(ui.settingsButton, &QPushButton::clicked, this, &MainWindow::showSettings);
    connect(ui.modButton, &QPushButton::clicked, this, &MainWindow::showModifier);
}

void MainWindow::handleTeleportResult(int x, int y, int floorBefore,
                                      const QString& pickedName,
                                      const QString& pickedDescription,
                                      Game::MoveResult result, int targetTileBefore)
{
    showPendingApproachHazardCgs();
    switch (result) {
        case Game::Move_Pickup:
        case Game::Move_Ok:
            if (result == Game::Move_Pickup)
                GameAudio::play(GameAudio::Cue::Pickup);
            else if (targetTileBefore == Tile_DoorRed || targetTileBefore == Tile_DoorBlue ||
                     targetTileBefore == Tile_DoorGreen || targetTileBefore == Tile_DoorMagic ||
                     targetTileBefore == Tile_DoorIron || targetTileBefore == Tile_DarkWall)
                GameAudio::play(GameAudio::Cue::Door);
            else
                GameAudio::play(GameAudio::Cue::Step);
            if (result == Game::Move_Pickup && !pickedName.isEmpty())
                showBattleFeedback(QString::fromUtf8("获得 %1：%2").arg(pickedName).arg(
                    QString(pickedDescription).replace(QString::fromUtf8("（未生效）"), QString::fromUtf8("（已生效）"))));
            if (result == Game::Move_Pickup)
                showStoryPickup(floorBefore, pickedName);
            if (m_game->floor3PrisonStoryPending())
                showPrisonTrapPrompt();
            else if (floorBefore == 3 && m_game->currentFloor() == 2)
                showOpeningPrisonStory();
            showFloor10AmbushStoryIfNeeded();
            showFloor20VampireStoryIfNeeded(floorBefore);
            showFloor33TrapStoryIfNeeded(floorBefore);
            showFloor32KnightStoryAfterMovement(floorBefore);
            showStoryMilestones();
            ui.mapWidget->update();
            updateHUD();
            break;
        case Game::Move_NPC:
            GameAudio::play(GameAudio::Cue::Dialogue);
            showNPCDialog(x, y);
            ui.mapWidget->update();
            updateHUD();
            break;
        case Game::Move_Shop:
            GameAudio::play(GameAudio::Cue::Shop);
            showShopDialog(x, y);
            ui.mapWidget->update();
            updateHUD();
            break;
        case Game::Move_Encounter: {
            if (!m_game->canDefeatMonsterAt(x, y)) {
                GameAudio::play(GameAudio::Cue::Blocked);
                ui.mapWidget->snapPlayerToGame();
                showBattleFeedback(QString::fromUtf8("你无法击败对方。请先提升属性或恢复生命。"));
                updateHUD();
                break;
            }
            if (runBossBattleAt(x, y)) break;
            GameAudio::play(GameAudio::Cue::Battle);
            // 点击怪物格也走统一战斗入口；瞬移后玩家坐标已在目标格，
            // 因而战斗结束后可以继续从该格移动或拾取战利品。
            const bool knightFight = floorBefore == 32 && m_game->monsterAt(x, y) &&
                MonsterDB::hasIndex(m_game->monsterAt(x, y)->GetName(), 24);
            std::vector<std::string> log;
            const auto fightResult = m_game->fightAt(x, y, log);
            showBattleFeedback(QString::fromStdString(summarizeBattleLog(log)));
            if (knightFight && fightResult == Game::Fight_PlayerWin)
                showScriptSceneOnce(20);
            if (fightResult == Game::Fight_PlayerWin)
                GameAudio::play(GameAudio::Cue::Victory);
            if (fightResult == Game::Fight_PlayerWin)
                showStoryMilestones();
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
            GameAudio::play(GameAudio::Cue::Stairs);
            m_game->goUpFloor(x, y);
            showOpeningFloorStory(floorBefore, m_game->currentFloor());
            ui.mapWidget->update();
            updateHUD();
            break;
        case Game::Move_StairsDown:
            GameAudio::play(GameAudio::Cue::Stairs);
            m_game->goDownFloor(x, y);
            showOpeningFloorStory(floorBefore, m_game->currentFloor());
            ui.mapWidget->update();
            updateHUD();
            break;
        default:
            if (result == Game::Move_DoorLocked)
                GameAudio::play(GameAudio::Cue::Blocked);
            break;
    }
}

void MainWindow::completePendingTeleport()
{
    if (!m_pendingTeleport.active) return;
    const PendingTeleport pending = m_pendingTeleport;
    m_pendingTeleport = {};
    const auto result = m_game->completeTeleportPlayerTo();
    if (result == Game::Move_Block)
        ui.mapWidget->snapPlayerToGame();
    startMonsterMovementAnimation();
    handleTeleportResult(pending.x, pending.y, pending.floorBefore,
                         pending.pickedName, pending.pickedDescription, result,
                         pending.targetTileBefore);
    ui.mapWidget->update();
}

void MainWindow::flushPendingMove()
{
    if (QApplication::activeModalWidget()) {
        m_hasPendingMove = false;
        m_heldMoveState.clear();
        return;
    }
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
    if (!ui.battleLabel ||
        (!m_battleFeedbackEnabled && !message.contains(QString::fromUtf8("无法击败"))))
        return;
    showTransientFeedback(message);
}

void MainWindow::showTransientFeedback(const QString& message)
{
    if (!ui.battleLabel) return;
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
    GameAudio::play(GameAudio::Cue::Undo);
    ui.undoButton->setEnabled(!m_undoHistory.empty());
    m_hasPendingMove = false;
    m_pendingTeleport = {};
    m_floor32KnightStoryFloor = -1;
    ui.mapWidget->snapPlayerToGame();
    ui.mapWidget->update();
    updateHUD();
}

MainWindow::~MainWindow()
{
    if (m_ownsGame) delete m_game;
}

static bool showSaveSlots(QWidget* parent, Game& game, bool initialSave, bool allowSave)
{
    QDialog dlg(parent);
    dlg.setObjectName(QStringLiteral("saveSlotsDialog"));
    dlg.setWindowTitle(QString::fromUtf8("魔塔存档"));
    dlg.setFixedSize(520, 460);
    applyRuntimeArtSkin(dlg);
    auto* layout = new QVBoxLayout(&dlg);
    layout->addWidget(new QLabel(QString::fromUtf8(
        "即时存档可随时覆盖；另有 10 个普通存档槽位。保存和读取都在游戏内完成。"), &dlg));
    auto* slotList = new QListWidget(&dlg);
    layout->addWidget(slotList, 1);

    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) dir = QDir::currentPath();
    const QString saveDir = QDir(dir).filePath(QStringLiteral("saves"));
    QDir().mkpath(saveDir);
    const QString quickPath = QDir(dir).filePath(QStringLiteral("quicksave.sav"));

    const auto slotPath = [&saveDir](int slot) {
        return QDir(saveDir).filePath(QStringLiteral("slot_%1.sav").arg(slot));
    };
    const auto selectedPath = [&](int slot) {
        return slot == 0 ? quickPath : slotPath(slot);
    };
    const auto refreshSlots = [&]() {
        slotList->clear();
        const QFileInfo quickInfo(quickPath);
        auto* quickRow = new QListWidgetItem(quickInfo.exists()
            ? QString::fromUtf8("即时存档    已保存（%1 KB）").arg(quickInfo.size() / 1024)
            : QString::fromUtf8("即时存档    空槽位"), slotList);
        quickRow->setData(Qt::UserRole, 0);
        for (int i = 1; i <= 10; ++i) {
            const QString path = slotPath(i);
            const QFileInfo info(path);
            const QString state = info.exists()
                ? QString::fromUtf8("已有存档（%1 KB）").arg(info.size() / 1024)
                : QString::fromUtf8("空槽位");
            auto* row = new QListWidgetItem(QString::fromUtf8("存档 %1    %2").arg(i).arg(state), slotList);
            row->setData(Qt::UserRole, i);
        }
        int defaultRow = initialSave || !quickInfo.exists() ? 1 : 0;
        if (!initialSave && !quickInfo.exists()) {
            for (int i = 1; i <= 10; ++i) {
                if (QFileInfo::exists(slotPath(i))) {
                    defaultRow = i;
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
    auto* close = new QPushButton(allowSave ? QString::fromUtf8("返回游戏")
                                          : QString::fromUtf8("返回主菜单"), &dlg);
    save->setObjectName(QStringLiteral("saveSelectedSlot"));
    load->setObjectName(QStringLiteral("loadSelectedSlot"));
    close->setObjectName(QStringLiteral("closeSaveSlots"));
    save->setVisible(allowSave);
    buttons->addWidget(save);
    buttons->addWidget(load);
    buttons->addWidget(close);
    layout->addLayout(buttons);

    QObject::connect(save, &QPushButton::clicked, &dlg, [&]() {
        auto* row = slotList->currentItem();
        if (!row) return;
        const int slot = row->data(Qt::UserRole).toInt();
        if (game.saveToFile(selectedPath(slot).toStdString())) {
            status->setText(slot == 0 ? QString::fromUtf8("即时存档已保存。")
                                      : QString::fromUtf8("已保存到存档 %1").arg(slot));
            refreshSlots();
            slotList->setCurrentRow(slot);
        } else {
            status->setText(QString::fromUtf8("保存失败，请重试。"));
        }
    });
    QObject::connect(load, &QPushButton::clicked, &dlg, [&]() {
        auto* row = slotList->currentItem();
        if (!row) return;
        const int slot = row->data(Qt::UserRole).toInt();
        const QString path = selectedPath(slot);
        if (!QFileInfo::exists(path)) {
            status->setText(slot == 0 ? QString::fromUtf8("即时存档为空。")
                                      : QString::fromUtf8("存档 %1 为空。").arg(slot));
            return;
        }
        if (!game.loadFromFile(path.toStdString())) {
            status->setText(QString::fromUtf8("读取失败，请重试。"));
            return;
        }
        dlg.accept();
    });
    QObject::connect(close, &QPushButton::clicked, &dlg, &QDialog::reject);
    return dlg.exec() == QDialog::Accepted;
}

bool MainWindow::loadFromSlots(Game& game, QWidget* parent)
{
    return showSaveSlots(parent, game, false, false);
}

void MainWindow::showSaveLoadDialog(bool initialSave)
{
    if (!showSaveSlots(this, *m_game, initialSave, true)) return;
    clearUndoHistory();
    m_hasPendingMove = false;
    m_pendingTeleport = {};
    m_floor32KnightStoryFloor = -1;
    ui.mapWidget->snapPlayerToGame();
    ui.mapWidget->update();
    updateHUD();
}

void MainWindow::quickSave()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) dir = QDir::currentPath();
    QDir().mkpath(dir);
    const QString path = QDir(dir).filePath(QStringLiteral("quicksave.sav"));
    const bool ok = m_game->saveToFile(path.toStdString());
    showTransientFeedback(ok
        ? QString::fromUtf8("即时存档已保存，按 F9 可读取。")
        : QString::fromUtf8("即时存档失败，请检查存储空间。"));
}

void MainWindow::quickLoad()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) dir = QDir::currentPath();
    const QString path = QDir(dir).filePath(QStringLiteral("quicksave.sav"));
    if (!QFileInfo::exists(path)) {
        showTransientFeedback(QString::fromUtf8("还没有即时存档。"));
        return;
    }
    if (!m_game->loadFromFile(path.toStdString())) {
        showTransientFeedback(QString::fromUtf8("即时存档读取失败。"));
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
    showTransientFeedback(QString::fromUtf8("即时存档已读取。"));
}

void MainWindow::showSettings()
{
    bool returnToMenu = false;
    QSettings settings(QStringLiteral("MyGO-Mota"), QStringLiteral("MyGO-Mota"));
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("设置"));
    dlg.setFixedSize(480, 510);
    auto* layout = new QVBoxLayout(&dlg);
    auto* animation = new QCheckBox(QString::fromUtf8("启用连续移动动画"), &dlg);
    animation->setChecked(ui.mapWidget->movementAnimationEnabled());
    auto* battle = new QCheckBox(QString::fromUtf8("显示战斗结果提示"), &dlg);
    battle->setChecked(m_battleFeedbackEnabled);
    layout->addWidget(animation);
    layout->addWidget(battle);
    auto* sound = new QCheckBox(QString::fromUtf8("启用音效"), &dlg);
    sound->setChecked(settings.value(QStringLiteral("soundEffectsEnabled"), true).toBool());
    layout->addWidget(sound);
    auto* volumeRow = new QHBoxLayout;
    volumeRow->addWidget(new QLabel(QString::fromUtf8("音效音量"), &dlg));
    auto* volume = new QSlider(Qt::Horizontal, &dlg);
    volume->setRange(0, 100);
    volume->setValue(settings.value(QStringLiteral("soundEffectsVolume"), 65).toInt());
    volumeRow->addWidget(volume, 1);
    auto* volumeValue = new QLabel(QString::number(volume->value()) + "%", &dlg);
    volumeRow->addWidget(volumeValue);
    connect(volume, &QSlider::valueChanged, volumeValue, [volumeValue](int value) {
        volumeValue->setText(QString::number(value) + "%");
    });
    volume->setEnabled(sound->isChecked());
    connect(sound, &QCheckBox::toggled, volume, &QWidget::setEnabled);
    layout->addLayout(volumeRow);
    auto* music = new QCheckBox(QString::fromUtf8("启用小游戏背景音乐"), &dlg);
    music->setChecked(settings.value(QStringLiteral("backgroundMusicEnabled"), true).toBool());
    layout->addWidget(music);
    auto* musicVolumeRow = new QHBoxLayout;
    musicVolumeRow->addWidget(new QLabel(QString::fromUtf8("音乐音量"), &dlg));
    auto* musicVolume = new QSlider(Qt::Horizontal, &dlg);
    musicVolume->setRange(0, 100);
    musicVolume->setValue(settings.value(QStringLiteral("backgroundMusicVolume"), 38).toInt());
    musicVolumeRow->addWidget(musicVolume, 1);
    auto* musicValue = new QLabel(QString::number(musicVolume->value()) + "%", &dlg);
    musicVolumeRow->addWidget(musicValue);
    connect(musicVolume, &QSlider::valueChanged, musicValue, [musicValue](int value) {
        musicValue->setText(QString::number(value) + "%");
    });
    musicVolume->setEnabled(music->isChecked());
    connect(music, &QCheckBox::toggled, musicVolume, &QWidget::setEnabled);
    layout->addLayout(musicVolumeRow);
    auto* musicTrackRow = new QHBoxLayout;
    musicTrackRow->addWidget(new QLabel(QString::fromUtf8("小游戏曲目"), &dlg));
    auto* musicTrack = new QComboBox(&dlg);
    musicTrack->setObjectName(QStringLiteral("settingsMusicTrack"));
    musicTrack->addItems({QString::fromUtf8("春日影 · 8-bit"),
                          QString::fromUtf8("KiLLKiSS · 8-bit")});
    musicTrack->setCurrentIndex(GameAudio::miniGameMusicTrack());
    musicTrack->setEnabled(music->isChecked());
    connect(music, &QCheckBox::toggled, musicTrack, &QWidget::setEnabled);
    musicTrackRow->addWidget(musicTrack, 1);
    layout->addLayout(musicTrackRow);
    layout->addWidget(new QLabel(QString::fromUtf8(
        "方向键移动；可点击连通区域内的地板、怪物和门。\n"
        "点击怪物会战斗；点击门需持有对应钥匙。\n"
        "拿到怪物手册后，左侧才会显示怪物与预计损失。\n"
        "点击右侧道具图标使用或查看；单词本可重读线索。\n"
        "F5 即时存档，F9 即时读档，Ctrl+Z 连续撤销。\n"
        "返回主菜单前请先保存，未保存的进度不会保留。"), &dlg));
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    buttons->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8("应用"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("取消"));
    auto* exitButton = new QPushButton(QString::fromUtf8("退出游戏"), &dlg);
    auto* menuButton = new QPushButton(QString::fromUtf8("返回主菜单"), &dlg);
    menuButton->setObjectName(QStringLiteral("returnToMenuButton"));
    exitButton->setStyleSheet(QStringLiteral("QPushButton { color: #ff9a9a; }"));
    layout->addStretch();
    layout->addWidget(buttons);
    layout->addWidget(menuButton);
    layout->addWidget(exitButton);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, [&]() {
        ui.mapWidget->setMovementAnimationEnabled(animation->isChecked());
        m_battleFeedbackEnabled = battle->isChecked();
        settings.setValue(QStringLiteral("movementAnimation"), animation->isChecked());
        settings.setValue(QStringLiteral("battleFeedback"), battle->isChecked());
        settings.setValue(QStringLiteral("soundEffectsEnabled"), sound->isChecked());
        settings.setValue(QStringLiteral("soundEffectsVolume"), volume->value());
        settings.setValue(QStringLiteral("backgroundMusicEnabled"), music->isChecked());
        settings.setValue(QStringLiteral("backgroundMusicVolume"), musicVolume->value());
        settings.setValue(QStringLiteral("backgroundMusicTrack"), musicTrack->currentIndex());
        GameAudio::refreshSettings();
        GameAudio::play(GameAudio::Cue::MenuSelect);
        dlg.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(menuButton, &QPushButton::clicked, &dlg, [&]() {
        returnToMenu = true;
        dlg.accept();
    });
    connect(exitButton, &QPushButton::clicked, &dlg, [&dlg]() {
        if (QMessageBox::question(&dlg, QString::fromUtf8("退出游戏"),
                QString::fromUtf8("确定要退出游戏吗？"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
            QApplication::quit();
        }
    });
    applyRuntimeArtSkin(dlg);
    dlg.exec();
    if (returnToMenu) {
        m_movementQueueTimer.stop();
        m_heldMoveState.clear();
        m_hasPendingMove = false;
        m_pendingTeleport = {};
        close();
    }
}

void MainWindow::showStoryMessage(const QString& message)
{
    showVisualNovelDialogue({
        {QString::fromUtf8("旁白"), message,
         QStringLiteral(":/images/characters/portraits/marina.png"), QStringLiteral("#ffd66b")}
    });

}

void MainWindow::loadAssets()
{
    auto* mw = ui.mapWidget;

    // Galgame 使用高分辨率立绘，地图行走使用用户指定的最新 8×8 参考素材。
    mw->loadPlayerImage(":/images/characters/portraits/anon.png");
    mw->loadPlayerOutfitSpriteSheet("reference_walk", ":/images/characters/player_outfits/anon_reference_walk_8x8_hq.png");
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
    // 属性商店由弦卷心经营；信息 NPC 用凛凛子，固定交易 NPC 用麻里奈。
    mw->loadTileImage(Tile_Shop,        ":/images/characters/npcs/kokoro_pico_240.png");
    mw->loadTileImage(Tile_NPC,         ":/images/characters/npcs/ririko_pico_240.png");
    mw->loadNPCImage("弦卷心",          ":/images/characters/npcs/kokoro_pico_240.png");
    mw->loadNPCImage("麻里奈",          ":/images/characters/npcs/marina_pico_240.png");
    mw->loadNPCImage("凛凛子",          ":/images/characters/npcs/ririko_pico_240.png");
    mw->loadNPCImage("老头",             ":/images/characters/npcs/ririko_pico_240.png");
    mw->loadNPCImage("小偷",             ":/images/characters/npcs/michelle_icon.png");
    mw->loadNPCImage("米歇尔",           ":/images/characters/npcs/michelle_icon.png");

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
        {"现场补给",      ":/images/runtime/items/potion.png"},
        {"灯的热牛奶",    ":/images/runtime/items/mygo/tomori_warm_milk.png"},
        {"爱音能量饮",    ":/images/runtime/items/mygo/anon_energy_drink.png"},
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
        const QString path = resolvedMonsterVisualPath(monsters[i].GetName());
        mw->loadMonsterImage(monsters[i].GetName(), path);
    }

    mw->update();
    if (m_playFirstFloorOpening) {
        QTimer::singleShot(0, this, [this]() { showFirstFloorOpeningStory(); });
    }
}

bool MainWindow::runBossBattleAt(int x, int y, bool floor32FirstStrike)
{
    Monster* monster = m_game->monsterAt(x, y);
    BossEncounterId bossId = BossEncounterId::None;
    if (floor32FirstStrike && m_game->currentFloor() == 32 &&
        m_game->floor32KnightStoryPending() && x == 7 && y == 10) {
        bossId = BossEncounterId::Floor32Knight;
    } else if (monster) {
        bossId = classifyBossEncounter(m_game->currentFloor(), monster->GetName());
    }
    const BossBattleDescriptor* descriptor = bossBattleDescriptor(bossId);
    if (!descriptor || m_game->bossEncounterStateAt(x, y) != Game::BossEncounterState::Ready)
        return false;
    const int openingDamage = floor32FirstStrike
        ? std::max(0, MonsterDB::getByIndex(24).GetATK() - m_game->player().def) : 0;
    if (!m_game->canDefeatMonsterAt(x, y, openingDamage)) {
        showBattleFeedback(QString::fromUtf8("你无法击败对方。请先提升属性或恢复生命。"));
        return true;
    }

    const auto toQString = [](std::string_view text) {
        return QString::fromUtf8(text.data(), static_cast<int>(text.size()));
    };
    std::vector<std::string> battleLog;
    Game::FightResult gameFightResult = Game::Fight_Stalemate;
    BossEncounterFlowCallbacks callbacks;
    callbacks.present = [&]() {
        if (bossId == BossEncounterId::Floor32Knight) {
            return showVisualNovelDialogue(storyPages(19), true);
        }
        if (bossId == BossEncounterId::Floor40Knight)
            return showVisualNovelDialogue(storyPages(24), true);
        if (bossId == BossEncounterId::Floor50Soyo)
            return showVisualNovelDialogue(storyPages(29), true);
        return showVisualNovelDialogue({
            {toQString(descriptor->speaker), toQString(descriptor->dialogue),
             toQString(descriptor->portraitPath), toQString(descriptor->accent),
             toQString(descriptor->cgPath)}
        }, true);
    };
    callbacks.markStory = [&]() { m_game->markStoryShown(std::string(descriptor->storyKey)); };
    callbacks.firstStrike = [&]() {
        const int damage = m_game->resolveFloor32KnightStory();
        if (damage > 0)
            showBattleFeedback(QString::fromUtf8("骑士队长先攻，造成 %1 点伤害。").arg(damage));
        updateHUD();
        return m_game->player().hp > 0;
    };
    callbacks.fight = [&]() {
        GameAudio::play(GameAudio::Cue::Battle);
        gameFightResult = m_game->fightAt(x, y, battleLog);
        switch (gameFightResult) {
        case Game::Fight_PlayerWin: return BossFlowFightResult::PlayerWin;
        case Game::Fight_PlayerDead: return BossFlowFightResult::PlayerDead;
        case Game::Fight_GameWin: return BossFlowFightResult::GameWin;
        case Game::Fight_Stalemate: return BossFlowFightResult::Stalemate;
        }
        return BossFlowFightResult::Stalemate;
    };
    callbacks.postFight = [&](BossFlowFightResult result) {
        ui.mapWidget->update();
        updateHUD();
        showBattleFeedback(QString::fromStdString(summarizeBattleLog(battleLog)));
        if (result == BossFlowFightResult::PlayerWin || result == BossFlowFightResult::GameWin)
            GameAudio::play(GameAudio::Cue::Victory);
        if (result == BossFlowFightResult::PlayerWin && bossId == BossEncounterId::Floor32Knight)
            showScriptSceneOnce(20);
        if (result == BossFlowFightResult::PlayerWin && bossId == BossEncounterId::Floor10Umiri)
            showScriptSceneOnce(10);
        if (result == BossFlowFightResult::PlayerWin || result == BossFlowFightResult::GameWin)
            startMonsterMovementAnimation();
        if (result == BossFlowFightResult::PlayerDead) gameOver();
        else if (result == BossFlowFightResult::GameWin) gameWin();
    };
    callbacks.gameOver = [&]() {
        ui.mapWidget->update();
        updateHUD();
        gameOver();
    };

    runBossEncounterFlow({true, m_game->storyShown(std::string(descriptor->storyKey)),
                          floor32FirstStrike}, callbacks);
    return true;
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
    if (name == QString::fromUtf8("红宝石"))
        return QString::fromUtf8("攻击力 +%1（拾取即生效）").arg(val);
    if (name == QString::fromUtf8("蓝宝石"))
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
    if (name == QString::fromUtf8("Mujica烟雾弹"))
        return QString::fromUtf8("确认后击败周围可爆破的怪物");
    if (name == QString::fromUtf8("Mujica舞台震响卷"))
        return QString::fromUtf8("确认后清除本层可破坏的墙壁");
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
        return QString::fromUtf8("点击阅读已收集的 NPC 与商人线索");
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
    if (dynamic_cast<const NoteBook*>(item) != nullptr) {
        showNotebookDialog();
        return;
    }
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

    if (isBomb || isEarthquake || isMagicKey) {
        const QString effect = isMagicKey
            ? QString::fromUtf8("打开当前楼层所有黄色门")
            : isBomb ? QString::fromUtf8("击败身旁可爆破的怪物")
                     : QString::fromUtf8("清除当前楼层可破坏的墙壁");
        if (!showVisualNovelChoice(itemName,
                QString::fromUtf8("%1：%2。确认后会消耗此道具。")
                    .arg(itemName, effect),
                QStringLiteral(":/images/characters/portraits/variants/anon_confident.png"),
                QString::fromUtf8("确认使用"), QString::fromUtf8("取消")))
            return;
    }

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
            QDialog preview(this);
            preview.setWindowTitle(QString::fromUtf8("Mujica镜面舞台票 · 预览"));
            auto* layout = new QVBoxLayout(&preview);
            auto* title = new QLabel(QString::fromUtf8("当前位置 (%1,%2) → 中心对称位置 (%3,%4)")
                                         .arg(m_game->player().x).arg(m_game->player().y)
                                         .arg(mirroredX).arg(mirroredY), &preview);
            title->setAlignment(Qt::AlignCenter);
            layout->addWidget(title);
            QPixmap mapPreview = ui.mapWidget->grab();
            if (!mapPreview.isNull()) {
                QPainter marker(&mapPreview);
                const qreal sx = mapPreview.width() / qreal(m_game->width());
                const qreal sy = mapPreview.height() / qreal(m_game->height());
                marker.setPen(QPen(QColor(255, 80, 190), 5));
                marker.setBrush(QColor(255, 80, 190, 70));
                marker.drawRect(QRectF(mirroredX * sx, mirroredY * sy, sx, sy).adjusted(3, 3, -3, -3));
                auto* image = new QLabel(&preview);
                image->setPixmap(mapPreview.scaled(660, 660, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                image->setAlignment(Qt::AlignCenter);
                layout->addWidget(image);
            }
            auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &preview);
            buttons->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8("确认使用"));
            buttons->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("取消"));
            connect(buttons, &QDialogButtonBox::accepted, &preview, &QDialog::accept);
            connect(buttons, &QDialogButtonBox::rejected, &preview, &QDialog::reject);
            layout->addWidget(buttons);
            applyRuntimeArtSkin(preview);
            if (preview.exec() != QDialog::Accepted) return;
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
    auto pages = storyPages(4);
    pages.push_back({QString::fromUtf8("藤都子"),
                     QString::fromUtf8("临兵斗者皆阵列在前！"), QString(),
                     QStringLiteral("#b9a7ff"),
                     QStringLiteral(":/images/runtime/cg/miyako_kuji_spell.png")});
    showVisualNovelDialogue(pages, true);
    m_game->markStoryShown("anon_soyo_scene_4");
    m_game->markStoryShown("floor3_prison_dialogue");

    // 对话全部播放完后才结算伤害并传送回二层，保留原版剧情节奏。
    if (m_game->floor3PrisonStoryPending()) {
        m_game->resolveFloor3PrisonStory();
        showOpeningPrisonStory();
        ui.mapWidget->update();
        updateHUD();
    }
}

bool MainWindow::showVisualNovelDialogue(const std::vector<VisualNovelPage>& pages, bool mandatory)
{
    if (pages.empty()) return true;
    const QPixmap sceneSnapshot = grab();
    ClickableStoryDialog dlg(mandatory, this);
    dlg.setWindowTitle(QString::fromUtf8("剧情"));
    dlg.setModal(true);
    dlg.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setFixedSize(size());
    dlg.setStyleSheet(
        "QDialog { background: #090715; border: none; }"
        "QLabel#vnName { font-size: 20px; font-weight: 800; padding: 3px 0; }"
        "QLabel#vnText { color: #fff4e6; font-size: 18px; }"
        "QLabel#vnPortrait { background: transparent; border: none; }"
        "QFrame#vnBox { background: rgba(18, 15, 35, 224); border-top: 2px solid #d5a8f0; }"
        "QPushButton { color: #fff7d0; background: #5f3d79; border: 1px solid #d5a8f0;"
        " border-radius: 6px; padding: 8px 24px; font-size: 15px; font-weight: 700; }"
        "QPushButton:hover { background: #79509a; }"
    );

    auto* backdrop = new QLabel(&dlg);
    backdrop->setObjectName(QStringLiteral("vnBackdrop"));
    backdrop->setGeometry(dlg.rect());
    backdrop->setAlignment(Qt::AlignCenter);
    backdrop->setScaledContents(false);

    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    auto* stageWidget = new QWidget(&dlg);
    auto* stage = new QHBoxLayout(stageWidget);
    stage->setContentsMargins(40, 20, 40, 0);
    auto* leftPortrait = new QLabel(&dlg);
    leftPortrait->setObjectName("vnPortrait");
    leftPortrait->setAttribute(Qt::WA_TranslucentBackground);
    leftPortrait->setAutoFillBackground(false);
    leftPortrait->setFixedSize(std::max(300, width() / 4), std::max(400, height() - 230));
    leftPortrait->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    auto* rightPortrait = new QLabel(&dlg);
    rightPortrait->setObjectName("vnPortrait");
    rightPortrait->setAttribute(Qt::WA_TranslucentBackground);
    rightPortrait->setAutoFillBackground(false);
    rightPortrait->setFixedSize(std::max(300, width() / 4), std::max(400, height() - 230));
    rightPortrait->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    stage->addWidget(leftPortrait, 0, Qt::AlignLeft | Qt::AlignBottom);
    stage->addStretch(1);
    stage->addWidget(rightPortrait, 0, Qt::AlignRight | Qt::AlignBottom);
    root->addWidget(stageWidget, 1);

    auto* box = new QFrame(&dlg);
    box->setObjectName("vnBox");
    box->setFixedHeight(std::clamp(height() / 4, 190, 250));
    auto* boxLayout = new QVBoxLayout(box);
    boxLayout->setContentsMargins(42, 14, 34, 16);
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

    StoryPortraitStage portraits(":/images/characters/portraits/variants/anon_calm.png");
    // Establish the actual interlocutor even when Anon speaks first.
    for (const auto& page : pages) {
        if (!page.speaker.contains(QString::fromUtf8("爱音")) && !page.portrait.isEmpty()) {
            portraits.observe(false, page.portrait.toStdString());
            break;
        }
    }
    StoryPager pager(static_cast<int>(pages.size()));
    const auto renderPage = [&]() {
        const VisualNovelPage& page = pages[static_cast<size_t>(pager.page())];
        name->setText(page.speaker);
        name->setStyleSheet(QStringLiteral("color: %1;").arg(page.accent));
        text->setText(page.text);
        pageCounter->setText(QStringLiteral("%1 / %2 · 点击画面或按任意键继续")
                                 .arg(pager.page() + 1).arg(pages.size()));

        const QPixmap explicitCg(page.cg);
        const bool hasExplicitCg = storyBackdropSource(!page.cg.isEmpty() && !explicitCg.isNull())
                                   == StoryBackdropSource::ExplicitCg;
        const QPixmap pageBackdrop = fillStoryCanvas(hasExplicitCg ? explicitCg : sceneSnapshot,
                                                     dlg.size(), !hasExplicitCg);
        backdrop->setPixmap(pageBackdrop);
        // 专属CG应完整成为画面主体；没有专属CG时以当前楼层场景为背景，
        // 并保留左右角色立绘组成对应的场景CG。
        stageWidget->setVisible(!hasExplicitCg);

        const bool playerSpeaking = page.speaker.contains(QString::fromUtf8("爱音"));
        portraits.observe(playerSpeaking, page.portrait.toStdString());
        const QString leftPath = dialoguePortraitPath(QString::fromStdString(portraits.left()));
        const QString rightPath = dialoguePortraitPath(QString::fromStdString(portraits.right()));
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
        setPortrait(rightPortrait, rightPath, !playerSpeaking && !page.portrait.isEmpty());
        GameAudio::playStoryVoice(page.speaker, page.text);
        next->setText(QString::fromUtf8("继续 ▶"));
    };

    const auto advance = [&]() {
        GameAudio::play(GameAudio::Cue::Dialogue);
        if (pager.advance()) {
            renderPage();
        } else {
            dlg.markCompleted();
            dlg.accept();
        }
    };
    dlg.advance = advance;
    connect(next, &QPushButton::clicked, &dlg, advance);
    for (QWidget* child : dlg.findChildren<QWidget*>())
        child->installEventFilter(&dlg);
    renderPage();
    backdrop->lower();
    dlg.move(mapToGlobal(QPoint(0, 0)));
    const bool completed = dlg.exec() == QDialog::Accepted && dlg.completed();
    GameAudio::stopStoryVoice();
    return completed;
}

std::vector<MainWindow::VisualNovelPage> MainWindow::storyPages(int scene) const
{
    std::vector<VisualNovelPage> pages;
    const QString cg = QString::fromUtf8(storySceneCg(scene));
    const StoryContext context{
        m_game->storyShown("anon_soyo_scene_2") || m_game->storyShown("anon_soyo_scene_5") ||
            m_game->storyShown("anon_soyo_scene_12") || m_game->storyShown("anon_soyo_scene_17") ||
            m_game->storyShown("anon_soyo_scene_18") || m_game->storyShown("anon_soyo_scene_23"),
        m_game->storyShown("anon_soyo_scene_17"),
        m_game->storyShown("anon_soyo_scene_18"),
        m_game->storyShown("anon_soyo_scene_23")
    };
    for (const StoryLine& line : storyScene(scene, context)) {
        const QString speaker = QString::fromUtf8(line.speaker);
        const QString portrait = QString::fromStdString(storyPortrait(line, scene));
        QString accent = QStringLiteral("#ffd66b");
        if (speaker.contains(QString::fromUtf8("爱音"))) {
            accent = QStringLiteral("#ff8fc7");
        } else if (speaker.contains(QString::fromUtf8("小长崎"))) {
            accent = QStringLiteral("#b58cff");
        } else if (speaker.contains(QString::fromUtf8("素世")) ||
                   speaker.contains(QString::fromUtf8("假魔王"))) {
            accent = QStringLiteral("#b58cff");
        } else if (speaker.contains(QString::fromUtf8("米歇尔"))) {
            accent = QStringLiteral("#ffb5d7");
        } else if (speaker.contains(QString::fromUtf8("海铃"))) {
            accent = QStringLiteral("#c7d8ff");
        } else if (speaker.contains(QString::fromUtf8("友希那"))) {
            accent = QStringLiteral("#b5c8ff");
        } else if (speaker.contains(QString::fromUtf8("香澄"))) {
            accent = QStringLiteral("#ff5f91");
        } else if (speaker.contains(QString::fromUtf8("心"))) {
            accent = QStringLiteral("#ffd66b");
        }
        pages.push_back({speaker, QString::fromUtf8(line.text), portrait, accent, cg});
    }
    return pages;
}

void MainWindow::showScriptSceneOnce(int scene)
{
    const std::string key = "anon_soyo_scene_" + std::to_string(scene);
    if (m_game->storyShown(key)) return;
    const auto pages = storyPages(scene);
    if (pages.empty()) return;
    m_game->markStoryShown(key);
    showVisualNovelDialogue(pages, true);
}

void MainWindow::showStoryPickup(int floor, const QString& name)
{
    if (floor == 5 && name == QString::fromUtf8("爱音拨片"))
        showScriptSceneOnce(7);
    else if (floor == 9 && name == QString::fromUtf8("素世谱架"))
        showScriptSceneOnce(8);
    else if (floor == 19 && name == QString::fromUtf8("MyGO和解徽章"))
        showScriptSceneOnce(13);
}

void MainWindow::showStoryMilestones()
{
    if (m_game->currentFloor() == 14 && m_game->floor14RedKeyRewardGranted())
        showScriptSceneOnce(11);
    if (m_game->currentFloor() == 34 && m_game->itemAt(3, 7))
        showScriptSceneOnce(22);
}

void MainWindow::showNotebookDialog()
{
    QDialog dlg(this);
    dlg.setObjectName(QStringLiteral("notebookDialog"));
    dlg.setWindowTitle(QString::fromUtf8("高松灯的单词本"));
    dlg.setModal(true);
    dlg.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setFixedSize(std::min(760, width() - 80), std::min(700, height() - 80));
    dlg.setStyleSheet(QStringLiteral(
        "QDialog { background:#18172b; border:2px solid #b3a0d5; color:#f4efff; }"
        "QLabel { color:#f4efff; }"
        "QFrame#notebookCard { background:#28253e; border:1px solid #62567c; border-radius:8px; }"
        "QPushButton { background:#6b4a89; color:white; padding:8px 20px; border-radius:6px; }"));
    auto* outer = new QVBoxLayout(&dlg);
    outer->setContentsMargins(28, 22, 28, 22);
    auto* title = new QLabel(QString::fromUtf8("高松灯的单词本 · 已记录 %1 条线索")
                                 .arg(m_game->notebookEntries().size()), &dlg);
    title->setStyleSheet(QStringLiteral("font-size:22px; font-weight:800; color:#dcb9ff;"));
    outer->addWidget(title);
    auto* scroll = new QScrollArea(&dlg);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* list = new QWidget(scroll);
    auto* rows = new QVBoxLayout(list);
    rows->setSpacing(12);
    if (m_game->notebookEntries().empty()) {
        auto* empty = new QLabel(QString::fromUtf8("还没有收集到线索。与塔里的 NPC 交谈吧。"), list);
        empty->setAlignment(Qt::AlignCenter);
        rows->addWidget(empty);
    }
    for (const NotebookEntry& entry : m_game->notebookEntries()) {
        auto* card = new QFrame(list);
        card->setObjectName(QStringLiteral("notebookCard"));
        auto* cardLayout = new QVBoxLayout(card);
        auto* heading = new QLabel(QString::fromUtf8("%1层 · %2")
                                       .arg(entry.floor).arg(QString::fromStdString(entry.source)), card);
        heading->setStyleSheet(QStringLiteral("font-weight:700; color:#ffd58c;"));
        auto* body = new QLabel(QString::fromStdString(entry.text), card);
        body->setObjectName(QStringLiteral("notebookEntryText"));
        body->setWordWrap(true);
        body->setTextInteractionFlags(Qt::TextSelectableByMouse);
        cardLayout->addWidget(heading);
        cardLayout->addWidget(body);
        rows->addWidget(card);
    }
    rows->addStretch();
    scroll->setWidget(list);
    outer->addWidget(scroll, 1);
    auto* close = new QPushButton(QString::fromUtf8("合上记事本"), &dlg);
    connect(close, &QPushButton::clicked, &dlg, &QDialog::accept);
    outer->addWidget(close, 0, Qt::AlignRight);
    dlg.move(mapToGlobal(QPoint((width() - dlg.width()) / 2,
                                (height() - dlg.height()) / 2)));
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

void MainWindow::showFirstFloorOpeningStory()
{
    static const std::string storyKey = "floor1_arrival_opening";
    if (!shouldShowFirstFloorOpening(m_playFirstFloorOpening, m_game->currentFloor(),
                                     m_game->storyShown(storyKey)))
        return;

    // 先落盘“一次性”标记，防止剧情播放期间触发存档或窗口重建后重复出现。
    m_game->markStoryShown(storyKey);
    m_playFirstFloorOpening = false;
    auto pages = storyPages(1);
    pages.push_back({QString::fromUtf8("操作提示 · 移动"),
         QString::fromUtf8("用方向键逐格移动，也可以点击与当前位置连通的地板、道具、怪物和门。点击怪物会在到达时战斗；持有对应钥匙时，点击门可直接开启。无法击败的怪物会先提示，不会强制开战。"),
         QString(), QStringLiteral("#9fd8ff")});
    pages.push_back({QString::fromUtf8("操作提示 · 探索"),
         QString::fromUtf8("拿到怪物手册后，左侧才会显示怪物与预计损失。获得灯的单词本后，点击右侧图标可重读线索；右下角可保存、读取、撤销。F5 即时存档，Ctrl+Z 可连续撤销。"),
         QString(), QStringLiteral("#9fd8ff")});
    pages.push_back({QString::fromUtf8("千早爱音"),
         QString::fromUtf8("好，先看清楚路。素世，你可别再往更高的地方跑了……第一层，出发！"),
         QStringLiteral(":/images/characters/portraits/variants/anon_confident.png"),
         QStringLiteral("#ff8fc7"), QString::fromUtf8(storySceneCg(1))});
    m_game->markStoryShown("anon_soyo_scene_1");
    showVisualNovelDialogue(pages, true);
}

void MainWindow::showOpeningPrisonStory()
{
    if (m_prisonReturnStoryShown || m_game->storyShown("floor3_prison_return")) return;
    m_prisonReturnStoryShown = true;
    m_game->markStoryShown("floor3_prison_return");
    showScriptSceneOnce(5);
}

void MainWindow::showFloor20VampireStoryIfNeeded(int floorBefore)
{
    if (floorBefore != 20 || m_game->currentFloor() != 20 ||
        m_game->tileAt(7, 10) != Tile_DoorMagic ||
        m_game->floor20VampireStoryShown())
        return;
    const Monster* vampire = m_game->monsterAt(7, 7);
    if (!vampire || !MonsterDB::hasIndex(vampire->GetName(), 15)) return;
    m_game->markFloor20VampireStoryShown();
    showScriptSceneOnce(14);
}

void MainWindow::showFloor33TrapStoryIfNeeded(int floorBefore)
{
    if (floorBefore != 33 || m_game->currentFloor() != 33 ||
        !m_game->floor33TrapTriggered() ||
        m_game->player().x != 11 || m_game->player().y != 6 ||
        m_floor33TrapStoryShown || m_game->storyShown("floor33_trap_dialogue"))
        return;
    m_floor33TrapStoryShown = true;
    m_game->markStoryShown("floor33_trap_dialogue");
    showScriptSceneOnce(21);
}

void MainWindow::showFloor32KnightStoryIfNeeded(int floorBefore)
{
    if (floorBefore != 32 || m_game->currentFloor() != 32 ||
        !m_game->floor32KnightStoryPending())
        return;
    runBossBattleAt(7, 10, true);
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

void MainWindow::showFloor10AmbushStoryIfNeeded()
{
    if (m_game->currentFloor() != 10 || !m_game->floor10AmbushTriggered() ||
        m_game->storyShown("floor10_ambush_cg"))
        return;
    m_game->markStoryShown("floor10_ambush_cg");
    showScriptSceneOnce(9);
}

void MainWindow::showFloor42CaptureStory()
{
    if (!m_game->floor42KnightStoryPending() ||
        m_game->storyShown("floor42_knight_capture"))
        return;
    m_game->markStoryShown("floor42_knight_capture");
    m_game->markStoryShown("floor_opening_42");
    showScriptSceneOnce(25);
    m_game->resolveFloor42KnightStory();
    ui.mapWidget->update();
    updateHUD();
}

void MainWindow::showPendingApproachHazardCgs()
{
    // 47 层的阻击巫师先完成后退动画，再展示本次邻接魔法的剧情。
    if (m_game->currentFloor() == 47 && ui.mapWidget->isMonsterMoving())
        return;
    const int mageFields = m_game->takePendingMageFieldEvents();
    const int guardFlanks = m_game->takePendingMagicGuardFlankEvents();
    if (mageFields <= 0 && guardFlanks <= 0) return;

    std::vector<VisualNovelPage> pages;
    for (int i = 0; i < mageFields; ++i) {
        pages.push_back({QString::fromUtf8("旁白"),
            QString::fromUtf8("巫师的邻接魔法阵在脚下闭合，魔力领域对爱音造成了伤害。"),
            QString(), QStringLiteral("#ffd66b"),
            QStringLiteral(":/images/runtime/cg/mage_field_encirclement.png")});
    }
    for (int i = 0; i < guardFlanks; ++i) {
        pages.push_back({QString::fromUtf8("旁白"),
            QString::fromUtf8("两名魔法警卫从相对方向同时发动夹击，爱音的生命值被削减一半。"),
            QString(), QStringLiteral("#ffd66b"),
            QStringLiteral(":/images/runtime/cg/magic_guard_flank.png")});
        pages.push_back({QString::fromUtf8("旁白"),
            QString::fromUtf8("警卫摘下面具——藏在盔甲之下的人正是藤都子。"),
            QString(), QStringLiteral("#ffd66b"),
            QStringLiteral(":/images/runtime/cg/miyako_guard_reveal.png")});
        pages.push_back({QString::fromUtf8("藤都子"),
            QString::fromUtf8("临兵斗者皆阵列在前！"),
            QString(), QStringLiteral("#b9a7ff"),
            QStringLiteral(":/images/runtime/cg/miyako_kuji_spell.png")});
    }
    showVisualNovelDialogue(pages);
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
        startMonsterMovementAnimation();
        if (ui.mapWidget->isMonsterMoving())
            m_floor42CaptureStoryQueued = true;
        else
            showFloor42CaptureStory();
        return;
    }
    // 二层初到时保持安静；信息由米歇尔第一次交谈给出。
    switch (toFloor) {
    case 3: showScriptSceneOnce(3); break;
    case 24: showScriptSceneOnce(15); break;
    case 49: showScriptSceneOnce(26); break;
    case 50:
        m_game->prepareFloor50MichelleReveal();
        showScriptSceneOnce(27);
        break;
    default: break;
    }
}

void MainWindow::showNPCDialog(int x, int y)
{
    NPC* npc = m_game->npcAt(x, y);
    if (!npc) return;

    Player& p = m_game->player();
    const QString npcDisplayName = npc->IsTrader() ? QString::fromUtf8("麻里奈") :
        npc->GetName() == "老头" ? QString::fromUtf8("凛凛子") :
        npc->GetName() == "小偷" ? QString::fromUtf8("米歇尔") :
        QString::fromStdString(npc->GetName());

    // NPC 对话使用与地图图块相同的角色头像，保持角色身份连续。
    QString npcPortrait;
    if (npc->IsTrader()) {
        npcPortrait = QStringLiteral(":/images/characters/portraits/marina.png");
    } else if (npc->GetName() == "小偷" || npc->GetName() == "米歇尔") {
        // 米歇尔在牢笼、离场和终幕分别使用不同动作，保持剧情状态连续。
        if (m_game->currentFloor() == 2 && x == 12 && y == 12)
            npcPortrait = QStringLiteral(":/images/characters/portraits/variants/michelle_caring.png");
        else if (!npc->HasGivenReward())
            npcPortrait = QStringLiteral(":/images/characters/portraits/variants/michelle_wave.png");
        else
            npcPortrait = QStringLiteral(":/images/characters/portraits/variants/michelle_confident.png");
    } else {
        npcPortrait = QStringLiteral(":/images/characters/portraits/ririko.png");
    }
    auto showNpcInfo = [&](const QString& title, const QString& text) {
        showVisualNovelDialogue({
            {title, text, npcPortrait, QStringLiteral("#ffb5d7")}
        });
    };
    const auto npcClue = [&]() {
        QStringList lines;
        for (const auto& line : npc->Dialog())
            lines.push_back(QString::fromStdString(line));
        return lines.join(QStringLiteral("\n")).trimmed();
    };
    const auto archiveNpcClueAndLeave = [&](const QString& clue) {
        if (clue.isEmpty()) return;
        const std::string key = "npc_" + std::to_string(m_game->currentFloor()) + "_" +
                                std::to_string(x) + "_" + std::to_string(y);
        m_game->recordNotebookClue(key, m_game->currentFloor(),
                                   npcDisplayName.toStdString(), clue.toStdString());
        m_game->dismissNpcAt(x, y);
        ui.mapWidget->update();
        updateHUD();
    };

    // 原版关键 NPC 事件（保留一次性状态）。
    const int classicId = npc->ClassicId();
    if (!npc->HasGivenReward() && classicId == 3) {
        npc->Interact(p); // 领取三层专属道具“怪物手册”
        showNpcInfo(QString::fromUtf8("怪物手册"),
            QString::fromUtf8("这本怪物手册交给你。\n它能查看本层怪物的能力。"));
        archiveNpcClueAndLeave(QString::fromUtf8("怪物手册能查看本层怪物的能力与预计战斗损失。"));
        updateHUD();
        return;
    }
    if (!npc->HasGivenReward() && classicId == 33) {
        if (!showVisualNovelChoice(npcDisplayName,
            QString::fromUtf8("确认接受攻防各提升 3% 吗？确认后才会生效。"),
            npcPortrait, QString::fromUtf8("确认提升"), QString::fromUtf8("暂不接受")))
            return;
        captureUndoSnapshot();
        p.atk = (p.atk * 103 + 99) / 100;
        p.def = (p.def * 103 + 99) / 100;
        npc->SetGiven(true);
        showNpcInfo(npcDisplayName, QString::fromUtf8("你的攻击力和防御力提升了 3%！"));
        archiveNpcClueAndLeave(QString::fromUtf8("攻击力和防御力都提升了3%；后续可用怪物手册核对战斗损失。"));
        updateHUD();
        return;
    }
    if (!npc->HasGivenReward() && classicId == 22 && m_game->currentFloor() == 26) {
        // 原版26层并不存在真正的公主：这里是洋娃娃。完成对话后，
        // 24层红门上方显现直通50层的隐藏楼梯，而不是直接传送。
        npc->SetGiven(true);
        m_game->unlockPrincessDollPassage();
        showScriptSceneOnce(16);
        archiveNpcClueAndLeave(QString::fromUtf8("26层公主其实是洋娃娃；完成对话后，24层红门后的隐藏通道会打开。"));
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
            const QString clue = QString::fromUtf8("暗墙的颜色比普通墙浅；这类隐藏路线不消耗钥匙。");
            showNpcInfo(QString::fromUtf8("麻里奈"), clue);
            archiveNpcClueAndLeave(clue);
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
            const QString clue = QString::fromUtf8("属性交换有风险；进入高层前请核对生命、攻击与防御。" );
            showNpcInfo(QString::fromUtf8("麻里奈"), clue);
            archiveNpcClueAndLeave(clue);
        }
        updateHUD();
        return;
    }
    if (!npc->HasGivenReward() && classicId == 26 && m_game->currentFloor() == 35) {
        // 35层只显露暗道；米歇尔的身份留到50层由她主动揭晓。
        npc->SetGiven(true);
        showScriptSceneOnce(23);
        m_game->recordNotebookClue("npc_35_michelle", 35, "米歇尔",
                                   "35层的实墙已显露为暗墙，仍需逐格撞开，不消耗钥匙；米歇尔将前往50层终幕。");
        m_game->completeFloor35MichelleStory();
        ui.mapWidget->update();
        updateHUD();
        return;
    }
    if (!npc->HasGivenReward() && classicId == 47 && m_game->currentFloor() == 50) {
        // 终幕揭开伪装：50层的米歇尔其实是长崎素世。
        npc->SetGiven(true);
        showScriptSceneOnce(28);
        m_game->recordNotebookClue("npc_50_reveal", 50, "长崎素世",
                                   "米歇尔就是长崎素世；她在50层揭开了伪装。");
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
            showScriptSceneOnce(18);
            m_game->recordNotebookClue("npc_2_cage_michelle", 2, "米歇尔",
                                       "米歇尔已经脱离2层牢笼，前往35层打开魔龙房间的暗墙。");
            m_game->dismissNpcAt(x, y);
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
            showScriptSceneOnce(2);
            m_game->recordNotebookClue("npc_2_4_8", 2, "米歇尔",
                                       "铁剑在5层，铁盾在9层。先去把它们找回来。");

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
            showScriptSceneOnce(12);
            m_game->recordNotebookClue("npc_15_michelle", 15, "米歇尔",
                                       "击败15层大章鱼后暗墙已经打开；米歇尔会去29层等候。");
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

            showScriptSceneOnce(17);
            m_game->recordNotebookClue("npc_29_michelle", 29, "米歇尔",
                                       "29层下方暗道已打开；米歇尔回到2层右下角牢笼，救出后去35层。");

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
            showNpcInfo(npcDisplayName,
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
        const bool accepted = showVisualNovelChoice(npcDisplayName, info,
            npcPortrait, canAfford ? QString::fromUtf8("完成交易") : QString::fromUtf8("金币不足"));

        if (accepted && canAfford) {
            p.gold -= npc->GetTradeGoldCost();
            if (tradeReward) {
                // NPC 给出的可使用/被动道具必须进入道具栏；即时属性奖励才直接结算。
                auto item = Game::createItemByName(tradeReward->GetName(), tradeReward->GetValue());
                if (item && item->IsUseItem()) {
                    p.AddItem(std::move(item));
                } else if (item && item->IsPassiveEffect()) {
                    item->Apply(p);
                    p.AddItem(std::move(item));
                } else if (item) {
                    item->Apply(p);
                }
            }
            npc->SetTradeDone(true);

            showNpcInfo(npcDisplayName,
                QString::fromUtf8("交易成功！获得了 %1。").arg(rewardDesc));
            const QString clue = npcClue().isEmpty()
                ? QString::fromUtf8("固定交易只可完成一次，之后可在灯的单词本里回看线索。")
                : npcClue();
            showNpcInfo(QString::fromUtf8("麻里奈"), clue);
            archiveNpcClueAndLeave(clue);
        } else if (accepted) {
            showNpcInfo(npcDisplayName,
                QString::fromUtf8("金币不够，交易暂时无法完成。"));
        }

        ui.mapWidget->update();
        updateHUD();
        return;
    }

    // 普通NPC（非交易或交易已完成）
    const std::string npcStoryKey = "npc_dialogue_" + std::to_string(m_game->currentFloor())
        + "_" + std::to_string(x) + "_" + std::to_string(y);
    // 兼容旧存档：若对白标记已经写入、但奖励尚未真正领取，仍允许再次交互补发。
    if (m_game->storyShown(npcStoryKey) && !npc->HasPendingReward()) {
        archiveNpcClueAndLeave(npcClue());
        return;
    }
    bool hadReward = !npc->HasGivenReward();
    std::string reply = npc->Interact(p);
    std::vector<VisualNovelPage> pages;
    if (hadReward && npc->HasGivenReward())
        pages.push_back({npcDisplayName,
                         QString::fromStdString(reply), npcPortrait, QStringLiteral("#ffb5d7")});
    for (const auto& line : npc->Dialog())
        pages.push_back({npcDisplayName,
                         QString::fromStdString(line), npcPortrait, QStringLiteral("#ffb5d7")});
    if (pages.empty())
        pages.push_back({npcDisplayName,
                         QString::fromStdString(reply), npcPortrait, QStringLiteral("#ffb5d7")});
    showVisualNovelDialogue(pages);
    m_game->markStoryShown(npcStoryKey);
    archiveNpcClueAndLeave(npcClue());
}

void MainWindow::showShopDialog(int x, int y)
{
    ShopData* shop = m_game->shopAt(x, y);
    if (!shop) return;

    if (m_game->currentFloor() == 4)
        showScriptSceneOnce(6);

    Player& p = m_game->player();

    // 4/12/32/46 层是可重复购买的原版属性商店，必须优先于一次性兑换商人判断。
    if (shop->classicShopFloor > 0 && shop->classicShopFloor != 28) {
        const ClassicShopOffer offer = classicShopOfferForFloor(shop->classicShopFloor, p.shopUseCount);
        const ClassicShopPresentation view = makeClassicShopPresentation(p, offer);
        QDialog dlg(this);
        dlg.setObjectName(QStringLiteral("classicShopDialog"));
        dlg.setWindowTitle(QString::fromUtf8("属性商店"));
        dlg.setFixedSize(820, 560);
        dlg.setStyleSheet(QString::fromUtf8(R"(
            QDialog#classicShopDialog {
                background-color: #101326;
                background-image: url(:/images/runtime/ui/panel_texture.png);
                color: #f7f1ff;
            }
            QLabel { color: #f7f1ff; background: transparent; }
            QWidget#shopKeeperPanel {
                background: rgba(22, 24, 48, 226);
                border: 1px solid #5d5a84;
                border-radius: 16px;
            }
            QLabel#shopBrand { color: #ffd86b; font-size: 21px; font-weight: 900; }
            QLabel#shopKeeperName { color: #ff9dcc; font-size: 18px; font-weight: 800; }
            QLabel#shopGreeting { color: #d7d3e8; font-size: 13px; }
            QLabel#shopTitle { color: #ffffff; font-size: 24px; font-weight: 900; }
            QLabel#shopSubtitle { color: #aaa7c6; font-size: 12px; }
            QLabel#shopPriceBadge {
                color: #251b08;
                background: #ffd86b;
                border: 2px solid #fff0a5;
                border-radius: 12px;
                padding: 8px 16px;
                font-size: 18px;
                font-weight: 900;
            }
            QLabel#shopBalance { color: #eee9ff; font-size: 14px; font-weight: 700; }
            QLabel#shopBalance[affordable="false"] { color: #ff829c; }
            QWidget[class="shopOfferCard"] {
                background: rgba(28, 31, 60, 235);
                border: 1px solid #67668e;
                border-radius: 14px;
            }
            QLabel[class="shopOfferIcon"] { font-size: 28px; font-weight: 900; }
            QLabel[class="shopOfferTitle"] { color: #ffffff; font-size: 17px; font-weight: 900; }
            QLabel[class="shopOfferGain"] { font-size: 22px; font-weight: 900; }
            QLabel[class="shopOfferStats"] { color: #bdb9d5; font-size: 13px; }
            QPushButton[class="shopBuyButton"] {
                color: white;
                background: #6f55bc;
                border: 1px solid #bba2ff;
                border-radius: 9px;
                padding: 9px 10px;
                font-size: 14px;
                font-weight: 800;
                min-height: 24px;
            }
            QPushButton[class="shopBuyButton"]:hover { background: #876ad6; }
            QPushButton[class="shopBuyButton"]:pressed { background: #59439b; }
            QPushButton[class="shopBuyButton"]:disabled {
                color: #77758a;
                background: #282a40;
                border-color: #45465c;
            }
            QPushButton#shopLeaveButton {
                color: #d8d5e9;
                background: transparent;
                border: 1px solid #5f607a;
                border-radius: 8px;
                padding: 7px 18px;
                min-height: 22px;
            }
            QPushButton#shopLeaveButton:hover { color: white; border-color: #aaa8c8; }
        )"));

        auto* outer = new QHBoxLayout(&dlg);
        outer->setContentsMargins(20, 20, 20, 20);
        outer->setSpacing(18);

        auto* keeperPanel = new QWidget(&dlg);
        keeperPanel->setObjectName(QStringLiteral("shopKeeperPanel"));
        keeperPanel->setFixedWidth(230);
        auto* keeperLayout = new QVBoxLayout(keeperPanel);
        keeperLayout->setContentsMargins(16, 16, 16, 16);
        keeperLayout->setSpacing(8);
        auto* brand = new QLabel(QStringLiteral("HAPPY SHOP"), keeperPanel);
        brand->setObjectName(QStringLiteral("shopBrand"));
        brand->setAlignment(Qt::AlignCenter);
        keeperLayout->addWidget(brand);
        auto* kokoro = new QLabel(keeperPanel);
        kokoro->setAlignment(Qt::AlignCenter);
        kokoro->setPixmap(QPixmap(QStringLiteral(":/images/characters/portraits/kokoro_shop.png"))
                              .scaled(190, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        keeperLayout->addWidget(kokoro, 1);
        auto* keeperName = new QLabel(QString::fromUtf8("弦卷心 · 店员"), keeperPanel);
        keeperName->setObjectName(QStringLiteral("shopKeeperName"));
        keeperName->setAlignment(Qt::AlignCenter);
        keeperLayout->addWidget(keeperName);
        auto* greeting = new QLabel(QString::fromUtf8("今天也要笑着变强哦！\n选择一项现场应援。"), keeperPanel);
        greeting->setObjectName(QStringLiteral("shopGreeting"));
        greeting->setAlignment(Qt::AlignCenter);
        greeting->setWordWrap(true);
        keeperLayout->addWidget(greeting);
        outer->addWidget(keeperPanel);

        auto* content = new QVBoxLayout();
        content->setSpacing(12);
        auto* title = new QLabel(QString::fromUtf8("现场属性应援"), &dlg);
        title->setObjectName(QStringLiteral("shopTitle"));
        content->addWidget(title);
        auto* subtitle = new QLabel(QString::fromUtf8("全塔商店共享购买次数 · 每次只能选择一项"), &dlg);
        subtitle->setObjectName(QStringLiteral("shopSubtitle"));
        content->addWidget(subtitle);

        auto* summaryRow = new QHBoxLayout();
        auto* priceBadge = new QLabel(QString::fromUtf8("第 %1 次  ·  %2 金币")
            .arg(view.purchaseNumber).arg(view.price), &dlg);
        priceBadge->setObjectName(QStringLiteral("shopPriceBadge"));
        summaryRow->addWidget(priceBadge);
        summaryRow->addStretch();
        const QString balanceText = view.affordable
            ? QString::fromUtf8("持有 %1 金币 · 可以购买").arg(view.gold)
            : QString::fromUtf8("持有 %1 金币 · 还差 %2").arg(view.gold).arg(view.missingGold);
        auto* balance = new QLabel(balanceText, &dlg);
        balance->setObjectName(QStringLiteral("shopBalance"));
        balance->setProperty("affordable", view.affordable);
        summaryRow->addWidget(balance);
        content->addLayout(summaryRow);

        struct Offer { QString id; QString icon; QString name; QString accent; std::function<void()> apply; };
        const Offer offers[] = {
            {QStringLiteral("hp"), QString::fromUtf8("♥"), QString::fromUtf8("生命值"), QStringLiteral("#ff7da8"), [&]{ p.hp += offer.hp; }},
            {QStringLiteral("atk"), QString::fromUtf8("♫"), QString::fromUtf8("攻击力"), QStringLiteral("#ffc85c"), [&]{ p.atk += offer.atk; }},
            {QStringLiteral("def"), QString::fromUtf8("◆"), QString::fromUtf8("防御力"), QStringLiteral("#70c9ff"), [&]{ p.def += offer.def; }}
        };

        auto* cardRow = new QHBoxLayout();
        cardRow->setSpacing(10);
        for (int i = 0; i < 3; ++i) {
            const auto& item = offers[i];
            const auto& cardView = view.cards[static_cast<std::size_t>(i)];
            auto* card = new QWidget(&dlg);
            card->setProperty("class", QStringLiteral("shopOfferCard"));
            card->setObjectName(QStringLiteral("shopOffer_%1").arg(item.id));
            auto* cardLayout = new QVBoxLayout(card);
            cardLayout->setContentsMargins(13, 14, 13, 13);
            cardLayout->setSpacing(7);
            auto* icon = new QLabel(item.icon, card);
            icon->setProperty("class", QStringLiteral("shopOfferIcon"));
            icon->setStyleSheet(QStringLiteral("color: %1;").arg(item.accent));
            icon->setAlignment(Qt::AlignCenter);
            cardLayout->addWidget(icon);
            auto* name = new QLabel(item.name, card);
            name->setProperty("class", QStringLiteral("shopOfferTitle"));
            name->setAlignment(Qt::AlignCenter);
            cardLayout->addWidget(name);
            auto* gain = new QLabel(QStringLiteral("+%1").arg(cardView.increase), card);
            gain->setProperty("class", QStringLiteral("shopOfferGain"));
            gain->setStyleSheet(QStringLiteral("color: %1;").arg(item.accent));
            gain->setAlignment(Qt::AlignCenter);
            cardLayout->addWidget(gain);
            auto* stats = new QLabel(QString::fromUtf8("当前 %1\n购买后 %2")
                .arg(cardView.beforeValue).arg(cardView.afterValue), card);
            stats->setProperty("class", QStringLiteral("shopOfferStats"));
            stats->setAlignment(Qt::AlignCenter);
            cardLayout->addWidget(stats);
            cardLayout->addStretch();
            auto* button = new QPushButton(QString::fromUtf8("购买 · %1 金币").arg(view.price), card);
            button->setObjectName(QStringLiteral("shopBuy_%1").arg(item.id));
            button->setProperty("class", QStringLiteral("shopBuyButton"));
            button->setEnabled(view.affordable);
            if (!view.affordable)
                button->setToolTip(QString::fromUtf8("金币不足，还差 %1").arg(view.missingGold));
            QObject::connect(button, &QPushButton::clicked, &dlg, [this, &dlg, &p, offer, item] {
                captureUndoSnapshot();
                p.gold -= offer.price;
                item.apply();
                ++p.shopUseCount;
                GameAudio::play(GameAudio::Cue::Pickup);
                dlg.accept();
            });
            cardLayout->addWidget(button);
            cardRow->addWidget(card, 1);
        }
        content->addLayout(cardRow, 1);

        auto* leave = new QPushButton(QString::fromUtf8("离开"), &dlg);
        leave->setObjectName(QStringLiteral("shopLeaveButton"));
        leave->setFixedWidth(110);
        QObject::connect(leave, &QPushButton::clicked, &dlg, &QDialog::reject);
        auto* footer = new QHBoxLayout();
        footer->addStretch();
        footer->addWidget(leave);
        content->addLayout(footer);
        outer->addLayout(content, 1);

        dlg.exec();
        updateHUD();
        return;
    }

    // 原版固定兑换商人：钥匙数量与价格保持 50 层魔塔配置。
    const int classicId = shop->classicNpcId;
    const QString shopPortrait = QStringLiteral(":/images/characters/portraits/marina.png");
    const auto finishOneTimeMerchant = [&]() {
        const QString clue = oneTimeMerchantClue(classicId);
        showVisualNovelDialogue({
            {QString::fromUtf8("麻里奈"), clue, shopPortrait, QStringLiteral("#ffb5d7")}
        });
        const std::string key = "shop_" + std::to_string(m_game->currentFloor()) + "_" +
                                std::to_string(x) + "_" + std::to_string(y);
        m_game->recordNotebookClue(key, m_game->currentFloor(), "麻里奈", clue.toStdString());
        m_game->dismissShopAt(x, y);
        ui.mapWidget->update();
        updateHUD();
    };
    if (classicId == 24) {
        // 28层商人是收购商：黄色钥匙可无限次出售，每把100金币。
        const int yellowKeys = p.KeyCount(KeyType::Green);
        if (yellowKeys <= 0) {
            showVisualNovelDialogue({
                {QString::fromUtf8("麻里奈"),
                 QString::fromUtf8("带黄色Live票来，我会按每把100金币无限收购。"),
                 shopPortrait, QStringLiteral("#ffb5d7")}
            });
            updateHUD();
            return;
        }
        const bool accepted = showVisualNovelChoice(QString::fromUtf8("麻里奈"),
            QString::fromUtf8("出售黄色Live票 ×1\n获得：100 金币\n当前黄色Live票：%1\n无上限")
                .arg(yellowKeys), shopPortrait, QString::fromUtf8("出售"));
        if (accepted) {
            captureUndoSnapshot();
            p.UseKey(KeyType::Green);
            p.gold += 100;
            showVisualNovelDialogue({
                {QString::fromUtf8("麻里奈"),
                 QString::fromUtf8("收购完成，获得100金币。下次还可以继续出售。"),
                 shopPortrait, QStringLiteral("#ffb5d7")}
            });
        }
        updateHUD();
        return;
    }
    if (shop->classicPurchaseCount > 0) {
        finishOneTimeMerchant();
        updateHUD();
        return;
    }
    showVisualNovelDialogue({
        {QString::fromUtf8("麻里奈"),
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
        const bool accepted = showVisualNovelChoice(QString::fromUtf8("麻里奈"),
            QString::fromUtf8("%1\n价格：%2 金币\n当前金币：%3\n每个摊位只能购买一次。")
                .arg(offer.text).arg(offer.cost).arg(p.gold),
            shopPortrait, canAfford ? QString::fromUtf8("购买") : QString::fromUtf8("金币不足"));
        if (accepted && canAfford) {
            captureUndoSnapshot();
            p.gold -= offer.cost;
            offer.grant();
            shop->classicPurchaseCount = 1;
            showVisualNovelDialogue({
                {QString::fromUtf8("麻里奈"),
                 QString::fromUtf8("交易完成！这是你的 %1。这个摊位不会再次出售。")
                     .arg(offer.text), shopPortrait, QStringLiteral("#ffb5d7")}
            });
            finishOneTimeMerchant();
        } else if (accepted) {
            showVisualNovelDialogue({
                {QString::fromUtf8("麻里奈"), QString::fromUtf8("金币不够，等你准备好再来吧。"),
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
            {QString::fromUtf8("麻里奈"),
             QString::fromUtf8("购买了 %1！%2\n本摊位交易已完成。")
                 .arg(purchasedName).arg(purchasedEffect),
             shopPortrait, QStringLiteral("#ffb5d7")}
        });
        finishOneTimeMerchant();
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
    const bool handbookUnlocked = m_game && m_game->player().HasMonsterBook();
    ui.monsterScroll->setVisible(handbookUnlocked);
    if (!handbookUnlocked)
        return;

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

    // 同一楼层同名怪物只在手册中保留一种，避免重复条目挤占侧栏。
    const auto monsters = m_game->uniqueMonsterTypesOnCurrentFloor();
    if (monsters.empty()) {
        auto* emptyLabel = new QLabel(QString::fromUtf8("本层无怪物"), ui.monsterPanel);
        emptyLabel->setStyleSheet("color: #666; font-size: 13px; padding: 12px;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        // 插入到 stretch 之前
        ui.monsterLayout->insertWidget(ui.monsterLayout->count() - 1, emptyLabel);
        return;
    }

    for (const Monster& mon : monsters) {
        const int sameTypeCount = static_cast<int>(std::count_if(
            m_game->currentFloorData().monsters.begin(),
            m_game->currentFloorData().monsters.end(),
            [&mon](const auto& entry) { return entry.second.GetName() == mon.GetName(); }));

        auto* row = new QWidget(ui.monsterPanel);
        row->setStyleSheet("background-color: #1e1e36; border-radius: 4px;");
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(4, 4, 4, 4);
        rowLayout->setSpacing(8);

        // 怪物小图
        auto* imgLabel = new QLabel(row);
        // Use the same high-resolution runtime sprite as the map.  The 60x60
        // presentation matches one map tile while the 240x240 source keeps
        // faces and costume details readable in the handbook.
        imgLabel->setFixedSize(64, 64);
        imgLabel->setAlignment(Qt::AlignCenter);
        imgLabel->setStyleSheet("border: 1px solid #665f78; border-radius: 4px; background: rgba(24,25,43,210);");

        const QString imgPath = resolvedMonsterVisualPath(mon.GetName());
        QPixmap px(imgPath);
        if (!px.isNull()) {
            imgLabel->setPixmap(px.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        rowLayout->addWidget(imgLabel);

        // 怪物信息
        QString info = QString::fromUtf8(
            "<b style='color:#f1cf7a;'>%1</b><br>"
            "<span style='color:#ff8b9d;'>生命 %2</span>  "
            "<span style='color:#ffb86b;'>攻击 %3</span><br>"
            "<span style='color:#9fc5ff;'>防御 %4</span>  "
            "<span style='color:#b8df9b;'>金币 %5</span><br>"
            "<span style='color:#8e91ab; font-size:10px;'>本层数量 %6</span>")
            .arg(QString::fromStdString(mon.GetName()))
            .arg(formatNumber(mon.GetHP())).arg(formatNumber(mon.GetATK()))
            .arg(formatNumber(mon.GetDEF())).arg(formatNumber(mon.GetGold()))
            .arg(sameTypeCount);

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
            "Mujica镜面舞台票", "Mujica舞台震响卷"
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
        "乐奈幸运硬币", "舞台升降卡", "撤场通行卡", "Mujica镜面舞台票",
        "Mujica舞台震响卷"
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

    // 移动时每一格都要更新数值，但没有拾取道具或怪物变化时，不要把
    // 两侧的整块 QWidget 列表销毁重建；这会阻塞下一格动画的起帧。
    const bool gameChanged = m_hudPanelGame != m_game;
    m_hudPanelGame = m_game;
    std::vector<std::string> inventoryNames;
    inventoryNames.reserve(m_game->player().InventoryCount());
    for (int i = 0; i < m_game->player().InventoryCount(); ++i) {
        const Item* item = m_game->player().GetItem(i);
        inventoryNames.push_back(item ? Game::canonicalItemName(item->GetName()) : std::string());
    }
    const bool inventoryChanged = gameChanged || inventoryNames != m_hudInventoryNames;
    if (inventoryChanged) m_hudInventoryNames = std::move(inventoryNames);

    const bool handbookUnlocked = m_game->player().HasMonsterBook();
    std::vector<std::pair<std::string, int>> monsterRoster;
    if (handbookUnlocked) {
        for (const Monster& monster : m_game->uniqueMonsterTypesOnCurrentFloor()) {
            const auto count = std::count_if(m_game->currentFloorData().monsters.begin(),
                                             m_game->currentFloorData().monsters.end(),
                [&monster](const auto& entry) {
                    return entry.second.GetName() == monster.GetName();
                });
            monsterRoster.emplace_back(monster.GetName(), static_cast<int>(count));
        }
    }
    const bool monstersChanged = gameChanged || floor != m_hudMonsterFloor ||
        handbookUnlocked != m_hudMonsterBookUnlocked || monsterRoster != m_hudMonsterRoster;
    if (monstersChanged) {
        m_hudMonsterFloor = floor;
        m_hudMonsterBookUnlocked = handbookUnlocked;
        m_hudMonsterRoster = std::move(monsterRoster);
    }

    // 道具图标栏位于属性区域下方；旧文字栏位保留兼容但不再占用界面空间。
    ui.invItemsLabel->setVisible(false);
    if (inventoryChanged) updateItemPanel();

    if (monstersChanged) updateMonsterPanel();
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

    // Qt/Windows 的自动重复有明显首键延迟。物理按下状态由游戏自己
    // 维护，格子动画结束后即可接续，不再等待系统发送 repeat 事件。
    if (event->isAutoRepeat()) return;
    m_heldMoveState.press(dx, dy);

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

    m_hasPendingMove = false;

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

    const int playerXBefore = m_game->player().x;
    const int playerYBefore = m_game->player().y;
    int nx = playerXBefore + dx;
    int ny = playerYBefore + dy;
    ui.mapWidget->setPlayerDirection(dx, dy);
    const int floorBefore = m_game->currentFloor();
    const int targetTileBefore = m_game->tileAt(nx, ny);
    const Item* steppedItem = m_game->itemAt(nx, ny);
    const QString pickedName = steppedItem
        ? QString::fromStdString(Game::canonicalItemName(steppedItem->GetName()))
        : QString();
    const QString pickedDescription = steppedItem ? getItemDescription(steppedItem) : QString();
    // 在执行移动/战斗前保存一步撤销点，确保战斗、拾取和楼梯切换都能回退。
    captureUndoSnapshot();
    auto result = m_game->tryMovePlayer(nx, ny);
    const bool movedOneTile = m_game->currentFloor() == floorBefore &&
        std::abs(m_game->player().x - playerXBefore) +
        std::abs(m_game->player().y - playerYBefore) == 1;
    if (movedOneTile &&
        (result == Game::Move_Ok || result == Game::Move_Pickup)) {
        int heldDx = 0;
        int heldDy = 0;
        if (m_heldMoveState.current(heldDx, heldDy)) {
            m_pendingMoveDx = heldDx;
            m_pendingMoveDy = heldDy;
            m_hasPendingMove = true;
        }
    }
    startMonsterMovementAnimation();
    showPendingApproachHazardCgs();

    switch (result) {
    case Game::Move_Block:
        break;
    case Game::Move_DoorLocked: {
        GameAudio::play(GameAudio::Cue::Blocked);
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
        if (targetTileBefore == Tile_DoorRed || targetTileBefore == Tile_DoorBlue ||
            targetTileBefore == Tile_DoorGreen || targetTileBefore == Tile_DoorMagic ||
            targetTileBefore == Tile_DoorIron || targetTileBefore == Tile_DarkWall)
            GameAudio::play(GameAudio::Cue::Door);
        else
            GameAudio::play(GameAudio::Cue::Step);
        if (m_game->floor3PrisonStoryPending())
            showPrisonTrapPrompt();
        else if (floorBefore == 3 && m_game->currentFloor() == 2)
            showOpeningPrisonStory();
        showFloor10AmbushStoryIfNeeded();
        showFloor20VampireStoryIfNeeded(floorBefore);
        showFloor33TrapStoryIfNeeded(floorBefore);
        showFloor32KnightStoryAfterMovement(floorBefore);
        showStoryMilestones();
        ui.mapWidget->update();
        updateHUD();
        break;
    case Game::Move_Pickup:
        GameAudio::play(GameAudio::Cue::Pickup);
        if (!pickedName.isEmpty())
            showBattleFeedback(QString::fromUtf8("获得 %1：%2").arg(pickedName).arg(
                QString(pickedDescription).replace(QString::fromUtf8("（未生效）"), QString::fromUtf8("（已生效）"))));
        showStoryPickup(floorBefore, pickedName);
        ui.mapWidget->update();
        updateHUD();
        break;
    case Game::Move_Encounter: {
        Monster* m = m_game->monsterAt(nx, ny);
        if (!m) break;
        if (!m_game->canDefeatMonsterAt(nx, ny)) {
            GameAudio::play(GameAudio::Cue::Blocked);
            showBattleFeedback(QString::fromUtf8("你无法击败对方。请先提升属性或恢复生命。"));
            break;
        }
        if (runBossBattleAt(nx, ny)) break;
        GameAudio::play(GameAudio::Cue::Battle);
        const bool knightFight = floorBefore == 32 &&
            MonsterDB::hasIndex(m->GetName(), 24);

        QString battlePrefix;
        if (m_game->player().hasGlasses) {
            battlePrefix = QString::fromUtf8("%1  HP:%2 ATK:%3 DEF:%4  ")
                .arg(QString::fromStdString(m->GetName()))
                .arg(m->GetHP()).arg(m->GetATK()).arg(m->GetDEF());
        }
        ui.mapWidget->update();
        std::vector<std::string> log;
        auto fightRes = m_game->fightAt(nx, ny, log);

        QString dlg = QString::fromStdString(summarizeBattleLog(log));
        if (!battlePrefix.isEmpty()) dlg = battlePrefix + "\n" + dlg;

        if (fightRes == Game::Fight_GameWin) {
            GameAudio::play(GameAudio::Cue::Victory);
            // 即使是最终战，也要先提交战斗产生的脚本移动队列。
            startMonsterMovementAnimation();
            ui.mapWidget->update();
            updateHUD();
            showBattleFeedback(dlg);
            gameWin();
            return;
        } else if (fightRes == Game::Fight_PlayerWin) {
            GameAudio::play(GameAudio::Cue::Victory);
            ui.mapWidget->update();
            updateHUD();
            showBattleFeedback(dlg);
            if (knightFight)
                showScriptSceneOnce(20);
            showStoryMilestones();
            startMonsterMovementAnimation();
            int heldDx = 0;
            int heldDy = 0;
            if (m_heldMoveState.current(heldDx, heldDy)) {
                m_pendingMoveDx = heldDx;
                m_pendingMoveDy = heldDy;
                m_hasPendingMove = true;
            }
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
        GameAudio::play(GameAudio::Cue::Dialogue);
        showNPCDialog(nx, ny);
        updateHUD();
        break;
    }
    case Game::Move_Shop:
        GameAudio::play(GameAudio::Cue::Shop);
        showShopDialog(nx, ny);
        break;
        case Game::Move_StairsUp:
            GameAudio::play(GameAudio::Cue::Stairs);
            m_game->goUpFloor(nx, ny);
            showOpeningFloorStory(floorBefore, m_game->currentFloor());
            ui.mapWidget->update();
            updateHUD();
            break;
        case Game::Move_StairsDown:
            GameAudio::play(GameAudio::Cue::Stairs);
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

void MainWindow::restartGame()
{
    if (m_ownsGame) delete m_game;
    m_game = new Game();
    m_ownsGame = true;
    m_game->loadDefaultMap();
    m_hudPanelGame = nullptr;
    ui.mapWidget->setGame(m_game);
    ui.mapWidget->update();
    updateHUD();
    setFocus();
    m_playFirstFloorOpening = true;
    QTimer::singleShot(0, this, [this]() { showFirstFloorOpeningStory(); });
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    int dx = 0;
    int dy = 0;
    switch (event->key()) {
    case Qt::Key_Left:  dx = -1; break;
    case Qt::Key_Right: dx =  1; break;
    case Qt::Key_Up:    dy = -1; break;
    case Qt::Key_Down:  dy =  1; break;
    default:
        QWidget::keyReleaseEvent(event);
        return;
    }
    if (event->isAutoRepeat()) return;

    m_heldMoveState.release(dx, dy);
    int heldDx = 0;
    int heldDy = 0;
    if (m_heldMoveState.current(heldDx, heldDy)) {
        m_pendingMoveDx = heldDx;
        m_pendingMoveDy = heldDy;
        m_hasPendingMove = true;
    } else {
        m_hasPendingMove = false;
    }
}

void MainWindow::gameOver()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(QString::fromUtf8("游戏结束"));
    msgBox.setText(QString::fromUtf8("你被击败了！\n\n游戏结束。"));
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStyleSheet(endDialogStyle());

    QPushButton* restartBtn = msgBox.addButton(QString::fromUtf8("重新开始"), QMessageBox::ActionRole);
    msgBox.addButton(QString::fromUtf8("返回主菜单"), QMessageBox::RejectRole);
    msgBox.setDefaultButton(restartBtn);

    applyRuntimeArtSkin(msgBox);
    msgBox.exec();

    if (msgBox.clickedButton() == restartBtn) {
        restartGame();
    } else {
        // 返回主菜单：关闭当前窗口，MenuWindow 会自动显示
        close();
    }
}

void MainWindow::gameWin()
{
    showScriptSceneOnce(30);
    showScriptSceneOnce(31);
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(QString::fromUtf8("游戏通关"));
    msgBox.setText(QString::fromUtf8("终幕结束。长崎素世放下了魔杖，爱音向她伸出了手。\n\n游戏通关！"));
    const QPixmap endingCg(QStringLiteral(":/images/runtime/cg/soyo_defeated_ending.png"));
    if (!endingCg.isNull())
        msgBox.setIconPixmap(endingCg.scaled(880, 495, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    else
        msgBox.setIcon(QMessageBox::Information);
    msgBox.setStyleSheet(endDialogStyle());

    QPushButton* restartBtn = msgBox.addButton(QString::fromUtf8("重新开始"), QMessageBox::ActionRole);
    QPushButton* menuBtn    = msgBox.addButton(QString::fromUtf8("返回主菜单"), QMessageBox::RejectRole);
    msgBox.setDefaultButton(menuBtn);

    applyRuntimeArtSkin(msgBox);
    msgBox.exec();

    if (msgBox.clickedButton() == restartBtn) {
        restartGame();
    } else {
        close();
    }
}
