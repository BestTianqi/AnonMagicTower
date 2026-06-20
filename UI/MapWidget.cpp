#include "MapWidget.h"
#include "Entities/MonsterDB.h"
#include <QPainter>
#include <QFont>

MapWidget::MapWidget(Game* game, QWidget* parent)
    : QWidget(parent), m_game(game)
{
    setFixedSize(900, 900);
    generatePlaceholders();
}

static QPixmap makePixmap(const QColor& fill, const QColor& border,
                          const QString& text, const QColor& textColor = Qt::white,
                          int fontSize = 12)
{
    QPixmap px(TILE_SIZE, TILE_SIZE);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);
    QRect r(0, 0, TILE_SIZE, TILE_SIZE);
    QRect inner = r.adjusted(2, 2, -2, -2);
    p.setBrush(fill);
    p.setPen(QPen(border, 2));
    p.drawRoundedRect(inner, 4, 4);
    if (!text.isEmpty()) {
        QFont f;
        f.setPixelSize(fontSize);
        f.setBold(true);
        p.setFont(f);
        p.setPen(textColor);
        p.drawText(r, Qt::AlignCenter, text);
    }
    p.end();
    return px;
}

void MapWidget::generatePlaceholders()
{
    // 地板
    m_tilePix[Tile_Floor] = makePixmap(QColor(180, 170, 150), QColor(150, 140, 120), "");
    // 墙壁
    m_tilePix[Tile_Wall] = makePixmap(QColor(55, 55, 60), QColor(40, 40, 45), "");
    // 暗墙（无眼镜时和墙一样）
    m_tilePix[Tile_DarkWall] = makePixmap(QColor(55, 55, 60), QColor(40, 40, 45), "");
    // 暗墙（有眼镜时变浅）
    m_darkWallRevealed = makePixmap(QColor(100, 95, 85), QColor(75, 70, 60), "暗", QColor(180, 180, 160), 10);
    // 上楼
    m_tilePix[Tile_StairsUp] = makePixmap(QColor(180, 160, 50), QColor(140, 120, 30),
        QString::fromUtf8("↑"), Qt::black, 20);
    // 下楼
    m_tilePix[Tile_StairsDown] = makePixmap(QColor(160, 100, 180), QColor(120, 70, 140),
        QString::fromUtf8("↓"), Qt::white, 20);
    // 道具
    m_tilePix[Tile_Item] = makePixmap(QColor(60, 170, 60), QColor(40, 130, 40),
        QString::fromUtf8("✦"), QColor(255, 255, 100), 16);
    // 红门
    m_tilePix[Tile_DoorRed] = makePixmap(QColor(180, 60, 50), QColor(140, 30, 20),
        QString::fromUtf8("红门"), Qt::white, 10);
    // 蓝门
    m_tilePix[Tile_DoorBlue] = makePixmap(QColor(50, 70, 180), QColor(30, 40, 140),
        QString::fromUtf8("蓝门"), Qt::white, 10);
    // 绿门
    m_tilePix[Tile_DoorGreen] = makePixmap(QColor(50, 160, 70), QColor(30, 120, 40),
        QString::fromUtf8("绿门"), Qt::white, 10);
    // NPC
    m_tilePix[Tile_NPC] = makePixmap(QColor(200, 160, 60), QColor(160, 120, 30),
        QString::fromUtf8("NPC"), Qt::white, 10);
    // 商店
    m_tilePix[Tile_Shop] = makePixmap(QColor(240, 200, 20), QColor(200, 160, 10),
        QString::fromUtf8("商店"), QColor(80, 40, 0), 10);

    // 怪物（按名称生成占位图，显示名字+数值）
    for (auto& m : MonsterDB::all()) {
        QPixmap px(TILE_SIZE, TILE_SIZE);
        px.fill(Qt::transparent);
        {
            QPainter p(&px);
            p.setRenderHint(QPainter::Antialiasing);
            QRect inner(2, 2, TILE_SIZE - 4, TILE_SIZE - 4);
            p.setBrush(QColor(200, 80, 80));
            p.setPen(QPen(QColor(160, 50, 50), 2));
            p.drawRoundedRect(inner, 4, 4);

            QFont f;
            // 名字
            f.setPixelSize(12);
            f.setBold(true);
            p.setFont(f);
            p.setPen(Qt::white);
            p.drawText(QRect(0, 3, TILE_SIZE, 18), Qt::AlignHCenter | Qt::AlignTop,
                QString::fromStdString(m.GetName()));

            // 数值
            f.setPixelSize(9);
            f.setBold(false);
            p.setFont(f);
            p.setPen(QColor(240, 240, 200));
            p.drawText(QRect(2, 24, TILE_SIZE - 4, 16), Qt::AlignHCenter | Qt::AlignTop,
                QString("HP:%1 ATK:%2").arg(m.GetHP()).arg(m.GetATK()));
            p.drawText(QRect(2, 38, TILE_SIZE - 4, 16), Qt::AlignHCenter | Qt::AlignTop,
                QString("DEF:%1 G:%2").arg(m.GetDEF()).arg(m.GetGold()));
        }
        px.detach();
        m_monsterPix[m.GetName()] = px;
    }
    // 默认怪物（用于未匹配的怪物）
    m_defaultMonsterPix = makePixmap(QColor(200, 80, 80), QColor(160, 50, 50),
        QString::fromUtf8("怪"), Qt::white, 14);

    // 默认道具
    m_defaultItemPix = m_tilePix[Tile_Item];

    // 玩家
    m_playerPix = makePixmap(QColor(60, 130, 240), QColor(30, 80, 180),
        QString::fromUtf8("勇"), Qt::white, 18);
}

