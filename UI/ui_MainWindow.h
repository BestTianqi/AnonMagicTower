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
    QLabel*      battleLabel   = nullptr;
    QPushButton* invButton    = nullptr;
    QPushButton* saveButton   = nullptr;
    QPushButton* quickSaveButton = nullptr;
    QPushButton* undoButton   = nullptr;
    QPushButton* loadButton   = nullptr;
    QPushButton* settingsButton = nullptr;
    QPushButton* editorButton = nullptr;
    QPushButton* modButton    = nullptr;

    void setupUi(QWidget* parent) {
        if (parent->objectName().isEmpty())
            parent->setObjectName("MainWindow");
        parent->resize(1920, 1080);
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

        invItemsLabel = new QLabel(sidePanel);
        invItemsLabel->setObjectName("invItemsLabel");
        invItemsLabel->setText("");
        invItemsLabel->setWordWrap(true);
        invItemsLabel->setStyleSheet("color: #c2c8df; font-size: 12px; padding: 6px 4px; background: rgba(10,11,20,120); border: 1px solid #45465d; border-radius: 4px;");
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

        invButton = new QPushButton(sidePanel);
        invButton->setObjectName("invButton");
        invButton->setText(QString::fromUtf8("🎒 背包"));
        invButton->setMinimumHeight(40);
        invButton->setStyleSheet(
            "QPushButton { background: #3a3a6a; color: #d0d0d0; border: 1px solid #66a; "
            "border-radius: 4px; padding: 8px; font-size: 14px; }"
            "QPushButton:hover { background: #4a4a8a; }"
        );
        vbox->addWidget(invButton);

        saveButton = new QPushButton(sidePanel);
        saveButton->setObjectName("saveButton");
        saveButton->setText(QString::fromUtf8("💾 保存"));
        saveButton->setMinimumHeight(40);
        saveButton->setStyleSheet(
            "QPushButton { background: #3a5a3a; color: #d0d0d0; border: 1px solid #5a5; "
            "border-radius: 4px; padding: 8px; font-size: 14px; }"
            "QPushButton:hover { background: #4a7a4a; }"
        );
        vbox->addWidget(saveButton);

        quickSaveButton = new QPushButton(sidePanel);
        quickSaveButton->setObjectName("quickSaveButton");
        quickSaveButton->setText(QString::fromUtf8("⚡ 即时存档"));
        quickSaveButton->setMinimumHeight(40);
        vbox->addWidget(quickSaveButton);

        undoButton = new QPushButton(sidePanel);
        undoButton->setObjectName("undoButton");
        undoButton->setText(QString::fromUtf8("↶ 撤销"));
        undoButton->setMinimumHeight(40);
        vbox->addWidget(undoButton);

        loadButton = new QPushButton(sidePanel);
        loadButton->setObjectName("loadButton");
        loadButton->setText(QString::fromUtf8("📂 读取"));
        loadButton->setMinimumHeight(40);
        loadButton->setStyleSheet(
            "QPushButton { background: #4a4a5a; color: #d0d0d0; border: 1px solid #66a; "
            "border-radius: 4px; padding: 8px; font-size: 14px; }"
            "QPushButton:hover { background: #5a5a7a; }"
        );
        vbox->addWidget(loadButton);

        settingsButton = new QPushButton(sidePanel);
        settingsButton->setObjectName("settingsButton");
        settingsButton->setText(QString::fromUtf8("⚙ 设置"));
        settingsButton->setMinimumHeight(40);
        vbox->addWidget(settingsButton);

        editorButton = new QPushButton(sidePanel);
        editorButton->setObjectName("editorButton");
        editorButton->setText(QString::fromUtf8("🛠 地图编辑器"));
        editorButton->setMinimumHeight(40);
        editorButton->setStyleSheet(
            "QPushButton { background: #5a4a3a; color: #d0d0d0; border: 1px solid #a85; "
            "border-radius: 4px; padding: 8px; font-size: 14px; }"
            "QPushButton:hover { background: #7a6a4a; }"
        );
        vbox->addWidget(editorButton);

        modButton = new QPushButton(sidePanel);
        modButton->setObjectName("modButton");
        modButton->setText(QString::fromUtf8("⚙ 修改器"));
        modButton->setMinimumHeight(40);
        modButton->setStyleSheet(
            "QPushButton { background: #6a3a3a; color: #ffd; border: 1px solid #d55; "
            "border-radius: 4px; padding: 8px; font-size: 14px; }"
            "QPushButton:hover { background: #8a4a4a; }"
        );
        vbox->addWidget(modButton);

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
