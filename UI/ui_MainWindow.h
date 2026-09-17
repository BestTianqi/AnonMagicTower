#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QFrame>
#include <QFont>
#include <QScrollArea>
#include <QGridLayout>
#include "MapWidget.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow {
public:
    MapWidget*   mapWidget     = nullptr;
    QWidget*     sidePanel     = nullptr;
    QScrollArea* monsterScroll = nullptr;
    QWidget*     monsterPanel  = nullptr;
    QVBoxLayout* monsterLayout = nullptr;
    QLabel*      floorLabel    = nullptr;
    QLabel*      gameTitle     = nullptr;
    QLabel*      gameSubtitle  = nullptr;
    QLabel*      hpLabel      = nullptr;
    QLabel*      atkLabel     = nullptr;
    QLabel*      defLabel     = nullptr;
    QLabel*      goldLabel    = nullptr;
    QLabel*      keysLabel    = nullptr;
    QLabel*      invItemsLabel = nullptr;
    QWidget*     itemPanel     = nullptr;
    QScrollArea* itemScroll    = nullptr;
    QGridLayout* itemLayout    = nullptr;
    QLabel*      battleLabel   = nullptr;
    QPushButton* saveButton   = nullptr;
    QPushButton* quickSaveButton = nullptr;
    QPushButton* undoButton   = nullptr;
    QPushButton* loadButton   = nullptr;
    QPushButton* settingsButton = nullptr;
    QPushButton* modButton    = nullptr;

    void setupUi(QWidget* parent) {
        if (parent->objectName().isEmpty())
            parent->setObjectName("MainWindow");
        parent->resize(1600, 900);
        parent->setMinimumSize(1280, 960);

        // === 侧边栏 ===
        sidePanel = new QWidget(parent);
        sidePanel->setObjectName("sidePanel");
        sidePanel->setFixedWidth(300);

        auto* vbox = new QVBoxLayout(sidePanel);
        vbox->setObjectName("verticalLayout");
        vbox->setContentsMargins(14, 14, 14, 14);
        vbox->setSpacing(6);

        QFont titleFont;
        titleFont.setPointSize(18);
        titleFont.setBold(true);

        QFont statFont;
        statFont.setPointSize(14);

        gameTitle = new QLabel(QString::fromUtf8("邦多利魔塔"), sidePanel);
        gameTitle->setObjectName("gameTitle");
        gameTitle->setAlignment(Qt::AlignCenter);
        gameTitle->setStyleSheet("color: #ff76b6; font-size: 21px; font-weight: 900; padding: 0 0 2px;");
        vbox->addWidget(gameTitle);

        gameSubtitle = new QLabel(QString::fromUtf8("GIRLS BAND PARTY  ·  LIVE TOWER"), sidePanel);
        gameSubtitle->setObjectName("gameSubtitle");
        gameSubtitle->setAlignment(Qt::AlignCenter);
        gameSubtitle->setStyleSheet("color: #9fd8ff; font-size: 10px; font-weight: 700; padding: 0 0 5px;");
        vbox->addWidget(gameSubtitle);

        floorLabel = new QLabel(sidePanel);
        floorLabel->setObjectName("floorLabel");
        floorLabel->setText(QString::fromUtf8("第 1 层"));
        floorLabel->setFont(titleFont);
        floorLabel->setAlignment(Qt::AlignCenter);
        floorLabel->setStyleSheet("color: #ffd66b; padding: 5px; background: rgba(52,28,70,150); border: 1px solid #7f558e; border-radius: 5px;");
        vbox->addWidget(floorLabel);

        auto* sep1 = new QFrame(sidePanel);
        sep1->setFrameShape(QFrame::HLine);
        sep1->setStyleSheet("color: #555;");
        vbox->addWidget(sep1);

        auto* statusTitle = new QLabel(QString::fromUtf8("角色状态"), sidePanel);
        statusTitle->setStyleSheet("color: #f1cf7a; font-size: 13px; font-weight: 700; padding: 2px 0;");
        vbox->addWidget(statusTitle);

        hpLabel = new QLabel(sidePanel);
        hpLabel->setObjectName("hpLabel");
        hpLabel->setText(QString::fromUtf8("❤ 生命: 100"));
        hpLabel->setFont(statFont);
        hpLabel->setMinimumHeight(30);
        hpLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        hpLabel->setStyleSheet("color: #e05555;");
        vbox->addWidget(hpLabel);

        atkLabel = new QLabel(sidePanel);
        atkLabel->setObjectName("atkLabel");
        atkLabel->setText(QString::fromUtf8("⚔ 攻击: 10"));
        atkLabel->setFont(statFont);
        atkLabel->setMinimumHeight(30);
        atkLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        atkLabel->setStyleSheet("color: #d4952a;");
        vbox->addWidget(atkLabel);

        defLabel = new QLabel(sidePanel);
        defLabel->setObjectName("defLabel");
        defLabel->setText(QString::fromUtf8("🛡 防御: 5"));
        defLabel->setFont(statFont);
        defLabel->setMinimumHeight(30);
        defLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        defLabel->setStyleSheet("color: #3b8bc2;");
        vbox->addWidget(defLabel);

        auto* sep2 = new QFrame(sidePanel);
        sep2->setFrameShape(QFrame::HLine);
        sep2->setStyleSheet("color: #555;");
        vbox->addWidget(sep2);

        goldLabel = new QLabel(sidePanel);
        goldLabel->setObjectName("goldLabel");
        goldLabel->setText(QString::fromUtf8("💰 金币: 0"));
        goldLabel->setFont(statFont);
        goldLabel->setMinimumHeight(30);
        goldLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        goldLabel->setStyleSheet("color: #c8a23b;");
        vbox->addWidget(goldLabel);

        keysLabel = new QLabel(sidePanel);
        keysLabel->setObjectName("keysLabel");
        keysLabel->setText(QString::fromUtf8("🔑 钥匙: 红0 蓝0 绿0"));
        keysLabel->setFont(statFont);
        keysLabel->setMinimumHeight(30);
        keysLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        keysLabel->setStyleSheet("color: #aaccaa;");
        vbox->addWidget(keysLabel);

        auto* itemTitle = new QLabel(QString::fromUtf8("持有道具"), sidePanel);
        itemTitle->setStyleSheet("color: #f1cf7a; font-size: 13px; font-weight: 700; padding: 2px 0;");
        vbox->addWidget(itemTitle);

        itemScroll = new QScrollArea(sidePanel);
        itemScroll->setObjectName("itemScroll");
        itemScroll->setFixedHeight(330);
        itemScroll->setWidgetResizable(true);
        itemScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        itemScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        itemScroll->setFrameShape(QFrame::NoFrame);
        itemScroll->setStyleSheet("QScrollArea#itemScroll { background: rgba(10,11,20,110); border: 1px solid #45465d; border-radius: 4px; }");

        itemPanel = new QWidget();
        itemPanel->setObjectName("itemPanel");
        itemPanel->setStyleSheet("QWidget#itemPanel { background: transparent; }");
        itemLayout = new QGridLayout(itemPanel);
        itemLayout->setContentsMargins(4, 4, 4, 4);
        itemLayout->setHorizontalSpacing(2);
        itemLayout->setVerticalSpacing(2);
        itemScroll->setWidget(itemPanel);
        vbox->addWidget(itemScroll);

        invItemsLabel = new QLabel(sidePanel);
        invItemsLabel->setObjectName("invItemsLabel");
        invItemsLabel->setText("");
        invItemsLabel->setWordWrap(true);
        invItemsLabel->setStyleSheet("color: #c2c8df; font-size: 12px; padding: 6px 4px; background: rgba(10,11,20,120); border: 1px solid #45465d; border-radius: 4px;");
        invItemsLabel->setVisible(false);
        vbox->addWidget(invItemsLabel);

        battleLabel = new QLabel(sidePanel);
        battleLabel->setObjectName("battleLabel");
        battleLabel->setWordWrap(true);
        battleLabel->setMinimumHeight(48);
        battleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        battleLabel->setStyleSheet("color: #ffd98a; font-size: 12px; padding: 7px; background: rgba(42,30,55,190); border: 1px solid #8c6ba8; border-radius: 4px;");
        battleLabel->setVisible(false);
        vbox->addWidget(battleLabel);

        auto* controlsHint = new QLabel(QString::fromUtf8("方向键移动  ·  走到怪物前自动战斗"), sidePanel);
        controlsHint->setObjectName("controlsHint");
        controlsHint->setAlignment(Qt::AlignCenter);
        controlsHint->setStyleSheet("color: #8490b8; font-size: 11px; padding: 3px 0;");
        vbox->addWidget(controlsHint);

        auto* spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
        vbox->addSpacerItem(spacer);

        auto* sep3 = new QFrame(sidePanel);
        sep3->setFrameShape(QFrame::HLine);
        sep3->setStyleSheet("color: #555;");
        vbox->addWidget(sep3);

        saveButton = new QPushButton(sidePanel);
        saveButton->setObjectName("saveButton");
        saveButton->setText(QString::fromUtf8("💾 保存"));
        saveButton->setMinimumWidth(84);
        saveButton->setFixedHeight(34);

        quickSaveButton = new QPushButton(sidePanel);
        quickSaveButton->setObjectName("quickSaveButton");
        quickSaveButton->setText(QString::fromUtf8("⚡ 即时存档"));
        quickSaveButton->setMinimumWidth(84);
        quickSaveButton->setFixedHeight(34);

        undoButton = new QPushButton(sidePanel);
        undoButton->setObjectName("undoButton");
        undoButton->setText(QString::fromUtf8("↶ 撤销"));
        undoButton->setMinimumWidth(84);
        undoButton->setFixedHeight(34);

        loadButton = new QPushButton(sidePanel);
        loadButton->setObjectName("loadButton");
        loadButton->setText(QString::fromUtf8("📂 读取"));
        loadButton->setMinimumWidth(84);
        loadButton->setFixedHeight(34);

        settingsButton = new QPushButton(sidePanel);
        settingsButton->setObjectName("settingsButton");
        settingsButton->setText(QString::fromUtf8("⚙ 设置"));
        settingsButton->setMinimumWidth(84);
        settingsButton->setFixedHeight(34);

        modButton = new QPushButton(sidePanel);
        modButton->setObjectName("modButton");
        modButton->setText(QString::fromUtf8("⚙ 修改器"));
        modButton->setMinimumWidth(84);
        modButton->setFixedHeight(34);

        auto* controlsGrid = new QGridLayout();
        controlsGrid->setContentsMargins(0, 0, 0, 0);
        controlsGrid->setHorizontalSpacing(4);
        controlsGrid->setVerticalSpacing(4);
        controlsGrid->addWidget(saveButton, 0, 0);
        controlsGrid->addWidget(quickSaveButton, 0, 1);
        controlsGrid->addWidget(undoButton, 0, 2);
        controlsGrid->addWidget(loadButton, 1, 0);
        controlsGrid->addWidget(settingsButton, 1, 1);
        controlsGrid->addWidget(modButton, 1, 2);
        for (int column = 0; column < 3; ++column)
            controlsGrid->setColumnStretch(column, 1);
        for (QPushButton* button : {saveButton, quickSaveButton, undoButton,
                                    loadButton, settingsButton, modButton}) {
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            button->setFixedHeight(34);
        }
        vbox->addLayout(controlsGrid);

        // === 左侧怪物面板 ===
        monsterScroll = new QScrollArea(parent);
        monsterScroll->setObjectName("monsterScroll");
        monsterScroll->setFixedWidth(240);
        monsterScroll->setWidgetResizable(true);
        monsterScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        monsterScroll->setStyleSheet(
            "QScrollArea { background-color: #141428; border: 1px solid #333; border-radius: 6px; }"
            "QScrollBar:vertical { background: #1a1a2e; width: 8px; }"
            "QScrollBar::handle:vertical { background: #444; border-radius: 4px; min-height: 20px; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        );

        monsterPanel = new QWidget();
        monsterPanel->setObjectName("monsterPanel");
        monsterPanel->setStyleSheet("background-color: #141428;");
        monsterLayout = new QVBoxLayout(monsterPanel);
        monsterLayout->setContentsMargins(8, 8, 8, 8);
        monsterLayout->setSpacing(6);

        auto* monsterTitle = new QLabel(QString::fromUtf8("本层怪物"));
        monsterTitle->setFont(titleFont);
        monsterTitle->setAlignment(Qt::AlignCenter);
        monsterTitle->setStyleSheet("color: #c8a23b; padding: 4px;");
        monsterLayout->addWidget(monsterTitle);

        auto* monSep = new QFrame(monsterPanel);
        monSep->setFrameShape(QFrame::HLine);
        monSep->setStyleSheet("color: #444;");
        monsterLayout->addWidget(monSep);

        monsterLayout->addStretch();
        monsterScroll->setWidget(monsterPanel);

        // === 地图控件 900x900 ===
        mapWidget = new MapWidget(nullptr, parent);
        mapWidget->setObjectName("mapWidget");

        // === 主布局：居中 地图 + 侧边栏 ===
        auto* hbox = new QHBoxLayout(parent);
        hbox->setObjectName("horizontalLayout");
        hbox->setContentsMargins(0, 0, 0, 0);
        hbox->setSpacing(0);

        hbox->addStretch();               // 左侧弹性空间
        hbox->addWidget(monsterScroll);  // 怪物面板 240
        hbox->addWidget(mapWidget);      // 地图 900x900
        hbox->addWidget(sidePanel);      // 侧边栏 280
        hbox->addStretch();              // 右侧弹性空间

        parent->setStyleSheet("QWidget#MainWindow { background-color: #1a1a2e; }");

        QMetaObject::connectSlotsByName(parent);
    }
};

namespace Ui {
    class MainWindow : public Ui_MainWindow {};
}

QT_END_NAMESPACE
