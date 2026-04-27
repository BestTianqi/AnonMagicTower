#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QFrame>
#include <QFont>
#include "MapWidget.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow {
public:
    MapWidget*   mapWidget   = nullptr;
    QWidget*     sidePanel   = nullptr;
    QLabel*      floorLabel  = nullptr;
    QLabel*      hpLabel     = nullptr;
    QLabel*      atkLabel    = nullptr;
    QLabel*      defLabel    = nullptr;
    QLabel*      goldLabel   = nullptr;
    QLabel*      keysLabel   = nullptr;
    QPushButton* saveButton  = nullptr;
    QPushButton* loadButton  = nullptr;

    void setupUi(QWidget* parent) {
        if (parent->objectName().isEmpty())
            parent->setObjectName("MainWindow");
        parent->resize(1920, 1080);
        parent->setMinimumSize(960, 540);

        mapWidget = new MapWidget(nullptr, parent);
        mapWidget->setObjectName("mapWidget");
        mapWidget->setMinimumSize(640, 480);

        sidePanel = new QWidget(parent);
        sidePanel->setObjectName("sidePanel");
        sidePanel->setFixedWidth(280);

        auto* vbox = new QVBoxLayout(sidePanel);
        vbox->setObjectName("verticalLayout");
        vbox->setContentsMargins(16, 16, 16, 16);
        vbox->setSpacing(10);

        QFont titleFont;
        titleFont.setPointSize(18);
        titleFont.setBold(true);

        QFont statFont;
        statFont.setPointSize(14);

        // floor indicator
        floorLabel = new QLabel(sidePanel);
        floorLabel->setObjectName("floorLabel");
        floorLabel->setText(QString::fromUtf8("第 1 层"));
        floorLabel->setFont(titleFont);
        floorLabel->setAlignment(Qt::AlignCenter);
        floorLabel->setStyleSheet("color: #c8a23b; padding: 6px;");
        vbox->addWidget(floorLabel);

        // separator
        auto* sep1 = new QFrame(sidePanel);
        sep1->setFrameShape(QFrame::HLine);
        sep1->setStyleSheet("color: #555;");
        vbox->addWidget(sep1);

        // HP
        hpLabel = new QLabel(sidePanel);
        hpLabel->setObjectName("hpLabel");
        hpLabel->setText(QString::fromUtf8("❤ 生命: 100"));
        hpLabel->setFont(statFont);
        hpLabel->setStyleSheet("color: #e05555;");
        vbox->addWidget(hpLabel);

        // ATK
        atkLabel = new QLabel(sidePanel);
        atkLabel->setObjectName("atkLabel");
        atkLabel->setText(QString::fromUtf8("⚔ 攻击: 10"));
        atkLabel->setFont(statFont);
        atkLabel->setStyleSheet("color: #d4952a;");
        vbox->addWidget(atkLabel);

        // DEF
        defLabel = new QLabel(sidePanel);
        defLabel->setObjectName("defLabel");
        defLabel->setText(QString::fromUtf8("🛡 防御: 5"));
        defLabel->setFont(statFont);
        defLabel->setStyleSheet("color: #3b8bc2;");
        vbox->addWidget(defLabel);

        // separator
        auto* sep2 = new QFrame(sidePanel);
        sep2->setFrameShape(QFrame::HLine);
        sep2->setStyleSheet("color: #555;");
        vbox->addWidget(sep2);

        // Gold
        goldLabel = new QLabel(sidePanel);
        goldLabel->setObjectName("goldLabel");
        goldLabel->setText(QString::fromUtf8("💰 金币: 0"));
        goldLabel->setFont(statFont);
        goldLabel->setStyleSheet("color: #c8a23b;");
        vbox->addWidget(goldLabel);

        // Keys
        keysLabel = new QLabel(sidePanel);
        keysLabel->setObjectName("keysLabel");
        keysLabel->setText(QString::fromUtf8("🔑 钥匙: 红0 蓝0 绿0"));
        keysLabel->setFont(statFont);
        keysLabel->setStyleSheet("color: #aaccaa;");
        vbox->addWidget(keysLabel);

        // spacer
        auto* spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
        vbox->addSpacerItem(spacer);

        // separator before buttons
        auto* sep3 = new QFrame(sidePanel);
        sep3->setFrameShape(QFrame::HLine);
        sep3->setStyleSheet("color: #555;");
        vbox->addWidget(sep3);

        // Save button
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

        // Load button
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

        // main layout
        auto* hbox = new QHBoxLayout(parent);
        hbox->setObjectName("horizontalLayout");
        hbox->setContentsMargins(0, 0, 0, 0);
        hbox->setSpacing(0);
        hbox->addWidget(mapWidget);
        hbox->addWidget(sidePanel);

        // dark background
        parent->setStyleSheet("QWidget#MainWindow { background-color: #1a1a2e; }");

        QMetaObject::connectSlotsByName(parent);
    }
};

namespace Ui {
    class MainWindow : public Ui_MainWindow {};
}

QT_END_NAMESPACE
