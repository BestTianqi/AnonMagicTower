#include "MapEditor.h"
#include "Entities/MonsterDB.h"
#include "Game/Game.h"

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

// ==================== MapEditWidget ====================

MapEditWidget::MapEditWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(900, 900);
    setMouseTracking(true);

    // 默认全墙
    m_tiles.assign(225, { Tile_Wall, "" });
    // 内部区域设为地板
    for (int y = 1; y < 14; ++y)
        for (int x = 1; x < 14; ++x)
            m_tiles[indexAt(x, y)].type = Tile_Floor;
}

void MapEditWidget::clearMap()
{
    m_tiles.assign(225, { Tile_Wall, "" });
    for (int y = 1; y < 14; ++y)
        for (int x = 1; x < 14; ++x)
            m_tiles[indexAt(x, y)].type = Tile_Floor;
    m_playerX = 1;
    m_playerY = 2;
    update();
}

void MapEditWidget::placeTile(int x, int y)
{
    if (x < 0 || y < 0 || x >= 15 || y >= 15) return;

    // 右键擦除 = 设为地板
    if (m_currentTile == Tile_Empty) {
        m_tiles[indexAt(x, y)].type = Tile_Floor;
        m_tiles[indexAt(x, y)].monsterName.clear();
    } else {
        m_tiles[indexAt(x, y)].type = m_currentTile;
        if (m_currentTile == Tile_Monster)
            m_tiles[indexAt(x, y)].monsterName = m_currentMonster;
        else
            m_tiles[indexAt(x, y)].monsterName.clear();
    }

    update();
    emit tileChanged(x, y);
}

void MapEditWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    // 背景
    painter.fillRect(0, 0, 900, 900, QColor(20, 20, 30));

    // tiles
    for (int y = 0; y < 15; ++y) {
        for (int x = 0; x < 15; ++x) {
            auto& t = m_tiles[indexAt(x, y)];
            QRect r(x * 60, y * 60, 60, 60);
            QRect inner = r.adjusted(1, 1, -1, -1);

            QColor fill;
            QString label;

            switch (t.type) {
            case Tile_Wall:      fill = QColor(55, 55, 60);   break;
            case Tile_Floor:     fill = QColor(180, 170, 150); break;
            case Tile_StairsUp:  fill = QColor(180, 160, 50);  label = QString::fromUtf8("上"); break;
            case Tile_StairsDown:fill = QColor(160, 100, 180); label = QString::fromUtf8("下"); break;
            case Tile_Monster:   fill = QColor(200, 80, 80);
                label = t.monsterName.empty()
                    ? QString::fromUtf8("怪")
                    : QString::fromStdString(t.monsterName);
                break;
            case Tile_Item:      fill = QColor(60, 170, 60);   label = QString::fromUtf8("宝"); break;
            default:             fill = QColor(30, 30, 30);   break;
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

    // 玩家位置
    QRect pr(m_playerX * 60, m_playerY * 60, 60, 60);
    painter.setBrush(QColor(60, 130, 240, 180));
    painter.setPen(QPen(QColor(30, 80, 180), 2));
    painter.drawEllipse(pr.adjusted(8, 8, -8, -8));
    {
        QFont f; f.setPixelSize(16); f.setBold(true);
        painter.setFont(f);
        painter.setPen(Qt::white);
        painter.drawText(pr, Qt::AlignCenter, QString::fromUtf8("勇"));
    }

    // 鼠标悬停高亮
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

    if (event->button() == Qt::LeftButton) {
        // 左键放玩家
        if (event->modifiers() & Qt::ShiftModifier) {
            m_playerX = x;
            m_playerY = y;
            update();
            return;
        }
        placeTile(x, y);
    } else if (event->button() == Qt::RightButton) {
        // 右键擦除
        m_tiles[indexAt(x, y)].type = Tile_Floor;
        m_tiles[indexAt(x, y)].monsterName.clear();
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

            // 按住左键拖拽绘制
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

MapEditor::MapEditor(QWidget* parent)
    : QWidget(parent, Qt::Window)
{
    setWindowTitle(QString::fromUtf8("地图编辑器"));
    resize(1300, 950);
    setMinimumSize(1200, 920);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    // === 左侧: 地图编辑区域 ===
    m_edit = new MapEditWidget(this);
    mainLayout->addWidget(m_edit);

    // === 右侧: 工具面板 ===
    auto* panel = new QWidget(this);
    panel->setFixedWidth(260);
    auto* pbox = new QVBoxLayout(panel);
    pbox->setContentsMargins(8, 8, 8, 8);
    pbox->setSpacing(8);

    // -- 图块选择 --
    auto* tileGroup = new QGroupBox(QString::fromUtf8("图块类型"), panel);
    auto* tbox = new QVBoxLayout(tileGroup);
    m_tileGroup = new QButtonGroup(this);
    m_tileGroup->setExclusive(true);

    struct TileBtn { int type; QString text; };
    TileBtn btns[] = {
        { Tile_Wall,       QString::fromUtf8("墙")   },
        { Tile_Floor,      QString::fromUtf8("地板")  },
        { Tile_StairsUp,   QString::fromUtf8("上楼梯") },
        { Tile_StairsDown, QString::fromUtf8("下楼梯") },
        { Tile_Monster,    QString::fromUtf8("怪物")  },
        { Tile_Item,       QString::fromUtf8("道具")  },
    };

    for (int i = 0; i < 6; ++i) {
        auto* rb = new QRadioButton(btns[i].text, tileGroup);
        m_tileGroup->addButton(rb, btns[i].type);
        tbox->addWidget(rb);
        if (btns[i].type == Tile_Wall) rb->setChecked(true);
    }
    pbox->addWidget(tileGroup);

    // -- 怪物选择 --
    auto* monGroup = new QGroupBox(QString::fromUtf8("怪物类型"), panel);
    auto* mbox = new QVBoxLayout(monGroup);
    m_monsterCombo = new QComboBox(monGroup);
    for (auto& m : MonsterDB::all())
        m_monsterCombo->addItem(QString::fromStdString(m.GetName()));
    mbox->addWidget(m_monsterCombo);
    pbox->addWidget(monGroup);

    // -- 操作提示 --
    auto* hintGroup = new QGroupBox(QString::fromUtf8("操作"), panel);
    auto* hbox = new QVBoxLayout(hintGroup);
    hbox->addWidget(new QLabel(QString::fromUtf8("左键点击: 放置图块"), hintGroup));
    hbox->addWidget(new QLabel(QString::fromUtf8("右键点击: 擦除为地板"), hintGroup));
    hbox->addWidget(new QLabel(QString::fromUtf8("Shift+左键: 放置玩家"), hintGroup));
    hbox->addWidget(new QLabel(QString::fromUtf8("拖拽: 连续绘制"), hintGroup));
    pbox->addWidget(hintGroup);

    // -- 状态 --
    m_statusLabel = new QLabel(QString::fromUtf8("就绪"), panel);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color: #888; font-size: 11px; padding: 4px;");
    pbox->addWidget(m_statusLabel);

    pbox->addStretch();

    // -- 按钮 --
    auto* sep = new QFrame(panel);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #555;");
    pbox->addWidget(sep);

    auto* saveBtn = new QPushButton(QString::fromUtf8("💾 保存地图"), panel);
    saveBtn->setMinimumHeight(36);
    saveBtn->setStyleSheet(
        "QPushButton { background: #3a5a3a; color: #d0d0d0; border-radius: 4px; padding: 6px; }"
        "QPushButton:hover { background: #4a7a4a; }"
    );
    pbox->addWidget(saveBtn);

    auto* loadBtn = new QPushButton(QString::fromUtf8("📂 加载地图"), panel);
    loadBtn->setMinimumHeight(36);
    loadBtn->setStyleSheet(
        "QPushButton { background: #4a4a5a; color: #d0d0d0; border-radius: 4px; padding: 6px; }"
        "QPushButton:hover { background: #5a5a7a; }"
    );
    pbox->addWidget(loadBtn);

    auto* clearBtn = new QPushButton(QString::fromUtf8("🔄 清空地图"), panel);
    clearBtn->setMinimumHeight(36);
    clearBtn->setStyleSheet(
        "QPushButton { background: #5a3a3a; color: #d0d0d0; border-radius: 4px; padding: 6px; }"
        "QPushButton:hover { background: #7a4a4a; }"
    );
    pbox->addWidget(clearBtn);

    mainLayout->addWidget(panel);

    // === 信号连接 ===
    connect(m_tileGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &MapEditor::onTileTypeChanged);

    connect(m_monsterCombo, &QComboBox::currentTextChanged, this, [this](const QString& name) {
        m_edit->setCurrentMonster(name.toStdString());
    });

    connect(m_edit, &MapEditWidget::tileChanged, this, [this](int x, int y) {
        m_statusLabel->setText(QString::fromUtf8("已放置: (%1, %2)").arg(x).arg(y));
    });

    connect(saveBtn,  &QPushButton::clicked, this, &MapEditor::onSave);
    connect(loadBtn,  &QPushButton::clicked, this, &MapEditor::onLoad);
    connect(clearBtn, &QPushButton::clicked, m_edit, &MapEditWidget::clearMap);

    // 初始怪物选择
    m_edit->setCurrentMonster(m_monsterCombo->currentText().toStdString());
}

void MapEditor::onTileTypeChanged(int id)
{
    m_edit->setCurrentTile(id);
    m_monsterCombo->setEnabled(id == Tile_Monster);
}

void MapEditor::onSave()
{
    QString path = QFileDialog::getSaveFileName(this,
        QString::fromUtf8("保存地图"),
        QString(),
        QString::fromUtf8("地图文件 (*.map)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("无法写入文件"));
        return;
    }

    QTextStream out(&file);
    out << "15 15\n";

    // tile grid
    auto& tiles = m_edit->tiles();
    for (int y = 0; y < 15; ++y) {
        for (int x = 0; x < 15; ++x) {
            out << tiles[y * 15 + x].type;
            if (x < 14) out << " ";
        }
        out << "\n";
    }

    // player position
    out << m_edit->playerX() << " " << m_edit->playerY() << "\n";

    // monsters
    int monCount = 0;
    for (auto& t : tiles)
        if (t.type == Tile_Monster && !t.monsterName.empty())
            ++monCount;

    out << monCount << "\n";
    for (int y = 0; y < 15; ++y) {
        for (int x = 0; x < 15; ++x) {
            auto& t = tiles[y * 15 + x];
            if (t.type == Tile_Monster && !t.monsterName.empty()) {
                Monster m = MonsterDB::get(t.monsterName);
                out << x << " " << y << " " << QString::fromStdString(m.GetName()) << " "
                    << m.GetHP() << " " << m.GetATK() << " "
                    << m.GetDEF() << " " << m.GetGold() << "\n";
            }
        }
    }

    file.close();
    m_statusLabel->setText(QString::fromUtf8("已保存: %1").arg(path));
}

void MapEditor::onLoad()
{
    QString path = QFileDialog::getOpenFileName(this,
        QString::fromUtf8("加载地图"),
        QString(),
        QString::fromUtf8("地图文件 (*.map)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("无法打开文件"));
        return;
    }

    QTextStream in(&file);
    int w, h;
    in >> w >> h;
    if (w != 15 || h != 15) {
        QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("仅支持 15x15 地图"));
        return;
    }

    std::vector<EditorTile> tiles(225);
    for (int y = 0; y < 15; ++y)
        for (int x = 0; x < 15; ++x)
            in >> tiles[y * 15 + x].type;

    int px, py;
    in >> px >> py;

    int monCount;
    in >> monCount;
    for (int i = 0; i < monCount; ++i) {
        int mx, my, mhp, matk, mdef, mgold;
        QString mname;
        in >> mx >> my >> mname >> mhp >> matk >> mdef >> mgold;
        tiles[my * 15 + mx].monsterName = mname.toStdString();
    }

    m_edit->setTiles(tiles);
    m_edit->setPlayerPos(px, py);
    m_statusLabel->setText(QString::fromUtf8("已加载: %1").arg(path));
}
