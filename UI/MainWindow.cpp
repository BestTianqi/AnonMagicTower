#include "MainWindow.h"
#include "MapWidget.h"
#include "MapEditor.h"
#include "Entities/MonsterDB.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMessageBox>
#include <QString>
#include <QFileDialog>
#include <QDir>
#include <QDialog>
#include <QVBoxLayout>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFrame>

MainWindow::MainWindow(Game* game, QWidget* parent)
    : QWidget(parent), m_game(game)
{
    setFocusPolicy(Qt::StrongFocus);
    ui.setupUi(this);
    setWindowTitle(QString::fromUtf8("魔塔"));

    ui.mapWidget->setGame(m_game);
    ui.mapWidget->setFocusPolicy(Qt::NoFocus);
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
                ui.mapWidget->update();
                updateHUD();
                QMessageBox::information(this, QString::fromUtf8("读取"), QString::fromUtf8("读取成功"));
            } else {
                QMessageBox::warning(this, QString::fromUtf8("读取"), QString::fromUtf8("读取失败"));
            }
        }
    });

    connect(ui.editorButton, &QPushButton::clicked, this, [this]() {
        auto* editor = new MapEditor();
        editor->setAttribute(Qt::WA_DeleteOnClose);
        editor->show();
    });

    connect(ui.invButton, &QPushButton::clicked, this, &MainWindow::showInventory);
}

void MainWindow::loadAssets()
{
    auto* mw = ui.mapWidget;

    // 尝试加载真实图片（如果不存在则使用占位图）
    mw->loadTileImage(Tile_Wall,       ":/images/wall.png");
    mw->loadTileImage(Tile_Floor,      ":/images/floor.png");
    mw->loadTileImage(Tile_StairsUp,   ":/images/stairs_up.png");
    mw->loadTileImage(Tile_StairsDown, ":/images/stairs_down.png");
    mw->loadTileImage(Tile_Item,       ":/images/item.png");
    mw->loadTileImage(Tile_DoorRed,    ":/images/door_red.png");
    mw->loadTileImage(Tile_DoorBlue,   ":/images/door_blue.png");
    mw->loadTileImage(Tile_DoorGreen,  ":/images/door_green.png");
    mw->loadTileImage(Tile_NPC,        ":/images/npc.png");
    mw->loadTileImage(Tile_Shop,       ":/images/shop.png");
    mw->loadTileImage(Tile_DarkWall,   ":/images/dark_wall.png");

    mw->loadPlayerImage(":/images/player.png");

    auto monsters = MonsterDB::all();
    for (size_t i = 0; i < monsters.size(); ++i) {
        QString path = QString(":/images/monster_%1.png").arg(i + 1, 2, 10, QChar('0'));
        mw->loadMonsterImage(monsters[i].GetName(), path);
    }

    mw->update();
}

QString MainWindow::getItemDescription(const Item* item) const
{
    if (!item) return QString::fromUtf8("(空)");

    QString name = QString::fromStdString(item->GetName());
    int val = item->GetValue();

    if (name == "Potion")
        return QString::fromUtf8("恢复 %1 点生命值").arg(val);
    if (name == "Weapon")
        return QString::fromUtf8("攻击力 +%1").arg(val);
    if (name == "Armor")
        return QString::fromUtf8("防御力 +%1").arg(val);
    if (name == "Treasure")
        return QString::fromUtf8("获得 %1 金币").arg(val);
    if (name == "Red Key")
        return QString::fromUtf8("红钥匙 ×1");
    if (name == "Blue Key")
        return QString::fromUtf8("蓝钥匙 ×1");
    if (name == "Green Key")
        return QString::fromUtf8("绿钥匙 ×1");
    if (name == QString::fromUtf8("万能钥匙"))
        return QString::fromUtf8("可开任何门3次（优先使用普通钥匙）");
    if (name == QString::fromUtf8("匿名眼镜"))
        return QString::fromUtf8("可以查看怪物属性");
    if (name == QString::fromUtf8("破墙锤"))
        return QString::fromUtf8("点击使用，下一次移动可摧毁墙壁");
    if (name == QString::fromUtf8("上楼器"))
        return QString::fromUtf8("点击使用，从当前位置上楼");
    if (name == QString::fromUtf8("下楼器"))
        return QString::fromUtf8("点击使用，从当前位置下楼");
    if (name == QString::fromUtf8("临时护盾"))
        return QString::fromUtf8("点击使用，下次战斗防御 +50");
    if (name == QString::fromUtf8("企鹅玩偶"))
        return QString::fromUtf8("面对高松灯和企鹅时伤害减半");
    if (name == QString::fromUtf8("抹茶芭菲"))
        return QString::fromUtf8("面对要乐奈和小猫时伤害减半");
    if (name == QString::fromUtf8("幸运金币"))
        return QString::fromUtf8("打怪和拾取金币翻倍");

    return name;
}

