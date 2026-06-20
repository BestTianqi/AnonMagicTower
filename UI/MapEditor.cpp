#include "MapEditor.h"
#include "Entities/MonsterDB.h"
#include "Game/Game.h"
#include "UI/MainWindow.h"

#include <QPainter>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QFrame>
#include <QScrollArea>
#include <QApplication>
#include <algorithm>

// ==================== MapEditWidget ====================

MapEditWidget::MapEditWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(900, 900);
    setMouseTracking(true);
    m_floor.tiles.assign(225, { Tile_Wall, "" });
    for (int y = 2; y < 13; ++y)
        for (int x = 2; x < 13; ++x)
            m_floor.tiles[indexAt(x, y)].type = Tile_Floor;
}

void MapEditWidget::clearFloor()
{
    m_floor.tiles.assign(225, { Tile_Wall, "" });
    for (int y = 2; y < 13; ++y)
        for (int x = 2; x < 13; ++x)
            m_floor.tiles[indexAt(x, y)].type = Tile_Floor;
    m_floor.playerX = 2;
    m_floor.playerY = 3;
    update();
}

void MapEditWidget::placeTile(int x, int y)
{
    if (x < 0 || y < 0 || x >= 15 || y >= 15) return;

    auto& t = m_floor.tiles[indexAt(x, y)];
    t.type = m_currentTile;

    // 根据当前图块类型填充数据
    t.monsterName.clear();
    t.itemName.clear();
    t.itemValue = 0;
    t.npcName.clear();
    t.npcDialog.clear();
    t.npcRewardItem.clear();
    t.npcRewardValue = 0;
    t.npcIsTrader = false;
    t.npcTradeGoldCost = 0;
    t.npcTradeRewardItem.clear();
    t.npcTradeRewardValue = 50;
    t.shopPotionPrice = 0;
    t.shopWeaponPrice = 0;
    t.shopArmorPrice  = 0;
    t.shopPotionValue = 200;
    t.shopWeaponValue = 5;
    t.shopArmorValue  = 8;

    if (m_currentTile == Tile_Monster)
        t.monsterName = m_currentMonster;
    else if (m_currentTile == Tile_Item) {
        t.itemName = m_currentItem;
        t.itemValue = m_currentItemValue;
    } else if (m_currentTile == Tile_NPC) {
        t.npcName = m_currentNPC;
        t.npcDialog = m_currentNPCDialog;
        t.npcRewardItem = m_currentNPCRewardItem;
        t.npcRewardValue = m_currentNPCRewardValue;
        if (m_npcIsTrader) {
            t.npcIsTrader = true;
            t.npcTradeGoldCost = m_npcTradeGoldCost;
            t.npcTradeRewardItem = m_npcTradeRewardItem;
            t.npcTradeRewardValue = m_npcTradeRewardValue;
        }
    } else if (m_currentTile == Tile_Shop) {
        t.shopPotionPrice = m_shopPotionPrice;
        t.shopWeaponPrice = m_shopWeaponPrice;
        t.shopArmorPrice  = m_shopArmorPrice;
        t.shopPotionValue = m_shopPotionValue;
        t.shopWeaponValue = m_shopWeaponValue;
        t.shopArmorValue  = m_shopArmorValue;
    }

    update();
    emit tileChanged(x, y);
}

void MapEditWidget::loadFromTile(int x, int y)
{
    auto& t = m_floor.tiles[indexAt(x, y)];
    m_currentTile = t.type;  // 点击时自动切换为图块类型
    if (t.type == Tile_Monster) {
        m_currentMonster = t.monsterName;
    } else if (t.type == Tile_Item) {
        m_currentItem = t.itemName;
        m_currentItemValue = t.itemValue;
    } else if (t.type == Tile_NPC) {
        m_currentNPC = t.npcName;
        m_currentNPCDialog = t.npcDialog;
        m_currentNPCRewardItem = t.npcRewardItem;
        m_currentNPCRewardValue = t.npcRewardValue;
        m_npcIsTrader = t.npcIsTrader;
        m_npcTradeGoldCost = t.npcTradeGoldCost;
        m_npcTradeRewardItem = t.npcTradeRewardItem;
        m_npcTradeRewardValue = t.npcTradeRewardValue;
    } else if (t.type == Tile_Shop) {
        m_shopPotionPrice = t.shopPotionPrice;
        m_shopWeaponPrice = t.shopWeaponPrice;
        m_shopArmorPrice  = t.shopArmorPrice;
        m_shopPotionValue = t.shopPotionValue;
        m_shopWeaponValue = t.shopWeaponValue;
        m_shopArmorValue  = t.shopArmorValue;
    }
}

void MapEditWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(0, 0, 900, 900, QColor(20, 20, 30));

    for (int y = 0; y < 15; ++y) {
        for (int x = 0; x < 15; ++x) {
            auto& t = m_floor.tiles[indexAt(x, y)];
            QRect r(x * 60, y * 60, 60, 60);
            QRect inner = r.adjusted(1, 1, -1, -1);

            QColor fill;
            QString label;
            QString itemDesc;

            switch (t.type) {
            case Tile_Wall:       fill = QColor(55, 55, 60);   break;
            case Tile_DarkWall:   fill = QColor(30, 30, 35);   label = QString::fromUtf8("暗墙"); break;
            case Tile_Floor:      fill = QColor(180, 170, 150); break;
            case Tile_StairsUp:   fill = QColor(180, 160, 50);  label = QString::fromUtf8("↑上"); break;
            case Tile_StairsDown: fill = QColor(160, 100, 180); label = QString::fromUtf8("↓下"); break;
            case Tile_DoorRed:    fill = QColor(180, 60, 50);   label = QString::fromUtf8("红门"); break;
            case Tile_DoorBlue:   fill = QColor(50, 70, 180);   label = QString::fromUtf8("蓝门"); break;
            case Tile_DoorGreen:  fill = QColor(50, 160, 70);   label = QString::fromUtf8("绿门"); break;
            case Tile_Monster:    fill = QColor(200, 80, 80);   break;
            case Tile_Item: {
                fill = QColor(60, 170, 60);
                label = QString::fromUtf8("宝");
                if (!t.itemName.empty()) {
                    QString iname = QString::fromStdString(t.itemName);
                    // 钥匙
                    if (iname == QString::fromUtf8("Red Key") || iname == QString::fromUtf8("红钥匙"))
                        { fill = QColor(200, 45, 45); label = QString::fromUtf8("红钥"); }
                    else if (iname == QString::fromUtf8("Blue Key") || iname == QString::fromUtf8("蓝钥匙"))
                        { fill = QColor(45, 60, 200); label = QString::fromUtf8("蓝钥"); }
                    else if (iname == QString::fromUtf8("Green Key") || iname == QString::fromUtf8("绿钥匙"))
                        { fill = QColor(45, 180, 60); label = QString::fromUtf8("绿钥"); }
                    else if (iname == QString::fromUtf8("万能钥匙"))
                        { fill = QColor(130, 60, 200); label = QString::fromUtf8("万能钥"); }
                    // 属性
                    else if (iname == QString::fromUtf8("Potion") || iname == QString::fromUtf8("生命药") || iname == QString::fromUtf8("药水"))
                        { fill = QColor(200, 60, 60); label = QString::fromUtf8("生命药");
                          itemDesc = QString("+%1HP").arg(t.itemValue); }
                    else if (iname == QString::fromUtf8("Weapon") || iname == QString::fromUtf8("武器"))
                        { fill = QColor(210, 140, 40); label = QString::fromUtf8("武器");
                          itemDesc = QString("ATK+%1").arg(t.itemValue); }
                    else if (iname == QString::fromUtf8("Armor") || iname == QString::fromUtf8("防具"))
                        { fill = QColor(60, 120, 200); label = QString::fromUtf8("防具");
                          itemDesc = QString("DEF+%1").arg(t.itemValue); }
                    else if (iname == QString::fromUtf8("Treasure") || iname == QString::fromUtf8("金币"))
                        { fill = QColor(220, 180, 40); label = QString::fromUtf8("金币");
                          itemDesc = QString("%1G").arg(t.itemValue); }
                    // 特殊
                    else if (iname == QString::fromUtf8("匿名眼镜"))
                        { fill = QColor(40, 180, 180); label = QString::fromUtf8("眼镜"); }
                    else if (iname == QString::fromUtf8("破墙锤"))
                        { fill = QColor(140, 100, 70); label = QString::fromUtf8("破墙锤"); }
                    else if (iname == QString::fromUtf8("上楼器"))
                        { fill = QColor(180, 170, 60); label = QString::fromUtf8("上楼器"); }
                    else if (iname == QString::fromUtf8("下楼器"))
                        { fill = QColor(160, 110, 180); label = QString::fromUtf8("下楼器"); }
                    else if (iname == QString::fromUtf8("临时护盾"))
                        { fill = QColor(80, 160, 220); label = QString::fromUtf8("护盾"); }
                    else if (iname == QString::fromUtf8("企鹅玩偶"))
                        { fill = QColor(220, 130, 170); label = QString::fromUtf8("企鹅"); }
                    else if (iname == QString::fromUtf8("抹茶芭菲"))
                        { fill = QColor(140, 200, 100); label = QString::fromUtf8("芭菲"); }
                    else if (iname == QString::fromUtf8("幸运金币"))
                        { fill = QColor(240, 200, 20); label = QString::fromUtf8("幸运币"); }
                    else
                        { label = iname.left(4); }
                }
                break;
            }
            case Tile_NPC:        fill = QColor(200, 160, 60);
                label = t.npcName.empty()
                    ? QString::fromUtf8("NPC")
                    : QString::fromStdString(t.npcName).left(3);
                break;
            case Tile_Shop:       fill = QColor(240, 200, 20);
                label = QString::fromUtf8("商店");
                break;
            default:              fill = QColor(30, 30, 30);   break;
            }

            painter.fillRect(inner, fill);
            painter.setPen(QPen(QColor(50, 50, 50), 1));
            painter.drawRect(r);

            if (t.type == Tile_Monster) {
                QString name = t.monsterName.empty()
                    ? QString::fromUtf8("怪")
                    : QString::fromStdString(t.monsterName);
                QFont f;
                f.setPixelSize(12);
                f.setBold(true);
                painter.setFont(f);
                painter.setPen(Qt::white);
                painter.drawText(r.adjusted(0, 3, 0, 0), Qt::AlignHCenter | Qt::AlignTop, name);

                if (!t.monsterName.empty()) {
                    Monster m = MonsterDB::get(t.monsterName);
                    f.setPixelSize(9);
                    f.setBold(false);
                    painter.setFont(f);
                    painter.setPen(QColor(240, 240, 200));
                    painter.drawText(r.adjusted(2, 24, -2, 0), Qt::AlignHCenter | Qt::AlignTop,
                        QString("HP:%1 ATK:%2").arg(m.GetHP()).arg(m.GetATK()));
                    painter.drawText(r.adjusted(2, 38, -2, 0), Qt::AlignHCenter | Qt::AlignTop,
                        QString("DEF:%1 G:%2").arg(m.GetDEF()).arg(m.GetGold()));
                }
            } else if (t.type == Tile_Item && !itemDesc.isEmpty()) {
                QFont f;
                f.setPixelSize(11);
                f.setBold(true);
                painter.setFont(f);
                painter.setPen(Qt::white);
                painter.drawText(r.adjusted(0, 2, 0, 0), Qt::AlignHCenter | Qt::AlignTop, label);
                f.setPixelSize(10);
                f.setBold(false);
                painter.setFont(f);
                painter.drawText(r.adjusted(2, 28, -2, 0), Qt::AlignHCenter | Qt::AlignTop, itemDesc);
            } else if (!label.isEmpty()) {
                QFont f;
                f.setPixelSize(label.length() > 2 ? 11 : 14);
                f.setBold(true);
                painter.setFont(f);
                painter.setPen(Qt::white);
                painter.drawText(r, Qt::AlignCenter, label);
            }
        }
    }

    // 玩家位置指示器
    QRect pr(m_floor.playerX * 60, m_floor.playerY * 60, 60, 60);
    painter.setBrush(QColor(60, 130, 240, 180));
    painter.setPen(QPen(QColor(30, 80, 180), 2));
    painter.drawEllipse(pr.adjusted(8, 8, -8, -8));
    QFont f; f.setPixelSize(16); f.setBold(true);
    painter.setFont(f);
    painter.setPen(Qt::white);
    painter.drawText(pr, Qt::AlignCenter, QString::fromUtf8("勇"));

    // 悬停高亮
    if (m_hoverX >= 0 && m_hoverY >= 0) {
        QRect hr(m_hoverX * 60, m_hoverY * 60, 60, 60);
        painter.setPen(QPen(QColor(255, 255, 0, 200), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(hr);
    }
}

void MapEditWidget::mousePressEvent(QMouseEvent* event)
{
    int x = event->pos().x() / 60;
    int y = event->pos().y() / 60;
    if (x < 0 || y < 0 || x >= 15 || y >= 15) return;

    if (event->button() == Qt::LeftButton) {
        if (event->modifiers() & Qt::ShiftModifier) {
            m_floor.playerX = x;
            m_floor.playerY = y;
            update();
            emit playerMoved(x, y);
            return;
        }
        // 左键：用编辑器当前状态放置图块
        placeTile(x, y);
        emit tilePicked(x, y);
    } else if (event->button() == Qt::RightButton) {
        auto& t = m_floor.tiles[indexAt(x, y)];
        bool hasContent = (t.type != Tile_Empty && t.type != Tile_Floor &&
                           t.type != Tile_Wall && t.type != Tile_DarkWall);
        if (hasContent) {
            // 右键点击有内容的图块：加载数据到编辑器（编辑模式）
            loadFromTile(x, y);
            emit tileTypePicked(m_currentTile);
            emit tilePicked(x, y);
        } else {
            // 右键点击空地/墙：擦除为地板
            t = { Tile_Floor, "" };
            update();
            emit tileChanged(x, y);
        }
    }
}

void MapEditWidget::mouseMoveEvent(QMouseEvent* event)
{
    int x = event->pos().x() / 60;
    int y = event->pos().y() / 60;
    if (x >= 0 && x < 15 && y >= 0 && y < 15) {
        if (x != m_hoverX || y != m_hoverY) {
            m_hoverX = x;
            m_hoverY = y;
            if (event->buttons() & Qt::LeftButton && !(event->modifiers() & Qt::ShiftModifier))
                placeTile(x, y);
            else
                update();
        }
    }
}

void MapEditWidget::leaveEvent(QEvent*)
{
    m_hoverX = -1;
    m_hoverY = -1;
    update();
}

// ==================== MapEditor ====================

// 物品列表定义
struct ItemDef { QString name; int defaultValue; const char* desc; };
static const ItemDef g_itemDefs[] = {
    {QString::fromUtf8("生命药"), 50, "恢复生命"},
    {QString::fromUtf8("武器"), 5, "攻击力+"},
    {QString::fromUtf8("防具"), 3, "防御力+"},
    {QString::fromUtf8("金币"), 10, "金币"},
    {QString::fromUtf8("红钥匙"), 1, "红钥匙"},
    {QString::fromUtf8("蓝钥匙"), 2, "蓝钥匙"},
    {QString::fromUtf8("绿钥匙"), 3, "绿钥匙"},
    {QString(), 0, nullptr}  // sentinel = 特殊物品分界线
};
static const char* g_specialItems[] = {
    "万能钥匙", "匿名眼镜", "破墙锤", "上楼器", "下楼器",
    "临时护盾", "企鹅玩偶", "抹茶芭菲", "幸运金币", nullptr
};

MapEditor::MapEditor(QWidget* parent)
    : QWidget(parent, Qt::Window)
{
    setWindowTitle(QString::fromUtf8("魔塔地图编辑器 v2.0"));
    resize(1400, 960);
    setMinimumSize(1300, 920);
    setStyleSheet("QWidget { background-color: #1a1a2e; color: #d0d0d0; }");

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // === 左侧：地图编辑 ===
    m_edit = new MapEditWidget(this);
    mainLayout->addWidget(m_edit);

    // === 右侧面板 ===
    auto* rightPanel = new QWidget(this);
    rightPanel->setFixedWidth(380);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 4, 4, 4);
    rightLayout->setSpacing(4);

    mainLayout->addWidget(rightPanel);

    // ====== 固定区域：楼层管理 + 图块类型（始终可见） ======

    // -- 楼层管理 --
    auto* floorGroup = new QGroupBox(QString::fromUtf8("楼层管理"), rightPanel);
    auto* fbox = new QHBoxLayout(floorGroup);
    auto* prevBtn = new QPushButton(QString::fromUtf8("<"), floorGroup);
    prevBtn->setFixedWidth(30);
    m_floorSpin = new QSpinBox(floorGroup);
    m_floorSpin->setRange(1, 20);
    m_floorSpin->setValue(1);
    m_floorSpin->setStyleSheet("QSpinBox { background: #222; color: #fff; border: 1px solid #555; padding: 4px; font-size: 14px; }");
    auto* nextBtn = new QPushButton(QString::fromUtf8(">"), floorGroup);
    nextBtn->setFixedWidth(30);
    auto* addFloorBtn = new QPushButton(QString::fromUtf8("+"), floorGroup);
    addFloorBtn->setFixedWidth(30);
    addFloorBtn->setToolTip(QString::fromUtf8("添加楼层"));
    m_removeFloorBtn = new QPushButton(QString::fromUtf8("-"), floorGroup);
    m_removeFloorBtn->setFixedWidth(30);
    m_removeFloorBtn->setToolTip(QString::fromUtf8("删除当前楼层"));
    m_floorCountLabel = new QLabel(QString::fromUtf8("/ 1"), floorGroup);

    fbox->addWidget(prevBtn);
    fbox->addWidget(m_floorSpin);
    fbox->addWidget(m_floorCountLabel);
    fbox->addWidget(nextBtn);
    fbox->addWidget(addFloorBtn);
    fbox->addWidget(m_removeFloorBtn);
    rightLayout->addWidget(floorGroup);

    // -- 图块类型（彩色按钮面板，始终可见） --
    auto* tileGroup = new QGroupBox(QString::fromUtf8("图块类型 (点击选择)"), rightPanel);
    auto* tgrid = new QGridLayout(tileGroup);
    tgrid->setSpacing(3);

    struct TileBtn { int type; QString text; QString bgColor; };
    TileBtn btns[] = {
        { Tile_Wall,       QString::fromUtf8(" 墙 "),      "#666" },
        { Tile_Floor,      QString::fromUtf8(" 地板 "),    "#4a4" },
        { Tile_StairsUp,   QString::fromUtf8(" 上楼梯 "),  "#bb0" },
        { Tile_StairsDown, QString::fromUtf8(" 下楼梯 "),  "#b6b" },
        { Tile_Monster,    QString::fromUtf8(" 怪物 "),    "#d44" },
        { Tile_Item,       QString::fromUtf8(" 道具 "),    "#4c4" },
        { Tile_DoorRed,    QString::fromUtf8(" 红门 "),    "#d33" },
        { Tile_DoorBlue,   QString::fromUtf8(" 蓝门 "),    "#44d" },
        { Tile_DoorGreen,  QString::fromUtf8(" 绿门 "),    "#3b3" },
        { Tile_NPC,        QString::fromUtf8(" NPC "),     "#db3" },
        { Tile_Shop,       QString::fromUtf8(" 商店 "),    "#da0" },
        { Tile_DarkWall,   QString::fromUtf8(" 暗墙 "),    "#333" },
    };
    const int btnCount = sizeof(btns) / sizeof(btns[0]);

    for (int i = 0; i < btnCount; ++i) {
        auto* btn = new QPushButton(btns[i].text, tileGroup);
        btn->setFixedHeight(34);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QString(
            "QPushButton { background-color: %1; color: #fff; border: 2px solid #888; "
            "border-radius: 3px; font-size: 13px; font-weight: bold; }"
            "QPushButton:hover { border-color: #fff; }"
        ).arg(btns[i].bgColor));
        tgrid->addWidget(btn, i / 5, i % 5);

        int tileType = btns[i].type;
        QString tileName = btns[i].text;
        connect(btn, &QPushButton::pressed, this, [this, tileType, tileName]() {
            m_currentTileType = tileType;
            m_edit->setCurrentTile(tileType);
            int idx = m_tileCombo->findData(tileType);
            if (idx >= 0) m_tileCombo->setCurrentIndex(idx);
            updatePanelForTile(tileType);
            setWindowTitle(QString::fromUtf8("地图编辑器 [%1]").arg(tileName.trimmed()));
            m_statusLabel->setText(QString::fromUtf8(">> 当前选中: %1 (type=%2) <<").arg(tileName.trimmed()).arg(tileType));
        });
    }
    rightLayout->addWidget(tileGroup);

    // 隐藏的 combobox 用于状态同步
    m_tileCombo = new QComboBox();
    m_tileCombo->addItem(QString::fromUtf8("墙"),       Tile_Wall);
    m_tileCombo->addItem(QString::fromUtf8("地板"),      Tile_Floor);
    m_tileCombo->addItem(QString::fromUtf8("上楼梯"),    Tile_StairsUp);
    m_tileCombo->addItem(QString::fromUtf8("下楼梯"),    Tile_StairsDown);
    m_tileCombo->addItem(QString::fromUtf8("怪物"),      Tile_Monster);
    m_tileCombo->addItem(QString::fromUtf8("道具"),      Tile_Item);
    m_tileCombo->addItem(QString::fromUtf8("红门"),      Tile_DoorRed);
    m_tileCombo->addItem(QString::fromUtf8("蓝门"),      Tile_DoorBlue);
    m_tileCombo->addItem(QString::fromUtf8("绿门"),      Tile_DoorGreen);
    m_tileCombo->addItem(QString::fromUtf8("NPC"),        Tile_NPC);
    m_tileCombo->addItem(QString::fromUtf8("商店"),       Tile_Shop);
    m_tileCombo->addItem(QString::fromUtf8("暗墙"),       Tile_DarkWall);
    m_tileCombo->setVisible(false);

    // ====== 可滚动区域：属性面板 ======
    auto* scroll = new QScrollArea(rightPanel);
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { border: 1px solid #444; }");

    auto* panel = new QWidget(scroll);
    auto* pbox = new QVBoxLayout(panel);
    pbox->setContentsMargins(8, 8, 8, 8);
    pbox->setSpacing(4);
    scroll->setWidget(panel);
    rightLayout->addWidget(scroll, 1);

    // -- 怪物选择面板 --
    auto* monGroup = new QGroupBox(QString::fromUtf8("怪物属性"), panel);
    auto* mbox = new QVBoxLayout(monGroup);
    m_monsterCombo = new QComboBox(monGroup);
    m_monsterCombo->setStyleSheet("QComboBox { background: #222; border: 1px solid #555; padding: 4px; }");
    for (auto& m : MonsterDB::all())
        m_monsterCombo->addItem(QString::fromStdString(m.GetName()));
    mbox->addWidget(new QLabel(QString::fromUtf8("选择怪物类型:"), monGroup));
    mbox->addWidget(m_monsterCombo);
    pbox->addWidget(monGroup);

    // -- 道具选择面板 --
    auto* itemGroup = new QGroupBox(QString::fromUtf8("道具选择"), panel);
    auto* ibox = new QVBoxLayout(itemGroup);
    m_itemPanel = new QWidget(itemGroup);
    auto* ilay = new QVBoxLayout(m_itemPanel);
    ilay->setContentsMargins(0, 0, 0, 0);

    // 数值行
    auto* valLay = new QHBoxLayout();
    m_itemValueLabel = new QLabel(QString::fromUtf8("数值:"), m_itemPanel);
    m_itemValueSpin = new QSpinBox(m_itemPanel);
    m_itemValueSpin->setRange(1, 9999);
    m_itemValueSpin->setValue(50);
    m_itemValueSpin->setStyleSheet("QSpinBox { background: #222; color: #fff; border: 1px solid #555; padding: 4px; }");
    valLay->addWidget(m_itemValueLabel);
    valLay->addWidget(m_itemValueSpin);
    valLay->addStretch();
    ilay->addLayout(valLay);

    // 道具按钮网格
    auto* igrid = new QGridLayout();
    igrid->setSpacing(2);

    struct ItemBtn { QString name; int val; QString color; bool hasValue; QString tip; };
    ItemBtn itemBtns[] = {
        { QString::fromUtf8("生命药"), 50, "#d44", true,  QString::fromUtf8("恢复生命值") },
        { QString::fromUtf8("武器"),   5,  "#d82", true,  QString::fromUtf8("攻击力+") },
        { QString::fromUtf8("防具"),   3,  "#48d", true,  QString::fromUtf8("防御力+") },
        { QString::fromUtf8("金币"),   10, "#da0", true,  QString::fromUtf8("获得金币") },
        { QString::fromUtf8("红钥匙"), 0,  "#d33", false, QString::fromUtf8("红钥匙 x1") },
        { QString::fromUtf8("蓝钥匙"), 0,  "#33d", false, QString::fromUtf8("蓝钥匙 x1") },
        { QString::fromUtf8("绿钥匙"), 0,  "#3a3", false, QString::fromUtf8("绿钥匙 x1") },
        { QString::fromUtf8("万能钥匙"), 0, "#84d", false, QString::fromUtf8("红蓝绿钥匙各+1") },
        { QString::fromUtf8("匿名眼镜"), 0, "#4aa", false, QString::fromUtf8("可查看怪物属性") },
        { QString::fromUtf8("破墙锤"),   0, "#864", false, QString::fromUtf8("摧毁墙壁") },
        { QString::fromUtf8("上楼器"),   0, "#aa0", false, QString::fromUtf8("从当前位置上楼") },
        { QString::fromUtf8("下楼器"),   0, "#a6a", false, QString::fromUtf8("从当前位置下楼") },
        { QString::fromUtf8("临时护盾"), 0, "#68d", false, QString::fromUtf8("防御力+10") },
        { QString::fromUtf8("企鹅玩偶"), 0, "#d6a", false, QString::fromUtf8("神秘的企鹅玩偶") },
        { QString::fromUtf8("抹茶芭菲"), 0, "#8c6", false, QString::fromUtf8("美味的抹茶芭菲") },
        { QString::fromUtf8("幸运金币"), 0, "#da0", false, QString::fromUtf8("打怪和拾取金币翻倍") },
    };
    const int itemBtnCount = sizeof(itemBtns) / sizeof(itemBtns[0]);

    // 隐藏的组合框，同步用
    m_itemCombo = new QComboBox();
    m_itemCombo->setVisible(false);

    for (int i = 0; i < itemBtnCount; ++i) {
        auto* ibtn = new QPushButton(itemBtns[i].name, m_itemPanel);
        ibtn->setFixedHeight(30);
        ibtn->setCursor(Qt::PointingHandCursor);
        ibtn->setToolTip(itemBtns[i].tip);
        ibtn->setStyleSheet(QString(
            "QPushButton { background-color: %1; color: #fff; border: 2px solid #666; "
            "border-radius: 3px; font-size: 12px; font-weight: bold; }"
            "QPushButton:hover { border-color: #fff; }"
        ).arg(itemBtns[i].color));
        igrid->addWidget(ibtn, i / 3, i % 3);

        QString iname = itemBtns[i].name;
        int ival = itemBtns[i].val;
        bool hasVal = itemBtns[i].hasValue;

        connect(ibtn, &QPushButton::pressed, this, [this, iname, ival, hasVal]() {
            m_selectedItemName = iname;
            m_selectedItemValue = hasVal ? m_itemValueSpin->value() : ival;
            m_itemValueLabel->setVisible(hasVal);
            m_itemValueSpin->setVisible(hasVal);
            if (hasVal) m_itemValueSpin->setValue(ival);
            m_edit->setCurrentItem(iname.toStdString(), m_selectedItemValue);
            // 同步隐藏 combobox
            int idx = m_itemCombo->findText(iname);
            if (idx >= 0) m_itemCombo->setCurrentIndex(idx);
            m_statusLabel->setText(QString::fromUtf8("道具: %1  — 在左侧地图放置").arg(iname));
        });
        m_itemCombo->addItem(itemBtns[i].name);
    }
    ilay->addLayout(igrid);
    ibox->addWidget(m_itemPanel);
    pbox->addWidget(itemGroup);

    // -- NPC 编辑面板 --
    auto* npcGroup = new QGroupBox(QString::fromUtf8("NPC 属性"), panel);
    auto* nbox = new QVBoxLayout(npcGroup);
    m_npcPanel = new QWidget(npcGroup);
    auto* nlay = new QVBoxLayout(m_npcPanel);
    nlay->setContentsMargins(0, 0, 0, 0);

    nlay->addWidget(new QLabel(QString::fromUtf8("NPC 名称:"), m_npcPanel));
    m_npcNameEdit = new QLineEdit(m_npcPanel);
    m_npcNameEdit->setPlaceholderText(QString::fromUtf8("输入NPC名称"));
    m_npcNameEdit->setStyleSheet("QLineEdit { background: #222; border: 1px solid #555; padding: 4px; }");
    nlay->addWidget(m_npcNameEdit);

    nlay->addWidget(new QLabel(QString::fromUtf8("对话（每行一句）:"), m_npcPanel));
    m_npcDialogEdit = new QTextEdit(m_npcPanel);
    m_npcDialogEdit->setPlaceholderText(QString::fromUtf8("输入NPC对话..."));
    m_npcDialogEdit->setMaximumHeight(80);
    m_npcDialogEdit->setStyleSheet("QTextEdit { background: #222; border: 1px solid #555; padding: 4px; }");
    nlay->addWidget(m_npcDialogEdit);

    nlay->addWidget(new QLabel(QString::fromUtf8("奖励道具:"), m_npcPanel));
    m_npcRewardCombo = new QComboBox(m_npcPanel);
    m_npcRewardCombo->setStyleSheet("QComboBox { background: #222; border: 1px solid #555; padding: 4px; }");
    m_npcRewardCombo->addItem(QString::fromUtf8("(无奖励)"));
    for (int i = 0; !g_itemDefs[i].name.isNull(); ++i)
        m_npcRewardCombo->addItem(g_itemDefs[i].name);
    m_npcRewardCombo->insertSeparator(m_npcRewardCombo->count());
    for (int i = 0; g_specialItems[i]; ++i)
        m_npcRewardCombo->addItem(QString::fromUtf8(g_specialItems[i]));
    nlay->addWidget(m_npcRewardCombo);

    // 奖励数值
    auto* rewardValRow = new QHBoxLayout();
    rewardValRow->addWidget(new QLabel(QString::fromUtf8("奖励数值:"), m_npcPanel));
    m_npcRewardValueSpin = new QSpinBox(m_npcPanel);
    m_npcRewardValueSpin->setRange(0, 9999);
    m_npcRewardValueSpin->setValue(50);
    m_npcRewardValueSpin->setStyleSheet("QSpinBox { background: #222; border: 1px solid #555; padding: 2px; }");
    rewardValRow->addWidget(m_npcRewardValueSpin);
    rewardValRow->addStretch();
    nlay->addLayout(rewardValRow);

    // -- NPC 交易设置 --
    m_npcTradeCheck = new QCheckBox(QString::fromUtf8("启用交易"), m_npcPanel);
    m_npcTradeCheck->setStyleSheet("QCheckBox { color: #ccaa88; }");
    nlay->addWidget(m_npcTradeCheck);

    m_npcTradePanel = new QWidget(m_npcPanel);
    m_npcTradePanel->setVisible(false);
    auto* tradeLay = new QVBoxLayout(m_npcTradePanel);
    tradeLay->setContentsMargins(8, 0, 0, 0);

    auto* goldRow = new QHBoxLayout();
    goldRow->addWidget(new QLabel(QString::fromUtf8("金币价格:"), m_npcTradePanel));
    m_npcTradeGoldSpin = new QSpinBox(m_npcTradePanel);
    m_npcTradeGoldSpin->setRange(0, 9999);
    m_npcTradeGoldSpin->setStyleSheet("QSpinBox { background: #222; border: 1px solid #555; padding: 2px; }");
    m_npcTradeGoldSpin->setToolTip(QString::fromUtf8("玩家需支付的金币数，0=免费"));
    goldRow->addWidget(m_npcTradeGoldSpin);
    goldRow->addStretch();
    tradeLay->addLayout(goldRow);

    auto* tradeRewardRow = new QHBoxLayout();
    tradeRewardRow->addWidget(new QLabel(QString::fromUtf8("交易物品:"), m_npcTradePanel));
    m_npcTradeRewardCombo = new QComboBox(m_npcTradePanel);
    m_npcTradeRewardCombo->setStyleSheet("QComboBox { background: #222; border: 1px solid #555; padding: 2px; }");
    m_npcTradeRewardCombo->addItem(QString::fromUtf8("(无)"));
    for (int i = 0; !g_itemDefs[i].name.isNull(); ++i)
        m_npcTradeRewardCombo->addItem(g_itemDefs[i].name);
    m_npcTradeRewardCombo->insertSeparator(m_npcTradeRewardCombo->count());
    for (int i = 0; g_specialItems[i]; ++i)
        m_npcTradeRewardCombo->addItem(QString::fromUtf8(g_specialItems[i]));
    tradeRewardRow->addWidget(m_npcTradeRewardCombo);
    m_npcTradeRewardValueSpin = new QSpinBox(m_npcTradePanel);
    m_npcTradeRewardValueSpin->setRange(0, 9999);
    m_npcTradeRewardValueSpin->setValue(50);
    m_npcTradeRewardValueSpin->setStyleSheet("QSpinBox { background: #222; border: 1px solid #555; padding: 2px; }");
    tradeRewardRow->addWidget(new QLabel(QString::fromUtf8("数值:"), m_npcTradePanel));
    tradeRewardRow->addWidget(m_npcTradeRewardValueSpin);
    tradeRewardRow->addStretch();
    tradeLay->addLayout(tradeRewardRow);

    nlay->addWidget(m_npcTradePanel);

    nbox->addWidget(m_npcPanel);
    pbox->addWidget(npcGroup);
    m_npcPanel->parentWidget()->setVisible(false);

    // -- 商店编辑面板 --
    auto* shopGroup = new QGroupBox(QString::fromUtf8("商店属性"), panel);
    auto* sbox = new QVBoxLayout(shopGroup);
    m_shopPanel = new QWidget(shopGroup);
    auto* slay = new QVBoxLayout(m_shopPanel);
    slay->setContentsMargins(0, 0, 0, 0);

    auto makeShopRow = [&](const QString& name, QSpinBox*& priceSpin, QSpinBox*& valueSpin,
                           int defaultPrice, int defaultValue,
                           const QString& valueSuffix, const QString& valuePrefix) {
        auto* group = new QVBoxLayout();
        auto* label = new QLabel(name, m_shopPanel);
        label->setStyleSheet("color: #c8a23b; font-weight: bold; font-size: 12px;");
        group->addWidget(label);

        auto* row = new QHBoxLayout();
        row->addWidget(new QLabel(QString::fromUtf8("价格:"), m_shopPanel));
        priceSpin = new QSpinBox(m_shopPanel);
        priceSpin->setRange(0, 9999);
        priceSpin->setValue(defaultPrice);
        priceSpin->setPrefix(QString::fromUtf8("C "));
        priceSpin->setSuffix(QString::fromUtf8(" G"));
        priceSpin->setStyleSheet("QSpinBox { background: #222; color: #fff; border: 1px solid #555; padding: 4px; }");
        priceSpin->setToolTip(QString::fromUtf8("0 表示不售卖"));
        row->addWidget(priceSpin);

        row->addWidget(new QLabel(QString::fromUtf8(" 数值:"), m_shopPanel));
        valueSpin = new QSpinBox(m_shopPanel);
        valueSpin->setRange(1, 9999);
        valueSpin->setValue(defaultValue);
        valueSpin->setPrefix(valuePrefix);
        valueSpin->setSuffix(valueSuffix);
        valueSpin->setStyleSheet("QSpinBox { background: #222; color: #fff; border: 1px solid #555; padding: 4px; }");
        row->addWidget(valueSpin);
        row->addStretch();

        group->addLayout(row);
        slay->addLayout(group);
    };

    makeShopRow(QString::fromUtf8("生命药"), m_shopPotionPriceSpin, m_shopPotionValueSpin,
        0, 200, QString::fromUtf8(" HP"), QString::fromUtf8("+"));
    makeShopRow(QString::fromUtf8("武器"),   m_shopWeaponPriceSpin, m_shopWeaponValueSpin,
        0, 5, QString::fromUtf8(" ATK"), QString::fromUtf8("+"));
    makeShopRow(QString::fromUtf8("防具"),   m_shopArmorPriceSpin,  m_shopArmorValueSpin,
        0, 8, QString::fromUtf8(" DEF"), QString::fromUtf8("+"));

    sbox->addWidget(m_shopPanel);
    pbox->addWidget(shopGroup);
    m_shopPanel->parentWidget()->setVisible(false);

    // -- 玩家位置 --
    auto* playerGroup = new QGroupBox(QString::fromUtf8("玩家起始位置"), panel);
    auto* pLay = new QHBoxLayout(playerGroup);
    auto* playerLabel = new QLabel(playerGroup);
    playerLabel->setStyleSheet("color: #88aacc; font-size: 12px;");
    pLay->addWidget(playerLabel);
    pbox->addWidget(playerGroup);

    // -- 状态（固定在底部，始终可见） --
    m_statusLabel = new QLabel(QString::fromUtf8("就绪 — 左键放置 右键擦除 Shift+左键放玩家"), rightPanel);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color: #aaa; font-size: 11px; padding: 4px;");
    rightLayout->addWidget(m_statusLabel);

    // -- 操作按钮（固定在底部，始终可见） --
    auto* sep = new QFrame(rightPanel);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #555;");
    rightLayout->addWidget(sep);

    auto btnStyle = QString(
        "QPushButton { min-height: 36px; border-radius: 4px; padding: 6px; font-size: 13px; }"
    );

    auto* newBtn = new QPushButton(QString::fromUtf8("新建地图"), rightPanel);
    newBtn->setStyleSheet(btnStyle + "QPushButton { background: #3a4a5a; color: #d0d0d0; }");
    rightLayout->addWidget(newBtn);

    auto* loadBtn = new QPushButton(QString::fromUtf8("加载地图"), rightPanel);
    loadBtn->setStyleSheet(btnStyle + "QPushButton { background: #4a4a5a; color: #d0d0d0; }");
    rightLayout->addWidget(loadBtn);

    auto* saveBtn = new QPushButton(QString::fromUtf8("保存地图"), rightPanel);
    saveBtn->setStyleSheet(btnStyle + "QPushButton { background: #3a5a3a; color: #d0d0d0; }");
    rightLayout->addWidget(saveBtn);

    auto* exportBtn = new QPushButton(QString::fromUtf8("导出当前层 (.map)"), rightPanel);
    exportBtn->setStyleSheet(btnStyle + "QPushButton { background: #3a4a3a; color: #d0d0d0; }");
    rightLayout->addWidget(exportBtn);

    auto* defaultBtn = new QPushButton(QString::fromUtf8("设为默认地图"), rightPanel);
    defaultBtn->setStyleSheet(btnStyle + "QPushButton { background: #5a4a1a; color: #ffd; }");
    rightLayout->addWidget(defaultBtn);

    auto* testBtn = new QPushButton(QString::fromUtf8("测试游玩"), rightPanel);
    testBtn->setStyleSheet(btnStyle + "QPushButton { background: #5a3a1a; color: #ffd; font-weight: bold; }");
    rightLayout->addWidget(testBtn);

    auto* clearBtn = new QPushButton(QString::fromUtf8("清空当前层"), rightPanel);
    clearBtn->setStyleSheet(btnStyle + "QPushButton { background: #5a3a3a; color: #d0d0d0; }");
    rightLayout->addWidget(clearBtn);

    // ====== 信号连接 ======

    // 楼层切换 — 直接调用 switchToFloor，不依赖 spinbox 信号链
    connect(m_floorSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int floor) {
        switchToFloor(floor);
    });
    connect(prevBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentFloor > 1)
            switchToFloor(m_currentFloor - 1);
    });
    connect(nextBtn, &QPushButton::clicked, this, [this]() {
        int maxFloor = 1;
        for (auto& kv : m_floors) if (kv.first > maxFloor) maxFloor = kv.first;
        if (m_currentFloor < maxFloor || (int)m_floors.size() < 20)
            switchToFloor(m_currentFloor + 1);
    });
    connect(addFloorBtn, &QPushButton::clicked, this, &MapEditor::addFloor);
    connect(m_removeFloorBtn, &QPushButton::clicked, this, &MapEditor::removeFloor);

    // 图块切换 — 通过每个 radiobutton 的 clicked 信号处理

    // 怪物选择
    connect(m_monsterCombo, &QComboBox::currentTextChanged, this, [this](const QString& name) {
        m_edit->setCurrentMonster(name.toStdString());
    });

    // 道具数值微调时更新当前道具
    connect(m_itemValueSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (m_tileCombo->currentData().toInt() == Tile_Item && !m_selectedItemName.isEmpty()) {
            m_selectedItemValue = val;
            m_edit->setCurrentItem(m_selectedItemName.toStdString(), val);
        }
    });

    // NPC 编辑 → 实时更新
    auto updateNpc = [this]() {
        if (m_tileCombo->currentData().toInt() == Tile_NPC) {
            // 对话
            std::vector<std::string> dialog;
            QString dtext = m_npcDialogEdit->toPlainText().trimmed();
            if (!dtext.isEmpty()) {
                for (auto& line : dtext.split('\n'))
                    dialog.push_back(line.toStdString());
            }
            // 奖励
            std::string rewardItem;
            int rewardValue = m_npcRewardValueSpin->value();
            if (m_npcRewardCombo->currentIndex() > 0)
                rewardItem = m_npcRewardCombo->currentText().toStdString();
            m_edit->setCurrentNPC(m_npcNameEdit->text().toStdString(),
                dialog, rewardItem, rewardValue);
            m_edit->setCurrentNPCTrade(
                m_npcTradeCheck->isChecked(),
                m_npcTradeGoldSpin->value(),
                m_npcTradeRewardCombo->currentText().toStdString(),
                m_npcTradeRewardValueSpin->value());
        }
    };
    connect(m_npcNameEdit, &QLineEdit::textChanged, this, updateNpc);
    connect(m_npcDialogEdit, &QTextEdit::textChanged, this, updateNpc);
    connect(m_npcRewardCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, updateNpc);
    connect(m_npcRewardValueSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateNpc);
    connect(m_npcTradeCheck, &QCheckBox::toggled, this, [this, updateNpc](bool checked) {
        m_npcTradePanel->setVisible(checked);
        updateNpc();
    });
    connect(m_npcTradeGoldSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateNpc);
    connect(m_npcTradeRewardCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, updateNpc);
    connect(m_npcTradeRewardValueSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateNpc);

    // 商店价格变动 → 更新当前编辑状态
    auto updateShop = [this]() {
        if (m_tileCombo->currentData().toInt() == Tile_Shop)
            m_edit->setCurrentShop(
                m_shopPotionPriceSpin->value(),
                m_shopWeaponPriceSpin->value(),
                m_shopArmorPriceSpin->value(),
                m_shopPotionValueSpin->value(),
                m_shopWeaponValueSpin->value(),
                m_shopArmorValueSpin->value());
    };
    connect(m_shopPotionPriceSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateShop);
    connect(m_shopWeaponPriceSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateShop);
    connect(m_shopArmorPriceSpin,  QOverload<int>::of(&QSpinBox::valueChanged), this, updateShop);
    connect(m_shopPotionValueSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateShop);
    connect(m_shopWeaponValueSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateShop);
    connect(m_shopArmorValueSpin,  QOverload<int>::of(&QSpinBox::valueChanged), this, updateShop);

    // 编辑区信号
    connect(m_edit, &MapEditWidget::tileTypePicked, this, [this](int tileType) {
        m_currentTileType = tileType;
        int idx = m_tileCombo->findData(tileType);
        if (idx >= 0) m_tileCombo->setCurrentIndex(idx);
        updatePanelForTile(tileType);
        populatePanels(tileType);
    });
    connect(m_edit, &MapEditWidget::tileChanged, this, [this](int x, int y) {
        m_statusLabel->setText(QString::fromUtf8("已放置 %1 于 (%2, %3)")
            .arg(m_tileCombo->currentText())
            .arg(x).arg(y));
    });
    connect(m_edit, &MapEditWidget::playerMoved, this, [this, playerLabel](int x, int y) {
        m_statusLabel->setText(QString::fromUtf8("玩家位置: (%1, %2) — Shift+左键可移动").arg(x).arg(y));
        playerLabel->setText(QString::fromUtf8("当前: (%1, %2)").arg(x).arg(y));
    });

    // 按钮
    connect(newBtn,     &QPushButton::clicked, this, &MapEditor::onNewMap);
    connect(saveBtn,    &QPushButton::clicked, this, &MapEditor::onSave);
    connect(exportBtn,  &QPushButton::clicked, this, &MapEditor::onExportFloor);
    connect(loadBtn,    &QPushButton::clicked, this, &MapEditor::onLoad);
    connect(defaultBtn, &QPushButton::clicked, this, &MapEditor::onSaveAsDefault);
    connect(testBtn,    &QPushButton::clicked, this, &MapEditor::onTestPlay);
    connect(clearBtn,   &QPushButton::clicked, m_edit, &MapEditWidget::clearFloor);

    // 点击图块时反写面板
    connect(m_edit, &MapEditWidget::tilePicked, this, [this](int, int) {
        populatePanels(m_tileCombo->currentData().toInt());
    });

    // 初始状态
    m_edit->setCurrentTile(Tile_Wall);
    m_edit->setCurrentMonster(m_monsterCombo->currentText().toStdString());
    m_selectedItemName = QString::fromUtf8("生命药");
    m_selectedItemValue = 50;
    m_edit->setCurrentItem(m_selectedItemName.toStdString(), m_selectedItemValue);
    m_edit->setCurrentNPC("", {}, "", 0);
    m_floors[1] = m_edit->floorData();
    updatePanelForTile(Tile_Wall);
}

