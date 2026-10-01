#include "MenuWindow.h"
#include "MainWindow.h"
#include "SlidingPuzzlePage.h"
#include "Game2048Page.h"
#include "RhythmGamePage.h"
#include "Game/Game.h"
#include "Audio/GameAudio.h"

#include <QApplication>
#include <QMessageBox>
#include <QIcon>
#include <QPainter>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSettings>
#include <memory>

MenuWindow::MenuWindow(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    m_backgroundImage.load(QStringLiteral(":/images/backgrounds/mujica_theater.png"));
    setWindowTitle(QString::fromUtf8("MYGO!!!!! × Ave Mujica：梦限大魔塔"));
    // 与游戏页的布局尺寸一致，嵌入后窗口不会把地图和侧栏压缩到不可用。
    setMinimumSize(1100, 760);
    // 默认以 1600×900 启动，窗口仍可手动放大到更高分辨率。
    resize(1600, 900);
    setStyleSheet(
        "QWidget#MenuWindow { background-color: #0a0812; color: #e8e9f2; }"
        "QPushButton { color: #fff7d0; border-image: url(:/images/runtime/ui/button_texture.png) 18 24 18 24 stretch stretch; padding: 10px 18px; font-size: 15px; font-weight: 700; }"
        "QPushButton:hover { color: #ffffff; }"
        "QLabel { color: #dfe3f5; }"
    );
    const QString buttonArt =
        "QPushButton { color: #fff7d0; border-image: url(:/images/runtime/ui/button_texture.png) 18 24 18 24 stretch stretch; padding: 10px 18px; font-size: 20px; font-weight: 700; }"
        "QPushButton:hover { color: white; }";
    for (QPushButton* button : {ui.newGameBtn, ui.loadGameBtn, ui.settingsBtn, ui.puzzleBtn, ui.mergeBtn, ui.rhythmBtn})
        button->setStyleSheet(buttonArt);
    ui.newGameBtn->setIcon(QIcon(":/images/runtime/items/weapon.png"));
    ui.loadGameBtn->setIcon(QIcon(":/images/runtime/items/treasure.png"));
    ui.settingsBtn->setIcon(QIcon(":/images/runtime/items/stairs_down.png"));
    for (QPushButton* button : {ui.newGameBtn, ui.loadGameBtn, ui.settingsBtn})
        button->setIconSize(QSize(38, 38));
    ui.newGameBtn->setText(QString::fromUtf8("新 游 戏"));
    ui.loadGameBtn->setText(QString::fromUtf8("读 取 存 档"));
    ui.settingsBtn->setText(QString::fromUtf8("设 置"));
    ui.titleLabel->setStyleSheet(
        "color: #fff2bd; background: transparent; padding: 8px 0;");
    positionMenuPortraits();

    GameAudio::prepare();
    for (QPushButton* button : {ui.newGameBtn, ui.loadGameBtn, ui.settingsBtn,
                                ui.puzzleBtn, ui.mergeBtn})
        connect(button, &QPushButton::clicked, this, []() {
            GameAudio::play(GameAudio::Cue::MenuSelect);
        });

    connect(ui.newGameBtn,   &QPushButton::clicked, this, &MenuWindow::onNewGame);
    connect(ui.loadGameBtn,  &QPushButton::clicked, this, &MenuWindow::onLoadGame);
    connect(ui.settingsBtn,  &QPushButton::clicked, this, &MenuWindow::onSettings);
    connect(ui.puzzleBtn, &QPushButton::clicked, this, &MenuWindow::onPuzzle);
    connect(ui.mergeBtn, &QPushButton::clicked, this, &MenuWindow::on2048);
    connect(ui.rhythmBtn, &QPushButton::clicked, this, &MenuWindow::onRhythm);
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
    positionMenuPortraits();
    // 游戏页与菜单共用同一个顶层窗口，始终覆盖整个客户区。
    if (m_gameWindow)
        m_gameWindow->setGeometry(rect());
    if (m_puzzlePage)
        m_puzzlePage->setGeometry(rect());
    if (m_mergePage)
        m_mergePage->setGeometry(rect());
    if (m_rhythmPage)
        m_rhythmPage->setGeometry(rect());
}

