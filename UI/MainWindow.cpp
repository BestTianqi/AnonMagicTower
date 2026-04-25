#include "MainWindow.h"
#include <QPainter>
#include <QKeyEvent>

MainWindow::MainWindow(Game* game, QWidget* parent)
    : QWidget(parent), m_game(game)
{
    setWindowTitle("魔塔 - 简易框架");
    resize(640, 480);
}

void MainWindow::paintEvent(QPaintEvent* /* event */)
{
    QPainter painter(this);
    const int tileSize = 48;
    if (!m_game) return;
    int w = m_game->width();
    int h = m_game->height();

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int t = m_game->map()[y*w + x];
            QRect r(x*tileSize, y*tileSize, tileSize, tileSize);
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
    QRect pr(m_game->player().x * tileSize, m_game->player().y * tileSize, tileSize, tileSize);
    painter.drawEllipse(pr);
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    int dx = 0, dy = 0;
    switch (event->key()) {
    case Qt::Key_Left: dx = -1; break;
    case Qt::Key_Right: dx = 1; break;
    case Qt::Key_Up: dy = -1; break;
    case Qt::Key_Down: dy = 1; break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }

    int nx = m_game->player().x + dx;
    int ny = m_game->player().y + dy;
    if (nx < 0 || ny < 0 || nx >= m_game->width() || ny >= m_game->height()) return;
    int tile = m_game->map()[ny * m_game->width() + nx];
    if (tile == Tile_Wall) return; // can't walk

    m_game->player().x = nx;
    m_game->player().y = ny;
    update();
}