// ====== 楼层管理 ======

void MapEditor::storeCurrentFloor()
{
    m_floors[m_currentFloor] = m_edit->floorData();
}

void MapEditor::switchToFloor(int floor, bool storeCurrent)
{
    if (storeCurrent)
        storeCurrentFloor();
    m_currentFloor = floor;
    if (m_floors.find(floor) == m_floors.end()) {
        EditorFloor ef;
        ef.tiles.assign(225, { Tile_Wall, "" });
        for (int y = 2; y < 13; ++y)
            for (int x = 2; x < 13; ++x)
                ef.tiles[y * 15 + x].type = Tile_Floor;
        m_floors[floor] = ef;
    }
    m_edit->setFloor(m_floors[floor]);
    m_floorSpin->blockSignals(true);
    m_floorSpin->setValue(floor);
    m_floorSpin->blockSignals(false);
    m_floorCountLabel->setText(QString("/ %1").arg((int)m_floors.size()));
    m_removeFloorBtn->setEnabled(m_floors.size() > 1);
    m_statusLabel->setText(QString::fromUtf8("切换到第 %1 层").arg(floor));
}

void MapEditor::addFloor()
{
    int newFloor = 1;
    while (m_floors.find(newFloor) != m_floors.end())
        newFloor++;
    if (newFloor > 20) {
        QMessageBox::warning(this, QString::fromUtf8("上限"), QString::fromUtf8("最多支持 20 层。"));
        return;
    }
    storeCurrentFloor();
    EditorFloor ef;
    ef.tiles.assign(225, { Tile_Wall, "" });
    for (int y = 2; y < 13; ++y)
        for (int x = 2; x < 13; ++x)
            ef.tiles[y * 15 + x].type = Tile_Floor;
    m_floors[newFloor] = ef;
    m_floorSpin->setMaximum(newFloor);
    switchToFloor(newFloor);
}

