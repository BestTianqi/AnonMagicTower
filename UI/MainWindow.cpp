#include "MainWindow.h"
#include "MapWidget.h"
#include "Entities/MonsterDB.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMessageBox>
#include <QString>
#include <QFileDialog>
#include <QDir>

MainWindow::MainWindow(Game* game, QWidget* parent)
    : QWidget(parent), m_game(game), m_floor(1)
{
    ui.setupUi(this);
    setWindowTitle(QString::fromUtf8("魔塔"));

    ui.mapWidget->setGame(m_game);
    updateHUD();

    connect(ui.saveButton, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getSaveFileName(this, QString::fromUtf8("保存存档"),
            QDir::currentPath(), QString::fromUtf8("保存文件 (*.txt)"));
        if (!file.isEmpty()) {
            bool ok = m_game->saveToFile(file.toStdString());
            QMessageBox::information(this, QString::fromUtf8("保存"), ok ? QString::fromUtf8("保存成功") : QString::fromUtf8("保存失败"));
        }
    });

    connect(ui.loadButton, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, QString::fromUtf8("读取存档"),
            QDir::currentPath(), QString::fromUtf8("保存文件 (*.txt)"));
        if (!file.isEmpty()) {
            bool ok = m_game->loadFromFile(file.toStdString());
            if (ok) {
                m_floor = 1;
                ui.mapWidget->update();
                updateHUD();
                QMessageBox::information(this, QString::fromUtf8("读取"), QString::fromUtf8("读取成功"));
            } else {
                QMessageBox::warning(this, QString::fromUtf8("读取"), QString::fromUtf8("读取失败"));
            }
        }
    });
}

void MainWindow::loadAssets()
{
    auto* mw = ui.mapWidget;

    // 地砖
    mw->loadTileImage(Tile_Wall,       ":/images/wall.png");
    mw->loadTileImage(Tile_Floor,      ":/images/floor.png");
    mw->loadTileImage(Tile_StairsUp,   ":/images/stairs_up.png");
    mw->loadTileImage(Tile_StairsDown, ":/images/stairs_down.png");
    mw->loadTileImage(Tile_Item,       ":/images/item.png");

    // 玩家
    mw->loadPlayerImage(":/images/player.png");

    // 18种怪物
    auto monsters = MonsterDB::all();
    for (size_t i = 0; i < monsters.size(); ++i) {
        QString path = QString(":/images/monster_%1.png").arg(i + 1, 2, 10, QChar('0'));
        mw->loadMonsterImage(monsters[i].GetName(), path);
    }

    mw->update();
}

void MainWindow::updateHUD()
{
    ui.floorLabel->setText(QString::fromUtf8("第 %1 层").arg(m_floor));
    ui.hpLabel->setText(QString::fromUtf8("❤ 生命: %1").arg(m_game->player().hp));
    ui.atkLabel->setText(QString::fromUtf8("⚔ 攻击: %1").arg(m_game->player().atk));
    ui.defLabel->setText(QString::fromUtf8("🛡 防御: %1").arg(m_game->player().def));
    ui.goldLabel->setText(QString::fromUtf8("💰 金币: %1").arg(m_game->player().gold));
    ui.keysLabel->setText(QString::fromUtf8("🔑 钥匙: 红%1 蓝%2 绿%3")
        .arg(m_game->player().KeyCount(KeyType::Red))
        .arg(m_game->player().KeyCount(KeyType::Blue))
        .arg(m_game->player().KeyCount(KeyType::Green)));
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    int dx = 0, dy = 0;
    switch (event->key()) {
    case Qt::Key_Left:  dx = -1; break;
    case Qt::Key_Right: dx =  1; break;
    case Qt::Key_Up:    dy = -1; break;
    case Qt::Key_Down:  dy =  1; break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }

    int nx = m_game->player().x + dx;
    int ny = m_game->player().y + dy;
    auto result = m_game->tryMovePlayer(nx, ny);

    switch (result) {
    case Game::Move_Block:
        break;
    case Game::Move_Ok:
        ui.mapWidget->update();
        break;
    case Game::Move_Pickup:
        ui.mapWidget->update();
        updateHUD();
        break;
    case Game::Move_Encounter: {
        ui.mapWidget->update();
        std::vector<std::string> log;
        auto fightRes = m_game->fightAt(nx, ny, log);

        QString dlg;
        for (const auto& s : log)
            dlg += QString::fromStdString(s) + "\n";

        if (fightRes == Game::Fight_PlayerWin) {
            ui.mapWidget->update();
            updateHUD();
            QMessageBox::information(this, QString::fromUtf8("战斗"), dlg);
        } else {
            QMessageBox::critical(this, QString::fromUtf8("战斗"), dlg);
            updateHUD();
        }
        break;
    }
    case Game::Move_StairsUp:
        m_floor++;
        ui.mapWidget->update();
        updateHUD();
        break;
    case Game::Move_StairsDown:
        if (m_floor > 1) m_floor--;
        ui.mapWidget->update();
        updateHUD();
        break;
    case Game::Move_PlayerDead:
        updateHUD();
        break;
    }
}