void MainWindow::showInventory()
{
    auto& inv = m_game->player().Inventory();
    int count = m_game->player().InventoryCount();

    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("背包"));
    dlg.resize(400, 400);
    dlg.setStyleSheet("QDialog { background-color: #1a1a2e; color: #d0d0d0; }");

    auto* layout = new QVBoxLayout(&dlg);

    auto* label = new QLabel(QString::fromUtf8("背包 (共 %1 件物品)").arg(count), &dlg);
    label->setStyleSheet("color: #c8a23b; font-size: 16px; font-weight: bold; padding: 8px;");
    layout->addWidget(label);

    auto* list = new QListWidget(&dlg);
    list->setStyleSheet(
        "QListWidget { background: #0d0d1a; color: #d0d0d0; border: 1px solid #555; "
        "font-size: 14px; }"
        "QListWidget::item { padding: 6px; border-bottom: 1px solid #333; }"
        "QListWidget::item:selected { background: #3a3a6a; }"
        "QListWidget::item:hover { background: #2a2a4a; }"
    );

    if (count == 0) {
        auto* emptyItem = new QListWidgetItem(QString::fromUtf8("背包是空的"), list);
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsSelectable);
        emptyItem->setForeground(QColor("#666688"));
    } else {
        for (int i = 0; i < count; ++i) {
            auto* item = m_game->player().GetItem(i);
            if (item) {
                QString text = QString::fromStdString(item->GetName()) + " — " + getItemDescription(item);
                auto* listItem = new QListWidgetItem(text, list);
                listItem->setData(Qt::UserRole, i);
                listItem->setToolTip(QString::fromUtf8("双击使用"));
            }
        }
    }
    layout->addWidget(list);

    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    btnBox->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8("使用"));
    btnBox->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("关闭"));
    btnBox->setStyleSheet(
        "QPushButton { background: #3a3a5a; color: #d0d0d0; border: 1px solid #66a; "
        "border-radius: 4px; padding: 6px 16px; font-size: 14px; }"
        "QPushButton:hover { background: #4a4a7a; }"
    );
    layout->addWidget(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, [&]() {
        auto* cur = list->currentItem();
        if (!cur || count == 0) return;
        int idx = cur->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < count) {
            auto* item = m_game->player().GetItem(idx);
            if (item) {
                QString msg = QString::fromUtf8("使用了 %1: %2")
                    .arg(QString::fromStdString(item->GetName()))
                    .arg(getItemDescription(item));
                m_game->player().UseItem(idx);
                updateHUD();
                QMessageBox::information(&dlg, QString::fromUtf8("使用物品"), msg);
                dlg.accept();
            }
        }
    });
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(list, &QListWidget::itemDoubleClicked, btnBox, &QDialogButtonBox::accepted);

    dlg.exec();
}