void MapEditor::removeFloor()
{
    if (m_floors.size() <= 1) return;
    m_floors.erase(m_currentFloor);
    // 找到最近楼层
    int nearest = 1;
    for (auto& kv : m_floors) { nearest = kv.first; break; }
    m_floorSpin->setMaximum(20);
    switchToFloor(nearest, false);  // 不保存旧数据，已被删除
}

void MapEditor::populatePanels(int tileType)
{
    if (tileType == Tile_Monster) {
        QString name = QString::fromStdString(m_edit->currentMonster());
        int idx = m_monsterCombo->findText(name);
        if (idx >= 0) m_monsterCombo->setCurrentIndex(idx);
    } else if (tileType == Tile_NPC) {
        m_npcNameEdit->setText(QString::fromStdString(m_edit->currentNPC()));
        QString dialogText;
        for (auto& d : m_edit->currentNPCDialog()) {
            if (!dialogText.isEmpty()) dialogText += "\n";
            dialogText += QString::fromStdString(d);
        }
        m_npcDialogEdit->setPlainText(dialogText);
        QString rewardName = QString::fromStdString(m_edit->currentNPCRewardItem());
        int idx = m_npcRewardCombo->findText(rewardName);
        if (idx >= 0) m_npcRewardCombo->setCurrentIndex(idx);
        else m_npcRewardCombo->setCurrentIndex(0);
        m_npcRewardValueSpin->setValue(m_edit->currentNPCRewardValue());
        m_npcTradeCheck->setChecked(m_edit->currentNPCIsTrader());
        m_npcTradePanel->setVisible(m_edit->currentNPCIsTrader());
        m_npcTradeGoldSpin->setValue(m_edit->currentNPCTradeGoldCost());
        QString tradeReward = QString::fromStdString(m_edit->currentNPCTradeRewardItem());
        idx = m_npcTradeRewardCombo->findText(tradeReward);
        if (idx >= 0) m_npcTradeRewardCombo->setCurrentIndex(idx);
        m_npcTradeRewardValueSpin->setValue(m_edit->currentNPCTradeRewardValue());
    }
}

