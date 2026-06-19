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
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QFrame>
#include <QScrollArea>
#include <QApplication>

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

    if (m_currentTile == Tile_Monster)
        t.monsterName = m_currentMonster;
    else if (m_currentTile == Tile_Item) {
        t.itemName = m_currentItem;
        t.itemValue = m_currentItemValue;
    } else if (m_currentTile == Tile_NPC) {
        t.npcName = m_currentNPC;
    }

    update();
    emit tileChanged(x, y);
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

            switch (t.type) {
            case Tile_Wall:       fill = QColor(55, 55, 60);   break;
            case Tile_Floor:      fill = QColor(180, 170, 150); break;
            case Tile_StairsUp:   fill = QColor(180, 160, 50);  label = QString::fromUtf8("↑上"); break;
            case Tile_StairsDown: fill = QColor(160, 100, 180); label = QString::fromUtf8("↓下"); break;
            case Tile_DoorRed:    fill = QColor(180, 60, 50);   label = QString::fromUtf8("红门"); break;
            case Tile_DoorBlue:   fill = QColor(50, 70, 180);   label = QString::fromUtf8("蓝门"); break;
            case Tile_DoorGreen:  fill = QColor(50, 160, 70);   label = QString::fromUtf8("绿门"); break;
            case Tile_Monster:    fill = QColor(200, 80, 80);
                label = t.monsterName.empty()
                    ? QString::fromUtf8("怪")
                    : QString::fromStdString(t.monsterName);
                break;
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
                    else if (iname == QString::fromUtf8("Potion") || iname == QString::fromUtf8("药水"))
                        { fill = QColor(200, 60, 60); label = QString::fromUtf8("生命药"); }
                    else if (iname == QString::fromUtf8("Weapon") || iname == QString::fromUtf8("武器"))
                        { fill = QColor(210, 140, 40); label = QString::fromUtf8("武器"); }
                    else if (iname == QString::fromUtf8("Armor") || iname == QString::fromUtf8("防具"))
                        { fill = QColor(60, 120, 200); label = QString::fromUtf8("防具"); }
                    else if (iname == QString::fromUtf8("Treasure") || iname == QString::fromUtf8("金币"))
                        { fill = QColor(220, 180, 40); label = QString::fromUtf8("金币"); }
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
            default:              fill = QColor(30, 30, 30);   break;
            }

            painter.fillRect(inner, fill);
            painter.setPen(QPen(QColor(50, 50, 50), 1));
            painter.drawRect(r);

            if (!label.isEmpty()) {
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
        placeTile(x, y);
    } else if (event->button() == Qt::RightButton) {
        // 右键擦除为地板
        auto& t = m_floor.tiles[indexAt(x, y)];
        t = { Tile_Floor, "" };
        update();
        emit tileChanged(x, y);
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
struct ItemDef { const char* name; int defaultValue; const char* desc; };
static const ItemDef g_itemDefs[] = {
    {"Potion", 50, "恢复生命"},
    {"Weapon", 5, "攻击力+"},
    {"Armor", 3, "防御力+"},
    {"Treasure", 10, "金币"},
    {"Red Key", 1, "红钥匙"},
    {"Blue Key", 2, "蓝钥匙"},
    {"Green Key", 3, "绿钥匙"},
    {nullptr, 0, nullptr}  // sentinel = 特殊物品分界线
};
static const char* g_specialItems[] = {
    "万能钥匙", "匿名眼镜", "破墙锤", "上楼器", "下楼器",
    "临时护盾", "企鹅玩偶", "抹茶芭菲", nullptr
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
    auto* scroll = new QScrollArea(this);
    scroll->setFixedWidth(380);
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { border: 1px solid #444; }");

    auto* panel = new QWidget(scroll);
    auto* pbox = new QVBoxLayout(panel);
    pbox->setContentsMargins(10, 10, 10, 10);
    pbox->setSpacing(6);
    scroll->setWidget(panel);
    mainLayout->addWidget(scroll);

    // -- 楼层管理 --
    auto* floorGroup = new QGroupBox(QString::fromUtf8("楼层管理"), panel);
    auto* fbox = new QHBoxLayout(floorGroup);
    auto* prevBtn = new QPushButton(QString::fromUtf8("◀"), floorGroup);
    prevBtn->setFixedWidth(30);
    m_floorSpin = new QSpinBox(floorGroup);
    m_floorSpin->setRange(1, 20);
    m_floorSpin->setValue(1);
    m_floorSpin->setStyleSheet("QSpinBox { background: #222; color: #fff; border: 1px solid #555; padding: 4px; font-size: 14px; }");
    auto* nextBtn = new QPushButton(QString::fromUtf8("▶"), floorGroup);
    nextBtn->setFixedWidth(30);
    auto* addFloorBtn = new QPushButton(QString::fromUtf8("＋"), floorGroup);
    addFloorBtn->setFixedWidth(30);
    addFloorBtn->setToolTip(QString::fromUtf8("添加楼层"));
    m_removeFloorBtn = new QPushButton(QString::fromUtf8("－"), floorGroup);
    m_removeFloorBtn->setFixedWidth(30);
    m_removeFloorBtn->setToolTip(QString::fromUtf8("删除当前楼层"));
    m_floorCountLabel = new QLabel(QString::fromUtf8("/ 1"), floorGroup);

    fbox->addWidget(prevBtn);
    fbox->addWidget(m_floorSpin);
    fbox->addWidget(m_floorCountLabel);
    fbox->addWidget(nextBtn);
    fbox->addWidget(addFloorBtn);
    fbox->addWidget(m_removeFloorBtn);
    pbox->addWidget(floorGroup);

    // -- 图块类型 --
    auto* tileGroup = new QGroupBox(QString::fromUtf8("图块类型"), panel);
    auto* tbox = new QVBoxLayout(tileGroup);
    m_tileGroup = new QButtonGroup(this);
    m_tileGroup->setExclusive(true);

    struct TileBtn { int type; QString text; QString color; };
    TileBtn btns[] = {
        { Tile_Wall,       QString::fromUtf8("墙"),     "#555" },
        { Tile_Floor,      QString::fromUtf8("地板"),   "#aaa" },
        { Tile_StairsUp,   QString::fromUtf8("上楼梯"), "#aa0" },
        { Tile_StairsDown, QString::fromUtf8("下楼梯"), "#a0a" },
        { Tile_Monster,    QString::fromUtf8("怪物"),   "#f55" },
        { Tile_Item,       QString::fromUtf8("道具"),   "#5f5" },
        { Tile_DoorRed,    QString::fromUtf8("红门"),   "#f44" },
        { Tile_DoorBlue,   QString::fromUtf8("蓝门"),   "#44f" },
        { Tile_DoorGreen,  QString::fromUtf8("绿门"),   "#4a4" },
        { Tile_NPC,        QString::fromUtf8("NPC"),    "#fa0" },
    };

    for (int i = 0; i < 10; ++i) {
        auto* rb = new QRadioButton(
            QString("<span style='color:%1'>■</span> %2").arg(btns[i].color, btns[i].text), tileGroup);
        m_tileGroup->addButton(rb, btns[i].type);
        tbox->addWidget(rb);
        if (btns[i].type == Tile_Wall) rb->setChecked(true);
    }
    pbox->addWidget(tileGroup);

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
    auto* itemGroup = new QGroupBox(QString::fromUtf8("道具属性"), panel);
    auto* ibox = new QVBoxLayout(itemGroup);
    m_itemPanel = new QWidget(itemGroup);
    auto* ilay = new QVBoxLayout(m_itemPanel);
    ilay->setContentsMargins(0, 0, 0, 0);
    m_itemCombo = new QComboBox(m_itemPanel);
    m_itemCombo->setStyleSheet("QComboBox { background: #222; border: 1px solid #555; padding: 4px; }");
    // 普通物品
    for (int i = 0; g_itemDefs[i].name; ++i)
        m_itemCombo->addItem(QString::fromUtf8(g_itemDefs[i].name));
    m_itemCombo->insertSeparator(m_itemCombo->count());
    // 特殊物品
    for (int i = 0; g_specialItems[i]; ++i)
        m_itemCombo->addItem(QString::fromUtf8(g_specialItems[i]));

    auto* valLay = new QHBoxLayout();
    m_itemValueLabel = new QLabel(QString::fromUtf8("数值:"), m_itemPanel);
    m_itemValueSpin = new QSpinBox(m_itemPanel);
    m_itemValueSpin->setRange(1, 9999);
    m_itemValueSpin->setValue(50);
    m_itemValueSpin->setStyleSheet("QSpinBox { background: #222; border: 1px solid #555; padding: 4px; width: 60px; }");
    valLay->addWidget(m_itemValueLabel);
    valLay->addWidget(m_itemValueSpin);
    valLay->addStretch();

    ilay->addWidget(new QLabel(QString::fromUtf8("道具类型:"), m_itemPanel));
    ilay->addWidget(m_itemCombo);
    ilay->addLayout(valLay);
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
    for (int i = 0; g_itemDefs[i].name; ++i)
        m_npcRewardCombo->addItem(QString::fromUtf8(g_itemDefs[i].name));
    m_npcRewardCombo->insertSeparator(m_npcRewardCombo->count());
    for (int i = 0; g_specialItems[i]; ++i)
        m_npcRewardCombo->addItem(QString::fromUtf8(g_specialItems[i]));
    nlay->addWidget(m_npcRewardCombo);

    nbox->addWidget(m_npcPanel);
    pbox->addWidget(npcGroup);

    // -- 玩家位置 --
    auto* playerGroup = new QGroupBox(QString::fromUtf8("玩家起始位置"), panel);
    auto* pLay = new QHBoxLayout(playerGroup);
    auto* playerLabel = new QLabel(playerGroup);
    playerLabel->setStyleSheet("color: #88aacc; font-size: 12px;");
    pLay->addWidget(playerLabel);
    pbox->addWidget(playerGroup);

    // -- 状态 --
    m_statusLabel = new QLabel(QString::fromUtf8("就绪 — 左键放置 右键擦除 Shift+左键放玩家"), panel);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color: #888; font-size: 11px; padding: 4px;");
    pbox->addWidget(m_statusLabel);

    pbox->addStretch();

    // -- 操作按钮 --
    auto* sep = new QFrame(panel);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #555;");
    pbox->addWidget(sep);

    auto btnStyle = QString(
        "QPushButton { min-height: 36px; border-radius: 4px; padding: 6px; font-size: 13px; }"
        "QPushButton:hover { filter: brightness(1.3); }"
    );

    auto* newBtn = new QPushButton(QString::fromUtf8("🆕 新建地图"), panel);
    newBtn->setStyleSheet(btnStyle + "QPushButton { background: #3a4a5a; color: #d0d0d0; }");
    pbox->addWidget(newBtn);

    auto* loadBtn = new QPushButton(QString::fromUtf8("📂 加载地图"), panel);
    loadBtn->setStyleSheet(btnStyle + "QPushButton { background: #4a4a5a; color: #d0d0d0; }");
    pbox->addWidget(loadBtn);

    auto* saveBtn = new QPushButton(QString::fromUtf8("💾 保存地图"), panel);
    saveBtn->setStyleSheet(btnStyle + "QPushButton { background: #3a5a3a; color: #d0d0d0; }");
    pbox->addWidget(saveBtn);

    auto* testBtn = new QPushButton(QString::fromUtf8("▶ 测试游玩"), panel);
    testBtn->setStyleSheet(btnStyle + "QPushButton { background: #5a3a1a; color: #ffd; font-weight: bold; }");
    pbox->addWidget(testBtn);

    auto* clearBtn = new QPushButton(QString::fromUtf8("🔄 清空当前层"), panel);
    clearBtn->setStyleSheet(btnStyle + "QPushButton { background: #5a3a3a; color: #d0d0d0; }");
    pbox->addWidget(clearBtn);

    pbox->addSpacing(10);

    // ====== 信号连接 ======

    // 楼层切换
    connect(m_floorSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MapEditor::onFloorChanged);
    connect(prevBtn, &QPushButton::clicked, this, [this]() {
        if (m_floorSpin->value() > 1) m_floorSpin->setValue(m_floorSpin->value() - 1);
    });
    connect(nextBtn, &QPushButton::clicked, this, [this]() {
        if (m_floorSpin->value() < m_floorSpin->maximum())
            m_floorSpin->setValue(m_floorSpin->value() + 1);
    });
    connect(addFloorBtn, &QPushButton::clicked, this, &MapEditor::addFloor);
    connect(m_removeFloorBtn, &QPushButton::clicked, this, &MapEditor::removeFloor);

    // 图块切换
    connect(m_tileGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &MapEditor::onTileTypeChanged);

    // 怪物选择
    connect(m_monsterCombo, &QComboBox::currentTextChanged, this, [this](const QString& name) {
        m_edit->setCurrentMonster(name.toStdString());
    });

    // 道具选择
    connect(m_itemCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        QString name = m_itemCombo->currentText();
        bool isSpecial = false;
        for (int i = 0; g_specialItems[i]; ++i)
            if (name == QString::fromUtf8(g_specialItems[i])) { isSpecial = true; break; }

        m_itemValueLabel->setVisible(!isSpecial);
        m_itemValueSpin->setVisible(!isSpecial);
        int val = isSpecial ? 0 : m_itemValueSpin->value();
        m_edit->setCurrentItem(name.toStdString(), val);
    });
    connect(m_itemValueSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (m_tileGroup->checkedId() == Tile_Item)
            m_edit->setCurrentItem(m_itemCombo->currentText().toStdString(), val);
    });

    // NPC 编辑 → 实时更新
    auto updateNpc = [this]() {
        if (m_tileGroup->checkedId() == Tile_NPC)
            m_edit->setCurrentNPC(m_npcNameEdit->text().toStdString());
    };
    connect(m_npcNameEdit, &QLineEdit::textChanged, this, updateNpc);

    // 编辑区信号
    connect(m_edit, &MapEditWidget::tileChanged, this, [this](int x, int y) {
        m_statusLabel->setText(QString::fromUtf8("已编辑: (%1, %2)").arg(x).arg(y));
    });
    connect(m_edit, &MapEditWidget::playerMoved, this, [this](int x, int y) {
        m_statusLabel->setText(QString::fromUtf8("玩家位置: (%1, %2)").arg(x).arg(y));
    });

    // 按钮
    connect(newBtn,   &QPushButton::clicked, this, &MapEditor::onNewMap);
    connect(saveBtn,  &QPushButton::clicked, this, &MapEditor::onSave);
    connect(loadBtn,  &QPushButton::clicked, this, &MapEditor::onLoad);
    connect(testBtn,  &QPushButton::clicked, this, &MapEditor::onTestPlay);
    connect(clearBtn, &QPushButton::clicked, m_edit, &MapEditWidget::clearFloor);

    // 初始状态
    m_edit->setCurrentMonster(m_monsterCombo->currentText().toStdString());
    m_edit->setCurrentItem(m_itemCombo->currentText().toStdString(), m_itemValueSpin->value());
    m_edit->setCurrentNPC(m_npcNameEdit->text().toStdString());
    m_floors[1] = m_edit->floorData();
    updatePanelForTile(Tile_Wall);
}

