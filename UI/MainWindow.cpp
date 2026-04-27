#include "MainWindow.h"
#include "MapWidget.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMessageBox>
#include <QString>
#include <QFileDialog>
#include <QDir>
#include <QLayout>

MainWindow::MainWindow(Game* game, QWidget* parent)
    : QWidget(parent), m_game(game)
{
    ui.setupUi(this);
    setWindowTitle("魔塔 - 简易框架");
    resize(800, 600);

    // replace placeholder with MapWidget instance
    MapWidget* mapW = new MapWidget(m_game, this);
    // find layout item and replace
    QLayout* lay = ui.mapWidget->parentWidget()->layout();
    if (lay) {
        // find index of placeholder widget
        for (int i = 0; i < lay->count(); ++i) {
            QLayoutItem* it = lay->itemAt(i);
            if (it && it->widget() == ui.mapWidget) {
                // remove placeholder and insert mapW
                QWidget* placeholder = ui.mapWidget;
                lay->replaceWidget(placeholder, mapW);
                placeholder->deleteLater();
                break;
            }
        }
    }

    // ensure pointer in ui now points to mapW
    ui.mapWidget = mapW;

    connect(ui.saveButton, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getSaveFileName(this, "保存存档", QDir::currentPath(), "保存文件 (*.txt)");
        if (!file.isEmpty()) {
            bool ok = m_game->saveToFile(file.toStdString());
            QMessageBox::information(this, "保存", ok ? "保存成功" : "保存失败");
        }
    });

    connect(ui.loadButton, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, "读取存档", QDir::currentPath(), "保存文件 (*.txt)");
        if (!file.isEmpty()) {
            bool ok = m_game->loadFromFile(file.toStdString());
            if (ok) {
                update();
                QMessageBox::information(this, "读取", "读取成功");
            } else {
                QMessageBox::warning(this, "读取", "读取失败");
            }
        }
    });
}

// 把原来的 paintEvent 改为在 ui.mapWidget 上绘制。如果 mapWidget 不方便，可保持原逻辑。下面仍使用 MainWindow::paintEvent 绘制整个窗口中的地图区域。

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

    // update side panel labels
    ui.hpLabel->setText(QString("HP: %1").arg(m_game->player().hp));
    ui.goldLabel->setText(QString("Gold: %1").arg(m_game->player().gold));
    ui.keysLabel->setText(QString("Keys: R%1 B%2 G%3")
            .arg(m_game->player().KeyCount(KeyType::Red))
            .arg(m_game->player().KeyCount(KeyType::Blue))
            .arg(m_game->player().KeyCount(KeyType::Green)));
}

// keyPressEvent 保持不变但调用 update() 来刷新 UI

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
    case Game::Move_Encounter: {
        update();
        // perform fight in-game with log
        int tx = nx, ty = ny;
        std::vector<std::string> log;
        auto fightRes = m_game->fightAt(tx, ty, log);

        // compose log into QString
        QString dlg;
        for (const auto &s : log) {
            dlg += QString::fromStdString(s) + "\n";
        }

        if (fightRes == Game::Fight_PlayerWin) {
            update();
            QMessageBox::information(this, "战斗", dlg);
        } else {
            QMessageBox::critical(this, "战斗", dlg);
        }
        break;
    }
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
