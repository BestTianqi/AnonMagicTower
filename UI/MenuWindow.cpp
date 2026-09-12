#include "MenuWindow.h"
#include "MainWindow.h"
#include "MapEditor.h"
#include "Game/Game.h"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>

MenuWindow::MenuWindow(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    setWindowTitle(QString::fromUtf8("魔塔"));
    setMinimumSize(520, 420);
    setStyleSheet(
        "QWidget#MenuWindow { background-color: #111322; background-image: url(:/images/backgrounds/tower_hub.png); background-position: center; background-repeat: no-repeat; color: #e8e9f2; }"
        "QPushButton { background: #2d3150; color: #f2f4ff; border: 1px solid #525a88; border-radius: 8px; padding: 10px 18px; font-size: 15px; }"
        "QPushButton:hover { background: #414875; border-color: #7f8bce; }"
        "QPushButton:pressed { background: #232640; }"
        "QLabel { color: #dfe3f5; }"
    );

    connect(ui.newGameBtn,   &QPushButton::clicked, this, &MenuWindow::onNewGame);
    connect(ui.loadGameBtn,  &QPushButton::clicked, this, &MenuWindow::onLoadGame);
    connect(ui.mapEditorBtn, &QPushButton::clicked, this, &MenuWindow::onMapEditor);
    connect(ui.settingsBtn,  &QPushButton::clicked, this, &MenuWindow::onSettings);
}

void MenuWindow::onNewGame()
{
    auto* game = new Game();
    game->loadDefaultMap();
    enterGame(game);
}

void MenuWindow::onLoadGame()
{
    QString file = QFileDialog::getOpenFileName(this,
        QString::fromUtf8("读取存档"),
        QString(),
        QString::fromUtf8("保存文件 (*.txt)"));
    if (file.isEmpty()) return;

    auto* game = new Game();
    if (!game->loadFromFile(file.toStdString())) {
        QMessageBox::warning(this, QString::fromUtf8("读取失败"),
            QString::fromUtf8("无法读取存档文件。"));
        delete game;
        return;
    }
    enterGame(game);
}

void MenuWindow::onMapEditor()
{
    auto* editor = new MapEditor();
    editor->setAttribute(Qt::WA_DeleteOnClose);
    editor->show();
}

void MenuWindow::onSettings()
{
    QMessageBox::information(this, QString::fromUtf8("设置"),
        QString::fromUtf8("设置功能开发中…\n\n"
            "操作说明：\n"
            "方向键：移动\n"
            "Shift+点击：地图编辑器中放置玩家\n"
            "右键：地图编辑器中擦除"));
}

void MenuWindow::enterGame(Game* game)
{
    m_gameWindow = new MainWindow(game);
    m_gameWindow->loadAssets();
    m_gameWindow->show();
    m_gameWindow->raise();
    m_gameWindow->activateWindow();
    m_gameWindow->setFocus();

    // 游戏窗口关闭时回到菜单
    connect(m_gameWindow, &QWidget::destroyed, this, [this]() {
        m_gameWindow = nullptr;
    });
    // 或者直接检测关闭事件来重新显示菜单
    m_gameWindow->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_gameWindow, &QObject::destroyed, this, &QWidget::show);

    hide();
}