void MainWindow::showNPCDialog(int x, int y)
{
    NPC* npc = m_game->npcAt(x, y);
    if (!npc) return;

    Player& p = m_game->player();

    // 交易NPC
    if (npc->IsTrader() && !npc->IsTradeDone()) {
        const Item* tradeReward = npc->GetTradeReward();
        QString rewardDesc;
        if (tradeReward)
            rewardDesc = getItemDescription(tradeReward);
        else
            rewardDesc = QString::fromUtf8("(无)");

        QString info = QString::fromUtf8(
            "【%1】\n\n"
            "交易物品: %2\n"
            "所需金币: %3\n"
            "你的金币: %4\n\n"
            "是否交易？")
            .arg(QString::fromStdString(npc->GetName()))
            .arg(rewardDesc)
            .arg(npc->GetTradeGoldCost())
            .arg(p.gold);

        bool canAfford = (npc->GetTradeGoldCost() <= p.gold);

        auto reply = QMessageBox::question(this, QString::fromUtf8("交易"),
            info,
            canAfford ? (QMessageBox::Yes | QMessageBox::No) : QMessageBox::No,
            QMessageBox::Yes);

        if (reply == QMessageBox::Yes && canAfford) {
            p.gold -= npc->GetTradeGoldCost();
            if (tradeReward) {
                // 创建可应用的物品副本
                auto item = Game::createItemByName(tradeReward->GetName(), tradeReward->GetValue());
                if (item) item->Apply(p);
            }
            npc->SetTradeDone(true);

            QMessageBox::information(this,
                QString::fromStdString(npc->GetName()),
                QString::fromUtf8("交易成功！获得了 %1。").arg(rewardDesc));
        } else {
            // 显示NPC对话
            const auto& dialog = npc->Dialog();
            if (!dialog.empty()) {
                QString fullDialog;
                for (size_t i = 0; i < dialog.size(); ++i) {
                    fullDialog += QString::fromStdString(npc->GetName()) + ": " + QString::fromStdString(dialog[i]);
                    if (i + 1 < dialog.size()) fullDialog += "\n";
                }
                QMessageBox::information(this,
                    QString::fromStdString(npc->GetName()),
                    fullDialog);
            }
        }

        ui.mapWidget->update();
        updateHUD();
        return;
    }

    // 普通NPC（非交易或交易已完成）
    bool hadReward = !npc->HasGivenReward();
    std::string reply = npc->Interact(p);

    if (hadReward && npc->HasGivenReward()) {
        QMessageBox::information(this,
            QString::fromStdString(npc->GetName()),
            QString::fromStdString(reply));
    } else {
        const auto& dialog = npc->Dialog();
        QString fullDialog;
        for (size_t i = 0; i < dialog.size(); ++i) {
            fullDialog += QString::fromStdString(npc->GetName()) + ": " + QString::fromStdString(dialog[i]);
            if (i + 1 < dialog.size()) fullDialog += "\n";
        }
        QMessageBox::information(this,
            QString::fromStdString(npc->GetName()),
            fullDialog);
    }
}