void MapEditor::updatePanelForTile(int tileType)
{
    bool isMonster = (tileType == Tile_Monster);
    bool isItem    = (tileType == Tile_Item);
    bool isNPC     = (tileType == Tile_NPC);
    bool isShop    = (tileType == Tile_Shop);

    m_monsterCombo->parentWidget()->setVisible(isMonster);  // monGroup
    m_itemPanel->parentWidget()->setVisible(isItem);        // itemGroup
    m_npcPanel->parentWidget()->setVisible(isNPC);          // npcGroup
    m_shopPanel->parentWidget()->setVisible(isShop);        // shopGroup
    if (isNPC) m_npcPanel->setVisible(true);                // 确保内容重新显示
    else       m_npcTradePanel->setVisible(false);          // 离开NPC时收起交易子面板

    // 更新道具数值标签可见性
    QString itemName = m_itemCombo->currentText();
    bool isSpecial = false;
    for (int i = 0; g_specialItems[i]; ++i)
        if (itemName == QString::fromUtf8(g_specialItems[i])) { isSpecial = true; break; }
    m_itemValueLabel->setVisible(!isSpecial);
    m_itemValueSpin->setVisible(!isSpecial);
}

void MapEditor::onTileTypeChanged(int id)
{
    m_edit->setCurrentTile(id);
    updatePanelForTile(id);
    m_statusLabel->setText(QString::fromUtf8("当前图块: %1  — 左键放置 右键擦除 Shift+左键放玩家")
        .arg(m_tileCombo->currentText()));
}