void MapWidget::loadTileImage(int tileType, const QString& path)
{
    QPixmap px(path);
    if (!px.isNull())
        m_tilePix[tileType] = px.scaled(TILE_SIZE, TILE_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void MapWidget::loadMonsterImage(const std::string& name, const QString& path)
{
    QPixmap px(path);
    if (!px.isNull())
        m_monsterPix[name] = px.scaled(TILE_SIZE, TILE_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void MapWidget::loadPlayerImage(const QString& path)
{
    QPixmap px(path);
    if (!px.isNull())
        m_playerPix = px.scaled(TILE_SIZE, TILE_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QSize MapWidget::sizeHint() const
{
    return QSize(900, 900);
}

// 根据道具名称返回对应颜色、标签和数值描述
static void itemAppearance(const std::string& name, int value, QColor& fill, QColor& border,
                           QString& label, QString& desc, QColor& textColor)
{
    QString qname = QString::fromStdString(name);

    // 钥匙类
    if (qname == QString::fromUtf8("Red Key") || qname == QString::fromUtf8("红钥匙"))
        { fill = QColor(200, 45, 45); border = QColor(160, 20, 20);
          label = QString::fromUtf8("红钥"); textColor = QColor(255, 220, 100); return; }
    if (qname == QString::fromUtf8("Blue Key") || qname == QString::fromUtf8("蓝钥匙"))
        { fill = QColor(45, 60, 200); border = QColor(20, 30, 160);
          label = QString::fromUtf8("蓝钥"); textColor = QColor(255, 220, 100); return; }
    if (qname == QString::fromUtf8("Green Key") || qname == QString::fromUtf8("绿钥匙"))
        { fill = QColor(45, 180, 60); border = QColor(20, 140, 30);
          label = QString::fromUtf8("绿钥"); textColor = QColor(255, 220, 100); return; }
    if (qname == QString::fromUtf8("万能钥匙"))
        { fill = QColor(130, 60, 200); border = QColor(90, 30, 160);
          label = QString::fromUtf8("万能钥"); textColor = QColor(255, 220, 100); return; }

    // 属性类
    if (qname == QString::fromUtf8("Potion") || qname == QString::fromUtf8("药水"))
        { fill = QColor(200, 60, 60); border = QColor(150, 30, 30);
          label = QString::fromUtf8("生命药"); textColor = Qt::white;
          desc = QString("+%1HP").arg(value); return; }
    if (qname == QString::fromUtf8("Weapon") || qname == QString::fromUtf8("武器"))
        { fill = QColor(210, 140, 40); border = QColor(160, 100, 20);
          label = QString::fromUtf8("武器"); textColor = Qt::white;
          desc = QString("ATK+%1").arg(value); return; }
    if (qname == QString::fromUtf8("Armor") || qname == QString::fromUtf8("防具"))
        { fill = QColor(60, 120, 200); border = QColor(30, 80, 160);
          label = QString::fromUtf8("防具"); textColor = Qt::white;
          desc = QString("DEF+%1").arg(value); return; }
    if (qname == QString::fromUtf8("Treasure") || qname == QString::fromUtf8("金币"))
        { fill = QColor(220, 180, 40); border = QColor(170, 130, 20);
          label = QString::fromUtf8("金币"); textColor = QColor(100, 60, 0);
          desc = QString("%1G").arg(value); return; }

    // 特殊道具
    if (qname == QString::fromUtf8("匿名眼镜"))
        { fill = QColor(40, 180, 180); border = QColor(20, 130, 130);
          label = QString::fromUtf8("眼镜"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("破墙锤"))
        { fill = QColor(140, 100, 70); border = QColor(100, 70, 40);
          label = QString::fromUtf8("破墙锤"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("上楼器"))
        { fill = QColor(180, 170, 60); border = QColor(140, 130, 30);
          label = QString::fromUtf8("上楼器"); textColor = Qt::black; return; }
    if (qname == QString::fromUtf8("下楼器"))
        { fill = QColor(160, 110, 180); border = QColor(120, 80, 140);
          label = QString::fromUtf8("下楼器"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("临时护盾"))
        { fill = QColor(80, 160, 220); border = QColor(50, 120, 180);
          label = QString::fromUtf8("护盾"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("企鹅玩偶"))
        { fill = QColor(220, 130, 170); border = QColor(170, 80, 120);
          label = QString::fromUtf8("企鹅"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("抹茶芭菲"))
        { fill = QColor(140, 200, 100); border = QColor(90, 150, 50);
          label = QString::fromUtf8("芭菲"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("幸运金币"))
        { fill = QColor(240, 200, 20); border = QColor(200, 150, 10);
          label = QString::fromUtf8("幸运币"); textColor = QColor(80, 40, 0); return; }

    // fallback
    fill = QColor(60, 170, 60); border = QColor(40, 130, 40);
    label = QString::fromUtf8("宝"); textColor = QColor(255, 255, 100);
}

void MapWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    if (!m_game) return;

    int w = m_game->width();
    int h = m_game->height();

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int t = m_game->map()[y * w + x];
            QRect r(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE);

            QPixmap* pix = nullptr;

            if (t == Tile_Monster) {
                Monster* m = m_game->monsterAt(x, y);
                if (m) {
                    auto it = m_monsterPix.find(m->GetName());
                    if (it != m_monsterPix.end())
                        pix = &it->second;
                }
                if (!pix) pix = &m_defaultMonsterPix;
            }

            if (t == Tile_Item && !pix) {
                const Item* item = m_game->itemAt(x, y);
                if (item) {
                    QColor fill, border, textColor;
                    QString label, desc;
                    itemAppearance(item->GetName(), item->GetValue(), fill, border, label, desc, textColor);

                    QPixmap px(TILE_SIZE, TILE_SIZE);
                    px.fill(Qt::transparent);
                    {
                        QPainter p(&px);
                        QRect inner(1, 1, TILE_SIZE - 2, TILE_SIZE - 2);
                        p.fillRect(inner, fill);
                        p.setPen(QPen(QColor(50, 50, 50), 1));
                        p.drawRect(0, 0, TILE_SIZE - 1, TILE_SIZE - 1);

                        QFont f;
                        if (desc.isEmpty()) {
                            f.setPixelSize(label.length() > 2 ? 11 : 14);
                            f.setBold(true);
                            p.setFont(f);
                            p.setPen(Qt::white);
                            p.drawText(QRect(0, 0, TILE_SIZE, TILE_SIZE),
                                Qt::AlignCenter, label);
                        } else {
                            f.setPixelSize(11);
                            f.setBold(true);
                            p.setFont(f);
                            p.setPen(Qt::white);
                            p.drawText(QRect(0, 2, TILE_SIZE, 20),
                                Qt::AlignHCenter | Qt::AlignTop, label);
                            f.setPixelSize(10);
                            f.setBold(false);
                            p.setFont(f);
                            p.drawText(QRect(2, 28, TILE_SIZE - 4, 28), Qt::AlignHCenter | Qt::AlignTop, desc);
                        }
                    }
                    px.detach();
                    painter.drawPixmap(r, px);
                    continue;
                }
            }

            if (t == Tile_DarkWall && m_game->player().hasGlasses)
                pix = &m_darkWallRevealed;

            if (!pix) {
                auto it = m_tilePix.find(t);
                if (it != m_tilePix.end())
                    pix = &it->second;
            }

            if (pix && !pix->isNull())
                painter.drawPixmap(r, *pix);
        }
    }

    // 玩家
    int px = m_game->player().x;
    int py = m_game->player().y;
    QRect pr(px * TILE_SIZE, py * TILE_SIZE, TILE_SIZE, TILE_SIZE);

    if (!m_playerPix.isNull())
        painter.drawPixmap(pr, m_playerPix);
}