void MainWindow::showShopDialog(int x, int y)
{
    const ShopData* shop = m_game->shopAt(x, y);
    if (!shop) return;

    Player& p = m_game->player();
    int surcharge = p.shopUseCount * 60;

    struct ShopItem {
        QString name;
        int basePrice;
        int actualPrice;
        QString effectDesc;
        std::function<void()> apply;
    };

    std::vector<ShopItem> items;
    items.push_back({QString::fromUtf8("生命药"), shop->potionPrice,
        shop->potionPrice > 0 ? shop->potionPrice + surcharge : 0,
        QString::fromUtf8("生命 +%1").arg(shop->potionValue),
        [&, hpVal = shop->potionValue]() { p.hp += hpVal; p.gold -= shop->potionPrice + p.shopUseCount * 60; p.shopUseCount++; }});
    items.push_back({QString::fromUtf8("武器"), shop->weaponPrice,
        shop->weaponPrice > 0 ? shop->weaponPrice + surcharge : 0,
        QString::fromUtf8("攻击 +%1").arg(shop->weaponValue),
        [&, atkVal = shop->weaponValue]() { p.atk += atkVal; p.gold -= shop->weaponPrice + p.shopUseCount * 60; p.shopUseCount++; }});
    items.push_back({QString::fromUtf8("防具"), shop->armorPrice,
        shop->armorPrice > 0 ? shop->armorPrice + surcharge : 0,
        QString::fromUtf8("防御 +%1").arg(shop->armorValue),
        [&, defVal = shop->armorValue]() { p.def += defVal; p.gold -= shop->armorPrice + p.shopUseCount * 60; p.shopUseCount++; }});

    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("商店"));
    dlg.setFixedSize(360, 360);
    dlg.setStyleSheet("QDialog { background-color: #1a1a2e; color: #d0d0d0; }");

    auto* layout = new QVBoxLayout(&dlg);
    layout->setSpacing(10);
    layout->setContentsMargins(16, 12, 16, 12);

    auto* infoLabel = new QLabel(
        QString::fromUtf8("💰 金币: %1  |  已购 %2 次  (+%3 G/次)")
            .arg(p.gold).arg(p.shopUseCount).arg(surcharge), &dlg);
    infoLabel->setStyleSheet("color: #c8a23b; font-size: 14px; font-weight: bold;");
    layout->addWidget(infoLabel);

    auto* sep = new QFrame(&dlg);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #444;");
    layout->addWidget(sep);

    for (auto& item : items) {
        auto* row = new QHBoxLayout();
        row->setSpacing(8);

        QString desc = item.basePrice > 0
            ? QString::fromUtf8("%1 (%2 G) — %3").arg(item.name).arg(item.actualPrice).arg(item.effectDesc)
            : QString::fromUtf8("%1 — 不售卖").arg(item.name);

        auto* label = new QLabel(desc, &dlg);
        label->setStyleSheet(item.basePrice > 0 ? "font-size: 13px;" : "color: #666; font-size: 13px;");
        row->addWidget(label, 1);

        auto* btn = new QPushButton(QString::fromUtf8("购买"), &dlg);
        btn->setFixedWidth(60);
        btn->setStyleSheet(
            "QPushButton { background: #3a5a3a; color: #d0d0d0; border: 1px solid #6a6; "
            "border-radius: 4px; padding: 4px 10px; font-size: 13px; }"
            "QPushButton:hover { background: #4a7a4a; }"
            "QPushButton:disabled { background: #333; color: #666; border-color: #444; }"
        );
        btn->setEnabled(item.basePrice > 0 && p.gold >= item.actualPrice);

        connect(btn, &QPushButton::clicked, &dlg, [&dlg, &item]() {
            item.apply();
            QMessageBox::information(&dlg, QString::fromUtf8("购买成功"),
                QString::fromUtf8("购买了 %1！%2（花费 %3 G）")
                    .arg(item.name).arg(item.effectDesc).arg(item.actualPrice));
            dlg.accept();
        });
        row->addWidget(btn);

        layout->addLayout(row);
    }

    layout->addStretch();

    auto* leaveBtn = new QPushButton(QString::fromUtf8("离开"), &dlg);
    leaveBtn->setFixedHeight(36);
    leaveBtn->setStyleSheet(
        "QPushButton { background: #3a3a5a; color: #d0d0d0; border: 1px solid #66a; "
        "border-radius: 4px; padding: 6px 16px; font-size: 14px; }"
        "QPushButton:hover { background: #4a4a7a; }"
    );
    connect(leaveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(leaveBtn);

    dlg.exec();
    updateHUD();
}

