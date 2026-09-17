#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QFont>
#include <QPixmap>

QT_BEGIN_NAMESPACE

class Ui_MenuWindow {
public:
    QLabel*      titleLabel   = nullptr;
    QLabel*      anonPortrait = nullptr;
    QLabel*      soyoPortrait = nullptr;
    QPushButton* newGameBtn   = nullptr;
    QPushButton* loadGameBtn  = nullptr;
    QPushButton* mapEditorBtn = nullptr;
    QPushButton* settingsBtn  = nullptr;

    void setupUi(QWidget* parent) {
        if (parent->objectName().isEmpty())
            parent->setObjectName("MenuWindow");
        parent->resize(1600, 900);
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

        // 两侧立绘独立于中央菜单布局，运行时按窗口大小缩放定位。
        anonPortrait = new QLabel(parent);
        anonPortrait->setObjectName("anonPortrait");
        anonPortrait->setAlignment(Qt::AlignCenter | Qt::AlignBottom);
        anonPortrait->setAttribute(Qt::WA_TransparentForMouseEvents);
        anonPortrait->setStyleSheet("background: transparent;");
        anonPortrait->setPixmap(QPixmap(QStringLiteral(":/images/characters/portraits/anon.png")));

        soyoPortrait = new QLabel(parent);
        soyoPortrait->setObjectName("soyoPortrait");
        soyoPortrait->setAlignment(Qt::AlignCenter | Qt::AlignBottom);
        soyoPortrait->setAttribute(Qt::WA_TransparentForMouseEvents);
        soyoPortrait->setStyleSheet("background: transparent;");
        soyoPortrait->setPixmap(QPixmap(QStringLiteral(":/images/characters/portraits/soyo_stage.png")));

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

        // 地图编辑器
        mapEditorBtn = new QPushButton(parent);
        mapEditorBtn->setText(QString::fromUtf8("🛠  地 图 编 辑 器"));
        mapEditorBtn->setMinimumHeight(60);
        mapEditorBtn->setMaximumWidth(400);
        mapEditorBtn->setCursor(Qt::PointingHandCursor);
        QString editorBtnStyle =
            "QPushButton {"
            "  background: rgba(60, 50, 30, 200);"
            "  color: #d0d0d0;"
            "  border: 2px solid #8a7a5a;"
            "  border-radius: 8px;"
            "  padding: 16px 0px;"
            "  font-size: 22px;"
            "  min-width: 320px;"
            "}"
            "QPushButton:hover {"
            "  background: rgba(100, 70, 40, 220);"
            "  border-color: #ccaa88;"
            "  color: #ffffff;"
            "}"
            "QPushButton:pressed {"
            "  background: rgba(120, 90, 50, 220);"
            "}";
        mapEditorBtn->setStyleSheet(editorBtnStyle);
        auto* cEditor = new QHBoxLayout();
        cEditor->addStretch(); cEditor->addWidget(mapEditorBtn); cEditor->addStretch();
        vbox->addLayout(cEditor);

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