// ====== 新建 ======

void MapEditor::onNewMap()
{
    auto reply = QMessageBox::question(this,
        QString::fromUtf8("新建地图"),
        QString::fromUtf8("确定要新建地图吗？当前未保存的内容将丢失。"),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    m_floors.clear();
    m_currentFloor = 1;
    EditorFloor ef;
    ef.tiles.assign(225, { Tile_Wall, "" });
    for (int y = 2; y < 13; ++y)
        for (int x = 2; x < 13; ++x)
            ef.tiles[y * 15 + x].type = Tile_Floor;
    m_floors[1] = ef;
    m_floorSpin->setMaximum(1);
    m_edit->setFloor(ef);
    m_floorSpin->setValue(1);
    m_floorCountLabel->setText("/ 1");
    m_statusLabel->setText(QString::fromUtf8("已新建空白地图"));
}

// ====== 保存 / 加载 ======

static std::unique_ptr<Item> createItem(const std::string& name, int value)
{
    // English names (from save files)
    if (name == "Potion")   return std::make_unique<Potion>(value);
    if (name == "Weapon")   return std::make_unique<Weapon>(value);
    if (name == "Armor")    return std::make_unique<Armor>(value);
    if (name == "Treasure") return std::make_unique<Treasure>(value);
    if (name == "Red Key")  return std::make_unique<Key>(KeyType::Red);
    if (name == "Blue Key") return std::make_unique<Key>(KeyType::Blue);
    if (name == "Green Key") return std::make_unique<Key>(KeyType::Green);
    // Chinese names (from editor buttons)
    if (name == QString::fromUtf8("生命药").toStdString())   return std::make_unique<Potion>(value);
    if (name == QString::fromUtf8("武器").toStdString())     return std::make_unique<Weapon>(value);
    if (name == QString::fromUtf8("防具").toStdString())     return std::make_unique<Armor>(value);
    if (name == QString::fromUtf8("金币").toStdString())     return std::make_unique<Treasure>(value);
    if (name == QString::fromUtf8("红钥匙").toStdString())   return std::make_unique<Key>(KeyType::Red);
    if (name == QString::fromUtf8("蓝钥匙").toStdString())   return std::make_unique<Key>(KeyType::Blue);
    if (name == QString::fromUtf8("绿钥匙").toStdString())   return std::make_unique<Key>(KeyType::Green);
    if (name == QString::fromUtf8("万能钥匙").toStdString()) return std::make_unique<MagicKey>();
    if (name == QString::fromUtf8("匿名眼镜").toStdString()) return std::make_unique<AnonGlasses>();
    if (name == QString::fromUtf8("破墙锤").toStdString())   return std::make_unique<WallBreaker>();
    if (name == QString::fromUtf8("上楼器").toStdString())   return std::make_unique<StairUpper>();
    if (name == QString::fromUtf8("下楼器").toStdString())   return std::make_unique<StairLower>();
    if (name == QString::fromUtf8("临时护盾").toStdString()) return std::make_unique<TempShield>();
    if (name == QString::fromUtf8("企鹅玩偶").toStdString()) return std::make_unique<PenguinDoll>();
    if (name == QString::fromUtf8("抹茶芭菲").toStdString()) return std::make_unique<MatchaParfait>();
    if (name == QString::fromUtf8("幸运金币").toStdString()) return std::make_unique<LuckyCoin>();
    return nullptr;
}

bool MapEditor::saveToPath(const QString& path)
{
    storeCurrentFloor();

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("无法写入文件"));
        return false;
    }

    // 按楼层号排序
    std::vector<std::pair<int, EditorFloor*>> sorted;
    for (auto& fp : m_floors)
        sorted.push_back({fp.first, &fp.second});
    std::sort(sorted.begin(), sorted.end(),
        [](auto& a, auto& b) { return a.first < b.first; });

    QTextStream out(&file);
    out << "MOTA2\n";
    out << "15 15\n";
    out << m_currentFloor << "\n";
    out << sorted.size() << "\n";

    for (auto& [fnum, ef] : sorted) {
        out << fnum << "\n";

        // 地图网格
        for (int y = 0; y < 15; ++y) {
            for (int x = 0; x < 15; ++x) {
                out << ef->tiles[y * 15 + x].type;
                if (x < 14) out << " ";
            }
            out << "\n";
        }

        // 地面物品（含钥匙）
        int itemCount = 0;
        for (auto& t : ef->tiles)
            if (t.type == Tile_Item && !t.itemName.empty())
                ++itemCount;
        out << itemCount << "\n";
        for (int y = 0; y < 15; ++y) {
            for (int x = 0; x < 15; ++x) {
                auto& t = ef->tiles[y * 15 + x];
                if (t.type == Tile_Item && !t.itemName.empty())
                    out << x << " " << y << " " << QString::fromStdString(t.itemName)
                        << " " << t.itemValue << "\n";
            }
        }

        // 怪物
        int monCount = 0;
        for (auto& t : ef->tiles)
            if (t.type == Tile_Monster && !t.monsterName.empty())
                ++monCount;
        out << monCount << "\n";
        for (int y = 0; y < 15; ++y) {
            for (int x = 0; x < 15; ++x) {
                auto& t = ef->tiles[y * 15 + x];
                if (t.type == Tile_Monster && !t.monsterName.empty()) {
                    Monster m = MonsterDB::get(t.monsterName);
                    out << (y * 15 + x) << " " << QString::fromStdString(m.GetName()) << " "
                        << m.GetHP() << " " << m.GetATK() << " "
                        << m.GetDEF() << " " << m.GetGold() << "\n";
                }
            }
        }

        // NPC
        int npcCount = 0;
        for (auto& t : ef->tiles)
            if (t.type == Tile_NPC && !t.npcName.empty())
                ++npcCount;
        out << npcCount << "\n";
        for (int y = 0; y < 15; ++y) {
            for (int x = 0; x < 15; ++x) {
                auto& t = ef->tiles[y * 15 + x];
                if (t.type == Tile_NPC && !t.npcName.empty()) {
                    out << x << " " << y << " " << QString::fromStdString(t.npcName) << " "
                        << "0 " << t.npcDialog.size() << " "
                        << (t.npcRewardItem.empty() ? "-" : QString::fromStdString(t.npcRewardItem)) << " "
                        << t.npcRewardValue << " "
                        << t.npcIsTrader << " " << t.npcTradeGoldCost << " "
                        << (t.npcTradeRewardItem.empty() ? "-" : QString::fromStdString(t.npcTradeRewardItem)) << " "
                        << t.npcTradeRewardValue << "\n";
                    for (auto& d : t.npcDialog)
                        out << QString::fromStdString(d) << "\n";
                }
            }
        }

    }


    // 商店 (SHOP block)
    {
        size_t totalShops = 0;
        for (auto& [fnum, ef] : sorted) {
            for (auto& t : ef->tiles)
                if (t.type == Tile_Shop && (t.shopPotionPrice > 0 || t.shopWeaponPrice > 0 || t.shopArmorPrice > 0))
                    ++totalShops;
        }
        out << "SHOP\n" << totalShops << "\n";
        for (auto& [fnum, ef] : sorted) {
            for (int y = 0; y < 15; ++y) {
                for (int x = 0; x < 15; ++x) {
                    auto& t = ef->tiles[y * 15 + x];
                    if (t.type == Tile_Shop && (t.shopPotionPrice > 0 || t.shopWeaponPrice > 0 || t.shopArmorPrice > 0))
                        out << fnum << " " << x << " " << y << " "
                            << t.shopPotionPrice << " " << t.shopWeaponPrice << " "
                            << t.shopArmorPrice << " "
                            << t.shopPotionValue << " " << t.shopWeaponValue << " "
                            << t.shopArmorValue << "\n";
                }
            }
        }
    }
    // 玩家数据 (取第一层玩家位置)
    auto& f1 = m_floors[1];
    out << f1.playerX << " " << f1.playerY << " 100 10 5 0\n";
    out << "0 0 0\n";
    out << "0 0 0\n";
    out << "0\n";

    file.close();
    return true;
}