void MainWindow::updateHUD()
{
    int floor = m_game->currentFloor();
    ui.floorLabel->setText(QString::fromUtf8("第 %1 层").arg(floor));
    ui.hpLabel->setText(QString::fromUtf8("❤ 生命: %1").arg(m_game->player().hp));
    ui.atkLabel->setText(QString::fromUtf8("⚔ 攻击: %1").arg(m_game->player().atk));
    ui.defLabel->setText(QString::fromUtf8("🛡 防御: %1").arg(m_game->player().def));
    ui.goldLabel->setText(QString::fromUtf8("💰 金币: %1").arg(m_game->player().gold));
    QString keyText = QString::fromUtf8("🔑 钥匙: 红%1 蓝%2 绿%3")
        .arg(m_game->player().KeyCount(KeyType::Red))
        .arg(m_game->player().KeyCount(KeyType::Blue))
        .arg(m_game->player().KeyCount(KeyType::Green));
    if (m_game->player().magicKeyUses > 0)
        keyText += QString::fromUtf8("  🔮×%1").arg(m_game->player().magicKeyUses);
    ui.keysLabel->setText(keyText);

    // 显示背包物品列表（金币和钥匙下方）
    int invCount = m_game->player().InventoryCount();
    if (invCount > 0) {
        QString items;
        for (int i = 0; i < invCount; ++i) {
            auto* item = m_game->player().GetItem(i);
            if (item) {
                if (!items.isEmpty()) items += " ";
                items += QString::fromStdString(item->GetName());
            }
        }
        ui.invItemsLabel->setText(QString::fromUtf8("🎒 物品: %1").arg(items));
        ui.invItemsLabel->setVisible(true);
    } else {
        ui.invItemsLabel->setVisible(false);
    }

    ui.invButton->setText(QString::fromUtf8("🎒 背包 (%1)").arg(invCount));
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    // 玩家已死亡则不响应
    if (m_game->player().hp <= 0) {
        QWidget::keyPressEvent(event);
        return;
    }

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

    // 检查上楼器/下楼器
    if (m_game->player().stairUpUsed) {
        m_game->player().stairUpUsed = false;
        m_game->goUpFloor(m_game->player().x, m_game->player().y);
        ui.mapWidget->update();
        updateHUD();
        return;
    }
    if (m_game->player().stairDownUsed) {
        m_game->player().stairDownUsed = false;
        m_game->goDownFloor(m_game->player().x, m_game->player().y);
        ui.mapWidget->update();
        updateHUD();
        return;
    }

    int nx = m_game->player().x + dx;
    int ny = m_game->player().y + dy;
    auto result = m_game->tryMovePlayer(nx, ny);

    switch (result) {
    case Game::Move_Block:
        break;
    case Game::Move_DoorLocked: {
        int tile = m_game->tileAt(nx, ny);
        QString keyName;
        if (tile == Tile_DoorRed) keyName = QString::fromUtf8("红钥匙");
        else if (tile == Tile_DoorBlue) keyName = QString::fromUtf8("蓝钥匙");
        else if (tile == Tile_DoorGreen) keyName = QString::fromUtf8("绿钥匙");
        QMessageBox::information(this, QString::fromUtf8("门已锁"),
            QString::fromUtf8("需要 %1 才能打开这扇门。").arg(keyName));
        break;
    }
    case Game::Move_Ok:
        ui.mapWidget->update();
        updateHUD();
        break;
    case Game::Move_Pickup:
        ui.mapWidget->update();
        updateHUD();
        break;
    case Game::Move_Encounter: {
        Monster* m = m_game->monsterAt(nx, ny);
        if (!m) break;

        // 预判战斗结果
        int dmgToMonster = std::max(0, m_game->player().atk - m->GetDEF());
        int shieldBonus = (m_game->player().tempShieldCharges > 0) ? 50 : 0;
        int dmgToPlayer = std::max(0, m->GetATK() - m_game->player().def - shieldBonus);
        // 特殊道具减伤
        std::string mn = m->GetName();
        if (m_game->player().hasPenguinDoll && (mn == "高松灯" || mn == "企鹅"))
            dmgToPlayer /= 2;
        if (m_game->player().hasMatchaParfait && (mn == "要乐奈" || mn == "小猫"))
            dmgToPlayer /= 2;
        int roundsToKill = (dmgToMonster > 0) ? (m->GetHP() + dmgToMonster - 1) / dmgToMonster : -1;
        int totalDamage = (roundsToKill > 0 && dmgToPlayer > 0) ? (roundsToKill - 1) * dmgToPlayer : 0;
        bool canWin = (dmgToMonster > 0) && (totalDamage < m_game->player().hp);

        // 需要确认的情况：无法取胜 或 有眼镜查看信息
        bool needConfirm = !canWin || m_game->player().hasGlasses;

        if (needConfirm) {
            QString info;
            if (m_game->player().hasGlasses) {
                info = QString::fromUtf8(
                    "【怪物信息】\n名称: %1\n生命: %2  攻击: %3  防御: %4  金币: %5\n\n"
                    "你的攻击: %6  你的防御: %7\n"
                    "预计造成伤害: %8/回合\n预计受到伤害: %9/回合\n"
                    "预计需要: %10 回合\n预计损失: %11 HP")
                    .arg(QString::fromStdString(m->GetName()))
                    .arg(m->GetHP()).arg(m->GetATK()).arg(m->GetDEF()).arg(m->GetGold())
                    .arg(m_game->player().atk).arg(m_game->player().def)
                    .arg(dmgToMonster).arg(dmgToPlayer)
                    .arg(roundsToKill > 0 ? QString::number(roundsToKill) : QString::fromUtf8("∞"))
                    .arg(totalDamage);
            }
            if (!canWin) {
                if (!info.isEmpty()) info += "\n\n";
                if (dmgToMonster <= 0)
                    info += QString::fromUtf8("⚠ 攻击力不足以穿透怪物防御！");
                else
                    info += QString::fromUtf8("⚠ 你很可能被击败（预计损失 %1 HP，当前只有 %2 HP）！")
                        .arg(totalDamage).arg(m_game->player().hp);
            }

            info += QString::fromUtf8("\n\n是否战斗？");
            auto reply = QMessageBox::question(this, QString::fromUtf8("遭遇怪物"), info,
                QMessageBox::Yes | QMessageBox::No, canWin ? QMessageBox::Yes : QMessageBox::No);
            if (reply != QMessageBox::Yes)
                break;
        }

        ui.mapWidget->update();
        std::vector<std::string> log;
        auto fightRes = m_game->fightAt(nx, ny, log);

        QString dlg;
        for (const auto& s : log)
            dlg += QString::fromStdString(s) + "\n";

        if (fightRes == Game::Fight_GameWin) {
            ui.mapWidget->update();
            updateHUD();
            QMessageBox::information(this, QString::fromUtf8("战斗"), dlg);
            gameWin();
            return;
        } else if (fightRes == Game::Fight_PlayerWin) {
            ui.mapWidget->update();
            updateHUD();
            QMessageBox::information(this, QString::fromUtf8("战斗"), dlg);
        } else {
            QMessageBox::critical(this, QString::fromUtf8("战斗"), dlg);
            updateHUD();
            gameOver();
            return;
        }
        break;
    }
    case Game::Move_NPC: {
        showNPCDialog(nx, ny);
        updateHUD();
        break;
    }
    case Game::Move_Shop:
        showShopDialog(nx, ny);
        break;
    case Game::Move_StairsUp:
        m_game->goUpFloor(nx, ny);
        ui.mapWidget->update();
        updateHUD();
        break;
    case Game::Move_StairsDown:
        m_game->goDownFloor(nx, ny);
        ui.mapWidget->update();
        updateHUD();
        break;
    case Game::Move_PlayerDead:
        updateHUD();
        gameOver();
        break;
    }
}

