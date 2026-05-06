#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QFont>

QT_BEGIN_NAMESPACE

class Ui_MenuWindow {
public:
    QLabel*      titleLabel  = nullptr;
    QPushButton* newGameBtn  = nullptr;
    QPushButton* loadGameBtn = nullptr;
    QPushButton* settingsBtn = nullptr;

    void setupUi(QWidget* parent) {
        if (parent->objectName().isEmpty())
            parent->setObjectName("MenuWindow");
        parent->resize(1920, 1080);
        parent->setMinimumSize(960, 540);
        parent->setStyleSheet("QWidget#MenuWindow { background-color: #0d0d1a; }");

        auto* vbox = new QVBoxLayout(parent);
        vbox->setAlignment(Qt::AlignCenter);

        vbox->addStretch(3);

        // 标题
        titleLabel = new QLabel(parent);
        titleLabel->setText(QString::fromUtf8("魔 塔"));
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont titleFont;
        titleFont.setPointSize(72);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
        titleLabel->setStyleSheet("color: #c8a23b;");
        vbox->addWidget(titleLabel);

        // 副标题
        auto* subtitle = new QLabel(parent);
        subtitle->setText(QString::fromUtf8("— Magic Tower —"));
        subtitle->setAlignment(Qt::AlignCenter);
        QFont subFont;
        subFont.setPointSize(16);
        subtitle->setFont(subFont);
        subtitle->setStyleSheet("color: #666688;");
        vbox->addWidget(subtitle);

        vbox->addSpacing(60);

        QString btnStyle =
            "QPushButton {"
            "  background: rgba(30, 30, 60, 200);"
            "  color: #d0d0d0;"
            "  border: 2px solid #5a5a8a;"
            "  border-radius: 8px;"
            "  padding: 16px 0px;"
            "  font-size: 22px;"
            "  min-width: 320px;"
            "}"
            "QPushButton:hover {"
            "  background: rgba(50, 50, 100, 220);"
            "  border-color: #8888cc;"
            "  color: #ffffff;"
            "}"
            "QPushButton:pressed {"
            "  background: rgba(70, 70, 120, 220);"
            "}";

        // 新游戏
        newGameBtn = new QPushButton(parent);
        newGameBtn->setText(QString::fromUtf8("⚔  新 游 戏"));
        newGameBtn->setMinimumHeight(60);
        newGameBtn->setMaximumWidth(400);
        newGameBtn->setCursor(Qt::PointingHandCursor);
        newGameBtn->setStyleSheet(btnStyle);
        auto* c1 = new QHBoxLayout();
        c1->addStretch(); c1->addWidget(newGameBtn); c1->addStretch();
        vbox->addLayout(c1);

        vbox->addSpacing(16);

        // 读取存档
        loadGameBtn = new QPushButton(parent);
        loadGameBtn->setText(QString::fromUtf8("📂  读 取 存 档"));
        loadGameBtn->setMinimumHeight(60);
        loadGameBtn->setMaximumWidth(400);
        loadGameBtn->setCursor(Qt::PointingHandCursor);
        loadGameBtn->setStyleSheet(btnStyle);
        auto* c2 = new QHBoxLayout();
        c2->addStretch(); c2->addWidget(loadGameBtn); c2->addStretch();
        vbox->addLayout(c2);

        vbox->addSpacing(16);

        // 设置
        settingsBtn = new QPushButton(parent);
        settingsBtn->setText(QString::fromUtf8("⚙  设 置"));
        settingsBtn->setMinimumHeight(60);
        settingsBtn->setMaximumWidth(400);
        settingsBtn->setCursor(Qt::PointingHandCursor);
        settingsBtn->setStyleSheet(btnStyle);
        auto* c3 = new QHBoxLayout();
        c3->addStretch(); c3->addWidget(settingsBtn); c3->addStretch();
        vbox->addLayout(c3);

        vbox->addStretch(4);
    }
};

namespace Ui {
    class MenuWindow : public Ui_MenuWindow {};
}

QT_END_NAMESPACE