void MapEditor::onSave()
{
    QString path = QFileDialog::getSaveFileName(this,
        QString::fromUtf8("保存地图"), QString(),
        QString::fromUtf8("魔塔存档 (*.txt)"));
    if (path.isEmpty()) return;

    int floorCount = (int)m_floors.size();
    if (saveToPath(path)) {
        m_statusLabel->setText(QString::fromUtf8("已保存 %1 层到: %2").arg(floorCount).arg(path));
        QMessageBox::information(this, QString::fromUtf8("保存成功"),
            QString::fromUtf8("已保存 %1 个楼层。\n\n"
                "提示：在游戏中需要放置楼梯(↑上/↓下)\n"
                "才能在不同楼层间切换！").arg(floorCount));
    }
}

void MapEditor::onSaveAsDefault()
{
    int floorCount = (int)m_floors.size();
    QString path = QDir::currentPath() + "/default_map.txt";
    if (saveToPath(path)) {
        m_statusLabel->setText(QString::fromUtf8("已设默认地图 (%1 层)！新游戏将使用此地图。").arg(floorCount));
        QMessageBox::information(this, QString::fromUtf8("默认地图"),
            QString::fromUtf8("已保存 %1 个楼层为默认地图。\n\n"
                "新游戏将加载此地图。\n"
                "提示：确保已放置楼梯(↑上/↓下)！").arg(floorCount));
    }
}

void MapEditor::onExportFloor()
{
    storeCurrentFloor();
    auto it = m_floors.find(m_currentFloor);
    if (it == m_floors.end()) return;

    QString path = QFileDialog::getSaveFileName(this,
        QString::fromUtf8("导出当前层"),
        QString::fromUtf8("floor_%1.map").arg(m_currentFloor),
        QString::fromUtf8("地图文件 (*.map)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("无法写入文件"));
        return;
    }

    auto& ef = it->second;
    QTextStream out(&file);

    // 格式: 15x15 网格 + 玩家位置 + 怪物/物品/NPC 数据
    out << "15 15\n";
    for (int y = 0; y < 15; ++y) {
        for (int x = 0; x < 15; ++x) {
            out << ef.tiles[y * 15 + x].type;
            if (x < 14) out << " ";
        }
        out << "\n";
    }

    out << ef.playerX << " " << ef.playerY << "\n";

    // 怪物列表
    std::vector<std::pair<int, EditorTile*>> monsters;
    std::vector<std::pair<int, EditorTile*>> items;
    std::vector<std::pair<int, EditorTile*>> npcs;
    for (int y = 0; y < 15; ++y)
        for (int x = 0; x < 15; ++x) {
            auto& t = ef.tiles[y * 15 + x];
            if (t.type == Tile_Monster && !t.monsterName.empty())
                monsters.push_back({y * 15 + x, &t});
            else if (t.type == Tile_Item && !t.itemName.empty())
                items.push_back({y * 15 + x, &t});
            else if (t.type == Tile_NPC && !t.npcName.empty())
                npcs.push_back({y * 15 + x, &t});
        }

    out << monsters.size() << "\n";
    for (auto& [key, t] : monsters) {
        Monster m = MonsterDB::get(t->monsterName);
        out << (key % 15) << " " << (key / 15) << " "
            << QString::fromStdString(m.GetName()) << " "
            << m.GetHP() << " " << m.GetATK() << " " << m.GetDEF() << " " << m.GetGold() << "\n";
    }

    out << items.size() << "\n";
    for (auto& [key, t] : items) {
        out << (key % 15) << " " << (key / 15) << " "
            << QString::fromStdString(t->itemName) << " " << t->itemValue << "\n";
    }

    out << npcs.size() << "\n";
    for (auto& [key, t] : npcs) {
        out << (key % 15) << " " << (key / 15) << " "
            << QString::fromStdString(t->npcName) << " 0 "
            << t->npcDialog.size() << " "
            << (t->npcRewardItem.empty() ? "-" : QString::fromStdString(t->npcRewardItem)) << " "
            << t->npcRewardValue << " "
            << t->npcIsTrader << " " << t->npcTradeGoldCost << " "
            << (t->npcTradeRewardItem.empty() ? "-" : QString::fromStdString(t->npcTradeRewardItem)) << " "
            << t->npcTradeRewardValue << "\n";
        for (auto& d : t->npcDialog)
            out << QString::fromStdString(d) << "\n";
    }

    // 商店
    std::vector<std::pair<int, EditorTile*>> shops;
    for (int y = 0; y < 15; ++y)
        for (int x = 0; x < 15; ++x) {
            auto& t = ef.tiles[y * 15 + x];
            if (t.type == Tile_Shop && (t.shopPotionPrice > 0 || t.shopWeaponPrice > 0 || t.shopArmorPrice > 0))
                shops.push_back({y * 15 + x, &t});
        }
    out << shops.size() << "\n";
    for (auto& [key, t] : shops) {
        out << (key % 15) << " " << (key / 15) << " "
            << t->shopPotionPrice << " " << t->shopWeaponPrice << " " << t->shopArmorPrice << " "
            << t->shopPotionValue << " " << t->shopWeaponValue << " " << t->shopArmorValue << "\n";
    }

    file.close();
    m_statusLabel->setText(QString::fromUtf8("已导出当前层: %1").arg(path));
}

void MapEditor::onLoad()
{
    QString path = QFileDialog::getOpenFileName(this,
        QString::fromUtf8("加载地图"), QString(),
        QString::fromUtf8("魔塔存档 (*.txt);;地图文件 (*.map)"));
    if (path.isEmpty()) return;

    // 先尝试 .map 格式
    if (path.endsWith(".map", Qt::CaseInsensitive)) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
        QTextStream in(&file);
        int w, h; in >> w >> h;
        if (w != 15 || h != 15) {
            QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("仅支持 15x15"));
            return;
        }
        m_floors.clear();
        EditorFloor ef;
        ef.tiles.resize(225);
        for (int y = 0; y < 15; ++y)
            for (int x = 0; x < 15; ++x) {
                int t; in >> t;
                ef.tiles[y * 15 + x].type = t;
            }
        in >> ef.playerX >> ef.playerY;
        int monCount; in >> monCount;
        for (int i = 0; i < monCount; ++i) {
            int mx, my, mhp, matk, mdef, mgold;
            QString mname;
            in >> mx >> my >> mname >> mhp >> matk >> mdef >> mgold;
            ef.tiles[my * 15 + mx].monsterName = mname.toStdString();
        }
        // 物品
        int icount; in >> icount;
        for (int i = 0; i < icount; ++i) {
            int ix, iy, ival; QString iname;
            in >> ix >> iy >> iname >> ival;
            ef.tiles[iy * 15 + ix].itemName = iname.toStdString();
            ef.tiles[iy * 15 + ix].itemValue = ival;
        }
        // NPC
        int ncount; in >> ncount;
        for (int i = 0; i < ncount; ++i) {
            int nx, ny, given, dsize, rewardValue, isTrader = 0, tradeGoldCost = 0, tradeRewardValue = 50;
            QString nname, rewardName, tradeRewardName = "-";
            in >> nx >> ny >> nname >> given >> dsize >> rewardName >> rewardValue;
            QString rest = in.readLine();
            if (!rest.trimmed().isEmpty()) {
                QTextStream rs(&rest);
                rs >> isTrader >> tradeGoldCost >> tradeRewardName >> tradeRewardValue;
            }
            auto& t = ef.tiles[ny * 15 + nx];
            t.npcName = nname.toStdString();
            if (rewardName != "-") {
                t.npcRewardItem = rewardName.toStdString();
                t.npcRewardValue = rewardValue;
            }
            t.npcIsTrader = (isTrader != 0);
            t.npcTradeGoldCost = tradeGoldCost;
            if (tradeRewardName != "-") {
                t.npcTradeRewardItem = tradeRewardName.toStdString();
                t.npcTradeRewardValue = tradeRewardValue;
            }
            for (int d = 0; d < dsize; ++d) {
                QString line = in.readLine();
                t.npcDialog.push_back(line.toStdString());
            }
        }
        // 商店
        int scount; in >> scount;
        for (int i = 0; i < scount; ++i) {
            int sx, sy, pp, wp, ap, pv = 200, wv = 5, av = 8;
            in >> sx >> sy >> pp >> wp >> ap;
            QString rest = in.readLine(); // 读剩余数值或空行
            QTextStream rs(&rest);
            rs >> pv >> wv >> av;
            ef.tiles[sy * 15 + sx].shopPotionPrice = pp;
            ef.tiles[sy * 15 + sx].shopWeaponPrice = wp;
            ef.tiles[sy * 15 + sx].shopArmorPrice  = ap;
            ef.tiles[sy * 15 + sx].shopPotionValue = pv;
            ef.tiles[sy * 15 + sx].shopWeaponValue = wv;
            ef.tiles[sy * 15 + sx].shopArmorValue  = av;
        }
        m_floors[1] = ef;
        m_floorSpin->setMaximum(1);
        switchToFloor(1, false);
        m_statusLabel->setText(QString::fromUtf8("已加载 .map: %1").arg(path));
        return;
    }

    // .txt 格式 (游戏存档)
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("无法打开文件"));
        return;
    }

    QTextStream in(&file);

    // 检测版本标记
    QString version;
    in >> version;
    bool isV2 = (version == "MOTA2");
    int mw, mh;
    if (!isV2) {
        mw = version.toInt();
        in >> mh;
    } else {
        in >> mw >> mh;
    }
    if (mw != 15 || mh != 15) {
        QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("仅支持 15x15"));
        return;
    }

    int currentFloor; in >> currentFloor;
    int floorCount; in >> floorCount;

    m_floors.clear();
    for (int fi = 0; fi < floorCount; ++fi) {
        int fnum; in >> fnum;
        EditorFloor ef;
        ef.tiles.resize(225);
        for (int y = 0; y < 15; ++y)
            for (int x = 0; x < 15; ++x)
                in >> ef.tiles[y * 15 + x].type;

        // 物品
        int icount; in >> icount;
        for (int i = 0; i < icount; ++i) {
            int ix, iy, ival; QString iname;
            in >> ix >> iy >> iname >> ival;
            auto& t = ef.tiles[iy * 15 + ix];
            t.itemName = iname.toStdString();
            t.itemValue = ival;
        }

        // 怪物
        int mcount; in >> mcount;
        for (int i = 0; i < mcount; ++i) {
            int key, mhp, matk, mdef, mgold; QString mname;
            in >> key >> mname >> mhp >> matk >> mdef >> mgold;
            int mx = key % 15, my = key / 15;
            ef.tiles[my * 15 + mx].monsterName = mname.toStdString();
        }

        // NPC
        int ncount; in >> ncount;
        for (int i = 0; i < ncount; ++i) {
            int nx, ny, given, dsize, rewardValue; QString nname, rewardName;
            in >> nx >> ny >> nname >> given >> dsize >> rewardName >> rewardValue;
            // 交易字段 (v2 扩展, 可选)
            QString rest = in.readLine();
            int isTrader = 0, tradeGoldCost = 0, tradeRewardValue = 50;
            QString tradeRewardName = "-";
            if (!rest.trimmed().isEmpty()) {
                QTextStream rs(&rest);
                rs >> isTrader >> tradeGoldCost >> tradeRewardName >> tradeRewardValue;
            }
            auto& t = ef.tiles[ny * 15 + nx];
            t.npcName = nname.toStdString();
            if (rewardName != "-") {
                t.npcRewardItem = rewardName.toStdString();
                t.npcRewardValue = rewardValue;
            }
            t.npcIsTrader = (isTrader != 0);
            t.npcTradeGoldCost = tradeGoldCost;
            if (tradeRewardName != "-") {
                t.npcTradeRewardItem = tradeRewardName.toStdString();
                t.npcTradeRewardValue = tradeRewardValue;
            }
            for (int d = 0; d < dsize; ++d) {
                QString line = in.readLine();
                t.npcDialog.push_back(line.toStdString());
            }
        }

        // 商店 (仅旧格式 per-floor)
        if (!isV2) {
            int scount; in >> scount;
            for (int i = 0; i < scount; ++i) {
                int sx, sy, pp, wp, ap, pv = 200, wv = 5, av = 8;
                in >> sx >> sy >> pp >> wp >> ap;
                QString rest = in.readLine();
                if (!rest.trimmed().isEmpty()) {
                    QTextStream rs(&rest);
                    rs >> pv >> wv >> av;
                }
                auto& t = ef.tiles[sy * 15 + sx];
                t.shopPotionPrice = pp;
                t.shopWeaponPrice = wp;
                t.shopArmorPrice  = ap;
                t.shopPotionValue = pv;
                t.shopWeaponValue = wv;
                t.shopArmorValue  = av;
            }
        }

        m_floors[fnum] = ef;
    }

    // 商店 (SHOP block, 新格式) 或旧格式直接进入玩家数据
    int px = 0, py = 0, php = 100, patk = 10, pdef = 5, pgold = 0;
    if (isV2) {
        // V2 格式: 跳过空行, 读取 SHOP 标记
        QString marker = in.readLine().trimmed();
        if (marker.isEmpty()) marker = in.readLine().trimmed();
        if (marker == "SHOP") {
            int totalShops; in >> totalShops;
            for (int i = 0; i < totalShops; ++i) {
                int fnum, sx, sy, pp, wp, ap, pv = 200, wv = 5, av = 8;
                in >> fnum >> sx >> sy >> pp >> wp >> ap;
                QString rest = in.readLine();
                if (!rest.trimmed().isEmpty()) {
                    QTextStream rs(&rest);
                    rs >> pv >> wv >> av;
                }
                auto it = m_floors.find(fnum);
                if (it != m_floors.end()) {
                    auto& t = it->second.tiles[sy * 15 + sx];
                    t.shopPotionPrice = pp;
                    t.shopWeaponPrice = wp;
                    t.shopArmorPrice  = ap;
                    t.shopPotionValue = pv;
                    t.shopWeaponValue = wv;
                    t.shopArmorValue  = av;
                }
            }
            // 读取玩家数据
            in >> px >> py >> php >> patk >> pdef >> pgold;
        }
    } else {
        // 旧格式: 读取玩家数据行
        in >> px >> py >> php >> patk >> pdef >> pgold;
    }

    // 应用玩家位置到第一层
    if (m_floors.find(1) != m_floors.end()) {
        m_floors[1].playerX = px;
        m_floors[1].playerY = py;
    }

    int maxFloor = 1;
    for (auto& kv : m_floors) if (kv.first > maxFloor) maxFloor = kv.first;
    m_floorSpin->setMaximum(std::max(maxFloor, 1));
    switchToFloor(1, false);
    m_statusLabel->setText(QString::fromUtf8("已加载: %1 (%2 层)").arg(path).arg(floorCount));
}

