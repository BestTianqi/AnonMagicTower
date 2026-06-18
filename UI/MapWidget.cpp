#include "MapWidget.h"
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

    // 默认怪物
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