void MenuWindow::positionMenuPortraits()
{
    if (!ui.anonPortrait || !ui.soyoPortrait) return;
    const int portraitWidth = qBound(220, width() / 5, 330);
    const int portraitHeight = qBound(340, height() * 5 / 6, 560);
    const int top = qMax(0, height() - portraitHeight - 18);
    const int sideMargin = qMax(12, (width() - portraitWidth * 2 - 400) / 2);

    auto place = [portraitWidth, portraitHeight, top](QLabel* label, const QString& path) {
        if (!label) return;
        label->setGeometry(0, top, portraitWidth, portraitHeight);
        const QPixmap source(path);
        label->setPixmap(source.scaled(portraitWidth, portraitHeight,
                                       Qt::KeepAspectRatio, Qt::SmoothTransformation));
    };
    place(ui.anonPortrait, QStringLiteral(":/images/characters/portraits/anon.png"));
    place(ui.soyoPortrait, QStringLiteral(":/images/characters/portraits/soyo_stage.png"));
    ui.anonPortrait->move(sideMargin, top);
    ui.soyoPortrait->move(width() - sideMargin - portraitWidth, top);
    ui.anonPortrait->raise();
    ui.soyoPortrait->raise();
}

void MenuWindow::setMenuControlsVisible(bool visible)
{
    for (QWidget* control : {static_cast<QWidget*>(ui.titleLabel),
                             static_cast<QWidget*>(ui.newGameBtn),
                             static_cast<QWidget*>(ui.loadGameBtn),
                             static_cast<QWidget*>(ui.settingsBtn),
                             static_cast<QWidget*>(ui.puzzleBtn),
                             static_cast<QWidget*>(ui.mergeBtn),
                             static_cast<QWidget*>(ui.rhythmBtn),
                             static_cast<QWidget*>(ui.anonPortrait),
                             static_cast<QWidget*>(ui.soyoPortrait)}) {
        control->setVisible(visible);
    }
}

void MenuWindow::onNewGame()
{
    auto* game = new Game();
    game->generateClassicTower();
    enterGame(game, true);
}

void MenuWindow::onPuzzle()
{
    if (m_puzzlePage) return;
    GameAudio::startMiniGameMusic();
    m_puzzlePage = new SlidingPuzzlePage(this);
    m_puzzlePage->setGeometry(rect());
    setMenuControlsVisible(false);
    m_puzzlePage->show();
    m_puzzlePage->raise();
    connect(m_puzzlePage, &SlidingPuzzlePage::returnToMenu, this, [this]() {
        GameAudio::stopMiniGameMusic();
        m_puzzlePage->hide();
        m_puzzlePage->deleteLater();
        m_puzzlePage = nullptr;
        setMenuControlsVisible(true);
        setFocus();
    });
}

void MenuWindow::on2048()
{
    if (m_mergePage) return;
    GameAudio::startMiniGameMusic();
    m_mergePage = new Game2048Page(this);
    m_mergePage->setGeometry(rect());
    setMenuControlsVisible(false);
    m_mergePage->show();
    m_mergePage->raise();
    m_mergePage->setFocus();
    connect(m_mergePage, &Game2048Page::returnToMenu, this, [this]() {
        GameAudio::stopMiniGameMusic();
        m_mergePage->hide();
        m_mergePage->deleteLater();
        m_mergePage = nullptr;
        setMenuControlsVisible(true);
        setFocus();
    });
}

void MenuWindow::onLoadGame()
{
    auto game = std::make_unique<Game>();
    if (MainWindow::loadFromSlots(*game, this)) enterGame(game.release(), false);
}

void MenuWindow::onRhythm()
{
    if (m_rhythmPage) return;
    GameAudio::stopMiniGameMusic();
    m_rhythmPage = new RhythmGamePage(this);
    m_rhythmPage->setGeometry(rect());
    setMenuControlsVisible(false);
    m_rhythmPage->show(); m_rhythmPage->raise(); m_rhythmPage->setFocus();
    connect(m_rhythmPage, &RhythmGamePage::returnToMenu, this, [this]() {
        m_rhythmPage->hide(); m_rhythmPage->deleteLater(); m_rhythmPage=nullptr;
        setMenuControlsVisible(true); setFocus();
    });
}

