#include "MapWidget.h"
#include <QPainter>

MapWidget::MapWidget(Game* game, QWidget* parent)
    : QWidget(parent), m_game(game)
{
}

QSize MapWidget::sizeHint() const
{
    if (!m_game) return QWidget::sizeHint();
    return QSize(m_game->width() * m_tileSize, m_game->height() * m_tileSize);
}

void MapWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    if (!m_game) return;
    int w = m_game->width();
    int h = m_game->height();

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int t = m_game->map()[y*w + x];
            QRect r(x*m_tileSize, y*m_tileSize, m_tileSize, m_tileSize);
            switch (t) {
            case Tile_Wall:
                painter.fillRect(r, Qt::darkGray);
                break;
            case Tile_Floor:
                painter.fillRect(r, Qt::lightGray);
                break;
            case Tile_StairsUp:
                painter.fillRect(r, Qt::yellow);
                break;
            case Tile_StairsDown:
                painter.fillRect(r, Qt::magenta);
                break;
            case Tile_Monster:
                painter.fillRect(r, Qt::red);
                break;
            case Tile_Item:
                painter.fillRect(r, Qt::green);
                break;
            default:
                painter.fillRect(r, Qt::black);
            }
            painter.drawRect(r);
        }
    }

    // draw player
    painter.setBrush(Qt::blue);
    QRect pr(m_game->player().x * m_tileSize, m_game->player().y * m_tileSize, m_tileSize, m_tileSize);
    painter.drawEllipse(pr);
}
