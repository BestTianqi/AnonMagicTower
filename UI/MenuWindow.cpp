#include "MenuWindow.h"
#include "MainWindow.h"
#include "MapEditor.h"
#include "Game/Game.h"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QIcon>
#include <QPainter>

MenuWindow::MenuWindow(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    m_backgroundImage.load(QStringLiteral(":/images/backgrounds/mujica_theater.png"));
    setWindowTitle(QString::fromUtf8("MYGO!!!!! × Ave Mujica：梦限大魔塔"));
    // 与游戏页的布局尺寸一致，嵌入后窗口不会把地图和侧栏压缩到不可用。
    setMinimumSize(1100, 760);
    setStyleSheet(
        "QWidget#MenuWindow { background-color: #0a0812; color: #e8e9f2; }"
        "QPushButton { color: #fff7d0; border-image: url(:/images/runtime/ui/button_texture.png) 18 24 18 24 stretch stretch; padding: 10px 18px; font-size: 15px; font-weight: 700; }"
        "QPushButton:hover { color: #ffffff; }"
        "QLabel { color: #dfe3f5; }"
    );
    const QString buttonArt =
        "QPushButton { color: #fff7d0; border-image: url(:/images/runtime/ui/button_texture.png) 18 24 18 24 stretch stretch; padding: 10px 18px; font-size: 20px; font-weight: 700; }"
        "QPushButton:hover { color: white; }";
    for (QPushButton* button : {ui.newGameBtn, ui.loadGameBtn, ui.mapEditorBtn, ui.settingsBtn})
        button->setStyleSheet(buttonArt);
    ui.newGameBtn->setIcon(QIcon(":/images/runtime/items/weapon.png"));
    ui.loadGameBtn->setIcon(QIcon(":/images/runtime/items/treasure.png"));
    ui.mapEditorBtn->setIcon(QIcon(":/images/runtime/items/artifact.png"));
    ui.settingsBtn->setIcon(QIcon(":/images/runtime/items/stairs_down.png"));
    for (QPushButton* button : {ui.newGameBtn, ui.loadGameBtn, ui.mapEditorBtn, ui.settingsBtn})
        button->setIconSize(QSize(38, 38));
    ui.newGameBtn->setText(QString::fromUtf8("新 游 戏"));
    ui.loadGameBtn->setText(QString::fromUtf8("读 取 存 档"));
    ui.mapEditorBtn->setText(QString::fromUtf8("地 图 编 辑 器"));
    ui.settingsBtn->setText(QString::fromUtf8("设 置"));
    ui.titleLabel->setStyleSheet(
        "color: #fff2bd; border-image: url(:/images/runtime/ui/title_plaque.png) 18 32 18 32 stretch stretch; padding: 18px 70px;");

    connect(ui.newGameBtn,   &QPushButton::clicked, this, &MenuWindow::onNewGame);
    connect(ui.loadGameBtn,  &QPushButton::clicked, this, &MenuWindow::onLoadGame);
    connect(ui.mapEditorBtn, &QPushButton::clicked, this, &MenuWindow::onMapEditor);
    connect(ui.settingsBtn,  &QPushButton::clicked, this, &MenuWindow::onSettings);
}

void MenuWindow::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#0a0812"));
    if (m_backgroundImage.isNull()) return;

    const QPixmap scaled = m_backgroundImage.scaled(size(), Qt::KeepAspectRatioByExpanding,
                                                     Qt::SmoothTransformation);
    const int x = (scaled.width() - width()) / 2;
    const int y = (scaled.height() - height()) / 2;
    painter.drawPixmap(0, 0, scaled, x, y, width(), height());
}

void MenuWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // 游戏页与菜单共用同一个顶层窗口，始终覆盖整个客户区。
    if (m_gameWindow)
        m_gameWindow->setGeometry(rect());
}

void MenuWindow::setMenuControlsVisible(bool visible)
{
    for (QWidget* control : {static_cast<QWidget*>(ui.titleLabel),
                             static_cast<QWidget*>(ui.newGameBtn),
                             static_cast<QWidget*>(ui.loadGameBtn),
                             static_cast<QWidget*>(ui.mapEditorBtn),
                             static_cast<QWidget*>(ui.settingsBtn)}) {
        control->setVisible(visible);
    }
}

void MenuWindow::onNewGame()
{
    auto* game = new Game();
    game->generateClassicTower();
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
    if (m_gameWindow) {
        delete game;
        return;
    }
    // MainWindow 作为菜单窗口的子页面显示，不再创建第二个顶层窗口。
    m_gameWindow = new MainWindow(game, this);
    m_gameWindow->setWindowFlags(Qt::Widget);
    m_gameWindow->setAttribute(Qt::WA_DeleteOnClose);
    m_gameWindow->setGeometry(rect());
    m_gameWindow->loadAssets();
    setMenuControlsVisible(false);
    m_gameWindow->show();
    m_gameWindow->raise();
    m_gameWindow->activateWindow();
    m_gameWindow->setFocus();

    // 游戏页关闭时恢复同一窗口中的菜单控件。
    connect(m_gameWindow, &QWidget::destroyed, this, [this]() {
        m_gameWindow = nullptr;
        setMenuControlsVisible(true);
        raise();
        activateWindow();
        setFocus();
    });
    // 兼容旧版窗口生命周期设置；父窗口本身保持可见。
    m_gameWindow->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_gameWindow, &QObject::destroyed, this, &QWidget::show);

}