// ====== 楼层管理 ======

void MapEditor::storeCurrentFloor()
{
    m_floors[m_currentFloor] = m_edit->floorData();
}

void MapEditor::switchToFloor(int floor)
{
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

void MapEditor::onFloorChanged(int floor)
{
    switchToFloor(floor);
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
    switchToFloor(nearest);
}

void MapEditor::updatePanelForTile(int tileType)
{
    bool isMonster = (tileType == Tile_Monster);
    bool isItem    = (tileType == Tile_Item);
    bool isNPC     = (tileType == Tile_NPC);

    m_monsterCombo->parentWidget()->parentWidget()->setVisible(isMonster); // monster group
    m_itemPanel->parentWidget()->parentWidget()->setVisible(isItem);      // item group
    m_npcPanel->parentWidget()->parentWidget()->setVisible(isNPC);        // npc group

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
    if (name == "Potion") return std::make_unique<Potion>(value);
    if (name == "Weapon") return std::make_unique<Weapon>(value);
    if (name == "Armor")  return std::make_unique<Armor>(value);
    if (name == "Treasure") return std::make_unique<Treasure>(value);
    if (name == "Red Key")  return std::make_unique<Key>(KeyType::Red);
    if (name == "Blue Key") return std::make_unique<Key>(KeyType::Blue);
    if (name == "Green Key") return std::make_unique<Key>(KeyType::Green);
    if (name == QString::fromUtf8("万能钥匙").toStdString()) return std::make_unique<MagicKey>();
    if (name == QString::fromUtf8("匿名眼镜").toStdString()) return std::make_unique<AnonGlasses>();
    if (name == QString::fromUtf8("破墙锤").toStdString())   return std::make_unique<WallBreaker>();
    if (name == QString::fromUtf8("上楼器").toStdString())   return std::make_unique<StairUpper>();
    if (name == QString::fromUtf8("下楼器").toStdString())   return std::make_unique<StairLower>();
    if (name == QString::fromUtf8("临时护盾").toStdString()) return std::make_unique<TempShield>();
    if (name == QString::fromUtf8("企鹅玩偶").toStdString()) return std::make_unique<PenguinDoll>();
    if (name == QString::fromUtf8("抹茶芭菲").toStdString()) return std::make_unique<MatchaParfait>();
    return nullptr;
}

void MapEditor::onSave()
{
    QString path = QFileDialog::getSaveFileName(this,
        QString::fromUtf8("保存地图"), QString(),
        QString::fromUtf8("魔塔存档 (*.txt)"));
    if (path.isEmpty()) return;

    storeCurrentFloor();

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("无法写入文件"));
        return;
    }

    QTextStream out(&file);
    out << "15 15\n";
    out << "1\n";                        // current floor = 1
    out << m_floors.size() << "\n";      // floor count

    for (auto& fp : m_floors) {
        int fnum = fp.first;
        auto& ef = fp.second;
        out << fnum << "\n";

        // 地图网格
        for (int y = 0; y < 15; ++y) {
            for (int x = 0; x < 15; ++x) {
                out << ef.tiles[y * 15 + x].type;
                if (x < 14) out << " ";
            }
            out << "\n";
        }

        // 地面物品（含钥匙）
        int itemCount = 0;
        for (auto& t : ef.tiles)
            if (t.type == Tile_Item && !t.itemName.empty())
                ++itemCount;
        out << itemCount << "\n";
        for (int y = 0; y < 15; ++y) {
            for (int x = 0; x < 15; ++x) {
                auto& t = ef.tiles[y * 15 + x];
                if (t.type == Tile_Item && !t.itemName.empty())
                    out << x << " " << y << " " << QString::fromStdString(t.itemName)
                        << " " << t.itemValue << "\n";
            }
        }

        // 怪物
        int monCount = 0;
        for (auto& t : ef.tiles)
            if (t.type == Tile_Monster && !t.monsterName.empty())
                ++monCount;
        out << monCount << "\n";
        for (int y = 0; y < 15; ++y) {
            for (int x = 0; x < 15; ++x) {
                auto& t = ef.tiles[y * 15 + x];
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
        for (auto& t : ef.tiles)
            if (t.type == Tile_NPC && !t.npcName.empty())
                ++npcCount;
        out << npcCount << "\n";
        for (int y = 0; y < 15; ++y) {
            for (int x = 0; x < 15; ++x) {
                auto& t = ef.tiles[y * 15 + x];
                if (t.type == Tile_NPC && !t.npcName.empty()) {
                    out << x << " " << y << " " << QString::fromStdString(t.npcName) << " "
                        << "0 " << t.npcDialog.size() << " "
                        << (t.npcRewardItem.empty() ? "-" : QString::fromStdString(t.npcRewardItem)) << " "
                        << t.npcRewardValue << "\n";
                    for (auto& d : t.npcDialog)
                        out << QString::fromStdString(d) << "\n";
                }
            }
        }
    }

    // 玩家（Floor 1）
    auto& f1 = m_floors[1];
    out << f1.playerX << " " << f1.playerY << " 100 10 5 0\n";  // x y hp atk def gold
    out << "0 0 0\n";    // keys
    out << "0 0 0\n";    // special flags
    out << "0\n";         // inventory count

    file.close();
    m_statusLabel->setText(QString::fromUtf8("已保存: %1").arg(path));
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
        m_floors[1] = ef;
        m_floorSpin->setMaximum(1);
        switchToFloor(1);
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
    int mw, mh; in >> mw >> mh;
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
            in.readLine(); // skip rest of line
            auto& t = ef.tiles[ny * 15 + nx];
            t.npcName = nname.toStdString();
            if (rewardName != "-") {
                t.npcRewardItem = rewardName.toStdString();
                t.npcRewardValue = rewardValue;
            }
            for (int d = 0; d < dsize; ++d) {
                QString line = in.readLine();
                t.npcDialog.push_back(line.toStdString());
            }
        }

        m_floors[fnum] = ef;
    }

    // 玩家位置
    int px, py, php, patk, pdef, pgold;
    in >> px >> py >> php >> patk >> pdef >> pgold;
    if (m_floors.find(1) != m_floors.end()) {
        m_floors[1].playerX = px;
        m_floors[1].playerY = py;
    }

    int maxFloor = 1;
    for (auto& kv : m_floors) if (kv.first > maxFloor) maxFloor = kv.first;
    m_floorSpin->setMaximum(std::max(maxFloor, 1));
    switchToFloor(1);
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
                    game->addNPCAt(x, y, NPC(t.npcName, t.npcDialog, std::move(reward)));
                }
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