void MenuWindow::onSettings()
{
    QSettings settings(QStringLiteral("MyGO-Mota"), QStringLiteral("MyGO-Mota"));
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("设置"));
    dlg.setFixedSize(420, 410);
    auto* layout = new QVBoxLayout(&dlg);
    auto* animation = new QCheckBox(QString::fromUtf8("启用连续移动动画"), &dlg);
    animation->setChecked(settings.value(QStringLiteral("movementAnimation"), true).toBool());
    auto* battle = new QCheckBox(QString::fromUtf8("显示战斗结果提示"), &dlg);
    battle->setChecked(settings.value(QStringLiteral("battleFeedback"), true).toBool());
    layout->addWidget(animation);
    layout->addWidget(battle);
    auto* sound = new QCheckBox(QString::fromUtf8("启用音效"), &dlg);
    sound->setChecked(settings.value(QStringLiteral("soundEffectsEnabled"), true).toBool());
    layout->addWidget(sound);
    auto* volumeRow = new QHBoxLayout;
    volumeRow->addWidget(new QLabel(QString::fromUtf8("音效音量"), &dlg));
    auto* volume = new QSlider(Qt::Horizontal, &dlg);
    volume->setRange(0, 100);
    volume->setValue(settings.value(QStringLiteral("soundEffectsVolume"), 65).toInt());
    volumeRow->addWidget(volume, 1);
    auto* volumeValue = new QLabel(QString::number(volume->value()) + "%", &dlg);
    volumeRow->addWidget(volumeValue);
    connect(volume, &QSlider::valueChanged, volumeValue, [volumeValue](int value) {
        volumeValue->setText(QString::number(value) + "%");
    });
    volume->setEnabled(sound->isChecked());
    connect(sound, &QCheckBox::toggled, volume, &QWidget::setEnabled);
    layout->addLayout(volumeRow);
    auto* music = new QCheckBox(QString::fromUtf8("启用小游戏背景音乐"), &dlg);
    music->setChecked(settings.value(QStringLiteral("backgroundMusicEnabled"), true).toBool());
    layout->addWidget(music);
    auto* musicVolumeRow = new QHBoxLayout;
    musicVolumeRow->addWidget(new QLabel(QString::fromUtf8("音乐音量"), &dlg));
    auto* musicVolume = new QSlider(Qt::Horizontal, &dlg);
    musicVolume->setRange(0, 100);
    musicVolume->setValue(settings.value(QStringLiteral("backgroundMusicVolume"), 38).toInt());
    musicVolumeRow->addWidget(musicVolume, 1);
    auto* musicValue = new QLabel(QString::number(musicVolume->value()) + "%", &dlg);
    musicVolumeRow->addWidget(musicValue);
    connect(musicVolume, &QSlider::valueChanged, musicValue, [musicValue](int value) {
        musicValue->setText(QString::number(value) + "%");
    });
    musicVolume->setEnabled(music->isChecked());
    connect(music, &QCheckBox::toggled, musicVolume, &QWidget::setEnabled);
    layout->addLayout(musicVolumeRow);
    auto* musicTrackRow = new QHBoxLayout;
    musicTrackRow->addWidget(new QLabel(QString::fromUtf8("小游戏曲目"), &dlg));
    auto* musicTrack = new QComboBox(&dlg);
    musicTrack->setObjectName(QStringLiteral("settingsMusicTrack"));
    musicTrack->addItems({QString::fromUtf8("春日影 · 8-bit"),
                          QString::fromUtf8("KiLLKiSS · 8-bit")});
    musicTrack->setCurrentIndex(GameAudio::miniGameMusicTrack());
    musicTrack->setEnabled(music->isChecked());
    connect(music, &QCheckBox::toggled, musicTrack, &QWidget::setEnabled);
    musicTrackRow->addWidget(musicTrack, 1);
    layout->addLayout(musicTrackRow);
    layout->addWidget(new QLabel(QString::fromUtf8(
        "方向键：移动\n"
        "固定道具栏：点击图标查看或使用\n"
        "消耗品按原版规则使用。"), &dlg));
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    buttons->button(QDialogButtonBox::Ok)->setText(QString::fromUtf8("应用"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QString::fromUtf8("取消"));
    auto* exitButton = new QPushButton(QString::fromUtf8("退出游戏"), &dlg);
    exitButton->setStyleSheet(QStringLiteral("QPushButton { color: #ff9a9a; }"));
    layout->addStretch();
    layout->addWidget(buttons);
    layout->addWidget(exitButton);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, [&]() {
        settings.setValue(QStringLiteral("movementAnimation"), animation->isChecked());
        settings.setValue(QStringLiteral("battleFeedback"), battle->isChecked());
        settings.setValue(QStringLiteral("soundEffectsEnabled"), sound->isChecked());
        settings.setValue(QStringLiteral("soundEffectsVolume"), volume->value());
        settings.setValue(QStringLiteral("backgroundMusicEnabled"), music->isChecked());
        settings.setValue(QStringLiteral("backgroundMusicVolume"), musicVolume->value());
        settings.setValue(QStringLiteral("backgroundMusicTrack"), musicTrack->currentIndex());
        GameAudio::refreshSettings();
        GameAudio::play(GameAudio::Cue::MenuSelect);
        dlg.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(exitButton, &QPushButton::clicked, &dlg, [&dlg]() {
        if (QMessageBox::question(&dlg, QString::fromUtf8("退出游戏"),
                QString::fromUtf8("确定要退出游戏吗？"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
            QApplication::quit();
        }
    });
    dlg.exec();
}

void MenuWindow::enterGame(Game* game, bool isNewGame)
{
    if (m_gameWindow) {
        delete game;
        return;
    }
    // MainWindow 作为菜单窗口的子页面显示，不再创建第二个顶层窗口。
    m_gameWindow = new MainWindow(game, this, isNewGame, true);
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
}