// ====== 测试游玩 ======

void MapEditor::onTestPlay()
{
    storeCurrentFloor();

    // 构建 Game 对象
    auto* game = new Game();
    game->initFloor(1); // 确保至少有第一层

    for (auto& fp : m_floors) {
        int fnum = fp.first;
        auto& ef = fp.second;
        game->initFloor(fnum);

        // 设置玩家起始位置 (仅第一层)
        if (fnum == 1) {
            game->player().x = ef.playerX;
            game->player().y = ef.playerY;
            game->player().hp = 100;
            game->player().atk = 10;
            game->player().def = 5;
        }

        // 恢复图块
        for (int y = 0; y < 15; ++y)
            for (int x = 0; x < 15; ++x) {
                auto& t = ef.tiles[y * 15 + x];
                game->setTile(x, y, t.type);
            }

        // 怪物
        for (int y = 0; y < 15; ++y)
            for (int x = 0; x < 15; ++x) {
                auto& t = ef.tiles[y * 15 + x];
                if (t.type == Tile_Monster && !t.monsterName.empty()) {
                    Monster m = MonsterDB::get(t.monsterName);
                    game->spawnMonster(x, y, m);
                }
            }

        // 物品
        for (int y = 0; y < 15; ++y)
            for (int x = 0; x < 15; ++x) {
                auto& t = ef.tiles[y * 15 + x];
                if (t.type == Tile_Item && !t.itemName.empty()) {
                    auto item = createItem(t.itemName, t.itemValue);
                    if (item) game->addItemAt(x, y, std::move(item));
                    else game->setTile(x, y, Tile_Floor); // 未知物品→地板
                }
            }

        // NPC
        for (int y = 0; y < 15; ++y)
            for (int x = 0; x < 15; ++x) {
                auto& t = ef.tiles[y * 15 + x];
                if (t.type == Tile_NPC && !t.npcName.empty()) {
                    auto reward = t.npcRewardItem.empty()
                        ? nullptr : createItem(t.npcRewardItem, t.npcRewardValue);
                    auto tradeReward = t.npcTradeRewardItem.empty()
                        ? nullptr : createItem(t.npcTradeRewardItem, t.npcTradeRewardValue);
                    game->addNPCAt(x, y, NPC(t.npcName, t.npcDialog, std::move(reward),
                        t.npcIsTrader, t.npcTradeGoldCost, std::move(tradeReward)));
                }
            }

        // 商店
        for (int y = 0; y < 15; ++y)
            for (int x = 0; x < 15; ++x) {
                auto& t = ef.tiles[y * 15 + x];
                if (t.type == Tile_Shop)
                    game->addShopAt(x, y, {t.shopPotionPrice, t.shopWeaponPrice, t.shopArmorPrice,
                                           t.shopPotionValue, t.shopWeaponValue, t.shopArmorValue});
            }
    }

    // 启动游戏
    auto* win = new MainWindow(game);
    win->loadAssets();
    win->setAttribute(Qt::WA_DeleteOnClose);
    win->show();
    win->raise();
    win->activateWindow();
    win->setFocus();

    m_statusLabel->setText(QString::fromUtf8("测试游玩窗口已打开"));
}

// ================================================================
// 重写 MainWindow.h 中的 Game 构造函数以支持外部 Game*
// (MainWindow 已支持 Game* 参数, 无需修改)
// ================================================================
