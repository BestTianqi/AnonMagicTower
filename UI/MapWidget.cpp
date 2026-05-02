#include "MapWidget.h"
#include <QPainter>

MapWidget::MapWidget(Game* game, QWidget* parent)
    : QWidget(parent), m_game(game)
{
    setMinimumSize(320, 240);
}

void MapWidget::loadTileImage(int tileType, const QString& path)
{
    m_tilePix[tileType] = QPixmap(path);
}

void MapWidget::loadMonsterImage(const std::string& name, const QString& path)
{
    m_monsterPix[name] = QPixmap(path);
}

void MapWidget::loadPlayerImage(const QString& path)
{
    m_playerPix = QPixmap(path);
}

QSize MapWidget::sizeHint() const
{
    if (!m_game) return QSize(640, 480);
    int ts = tileSize();
    return QSize(m_game->width() * ts, m_game->height() * ts);
}

int MapWidget::tileSize() const
{
    if (!m_game || m_game->width() <= 0 || m_game->height() <= 0)
        return 48;
    int tsW = width()  / m_game->width();
    int tsH = height() / m_game->height();
    return qMax(qMin(tsW, tsH), 24);
}

void MapWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    update();
}

void MapWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    if (!m_game) return;

    int w = m_game->width();
    int h = m_game->height();
    int ts = tileSize();

    int ox = (width()  - w * ts) / 2;
    int oy = (height() - h * ts) / 2;
    painter.translate(ox, oy);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int t = m_game->map()[y * w + x];
            QRect r(x * ts, y * ts, ts, ts);

            QPixmap* pix = nullptr;

            if (t == Tile_Monster) {
                // 有名字匹配的怪物图就用，否则用默认怪物图
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
    QRect pr(px * ts, py * ts, ts, ts);

    if (!m_playerPix.isNull())
        painter.drawPixmap(pr, m_playerPix);
}