void MainWindow::gameOver()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(QString::fromUtf8("游戏结束"));
    msgBox.setText(QString::fromUtf8("你被击败了！\n\n游戏结束。"));
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStyleSheet(
        "QMessageBox { background-color: #1a1a2e; color: #d0d0d0; }"
        "QLabel { color: #d0d0d0; font-size: 14px; }"
        "QPushButton { background: #3a3a5a; color: #d0d0d0; border: 1px solid #66a;"
        " border-radius: 4px; padding: 6px 16px; min-width: 80px; }"
        "QPushButton:hover { background: #4a4a7a; }"
    );

    QPushButton* restartBtn = msgBox.addButton(QString::fromUtf8("重新开始"), QMessageBox::ActionRole);
    QPushButton* menuBtn    = msgBox.addButton(QString::fromUtf8("返回主菜单"), QMessageBox::RejectRole);
    msgBox.setDefaultButton(restartBtn);

    msgBox.exec();

    if (msgBox.clickedButton() == restartBtn) {
        // 重新开始：创建新的 Game 并重新加载
        delete m_game;
        auto* newGame = new Game();
        newGame->loadDefaultMap();
        m_game = newGame;
        ui.mapWidget->setGame(m_game);
        ui.mapWidget->update();
        updateHUD();
        setFocus();
    } else {
        // 返回主菜单：关闭当前窗口，MenuWindow 会自动显示
        close();
    }
}

void MainWindow::gameWin()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(QString::fromUtf8("游戏通关"));
    msgBox.setText(QString::fromUtf8("恭喜！你击败了长崎素世！\n\n游戏通关！"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStyleSheet(
        "QMessageBox { background-color: #1a1a2e; color: #d0d0d0; }"
        "QLabel { color: #d0d0d0; font-size: 14px; }"
        "QPushButton { background: #3a3a5a; color: #d0d0d0; border: 1px solid #66a;"
        " border-radius: 4px; padding: 6px 16px; min-width: 80px; }"
        "QPushButton:hover { background: #4a4a7a; }"
    );

    QPushButton* restartBtn = msgBox.addButton(QString::fromUtf8("重新开始"), QMessageBox::ActionRole);
    QPushButton* menuBtn    = msgBox.addButton(QString::fromUtf8("返回主菜单"), QMessageBox::RejectRole);
    msgBox.setDefaultButton(menuBtn);

    msgBox.exec();

    if (msgBox.clickedButton() == restartBtn) {
        delete m_game;
        auto* newGame = new Game();
        newGame->loadDefaultMap();
        m_game = newGame;
        ui.mapWidget->setGame(m_game);
        ui.mapWidget->update();
        updateHUD();
        setFocus();
    } else {
        close();
    }
}
