#include "MapWidget.h"
#include <QPainter>

MapWidget::MapWidget(Game* game, QWidget* parent)
    : QWidget(parent), m_game(game)
{
    setFixedSize(900, 900);
}

void MapWidget::loadTileImage(int tileType, const QString& path)
{
    m_tilePix[tileType] = QPixmap(path).scaled(TILE_SIZE, TILE_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void MapWidget::loadMonsterImage(const std::string& name, const QString& path)
{
    m_monsterPix[name] = QPixmap(path).scaled(TILE_SIZE, TILE_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void MapWidget::loadPlayerImage(const QString& path)
{
    m_playerPix = QPixmap(path).scaled(TILE_SIZE, TILE_SIZE, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
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
