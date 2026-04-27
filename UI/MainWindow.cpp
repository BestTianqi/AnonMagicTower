#include "MainWindow.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMessageBox>
#include <QString>

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

    // draw key counts on top-right
    painter.setPen(Qt::black);
    painter.setBrush(Qt::NoBrush);
    int rx = width() - 150;
    int ry = 10;
    QString keyText = QString("R:%1  B:%2  G:%3")
            .arg(m_game->player().KeyCount(KeyType::Red))
            .arg(m_game->player().KeyCount(KeyType::Blue))
            .arg(m_game->player().KeyCount(KeyType::Green));
    painter.drawText(rx, ry + 12, keyText);
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
    auto result = m_game->tryMovePlayer(nx, ny);
    switch (result) {
    case Game::Move_Block:
        // nothing
        break;
    case Game::Move_Ok:
        // moved
        update();
        break;
    case Game::Move_Pickup:
        update();
        QMessageBox::information(this, "拾取", "你获得了物品（示例：红钥匙）");
        break;
    case Game::Move_Encounter:
        update();
        QMessageBox::information(this, "遭遇", "遇到怪物！（战斗尚未实现）");
        break;
    case Game::Move_StairsUp:
        update();
        QMessageBox::information(this, "楼梯", "上楼（示例响应）");
        break;
    case Game::Move_StairsDown:
        update();
        QMessageBox::information(this, "楼梯", "下楼（示例响应）");
        break;
    }
}
