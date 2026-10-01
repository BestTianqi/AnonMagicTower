#include "Game2048Page.h"
#include "Audio/GameAudio.h"
#include "MiniGameSession.h"
#include <QScrollArea>
#include <QSignalBlocker>
#include <QApplication>
#include <QFrame>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <cmath>

namespace {
using Direction = Game2048State::Direction;
int levelFor(int value) {
    int level = -1;
    for (; value > 1; value >>= 1) ++level;
    return std::max(0, level);
}
struct TileTheme {
    const char* name;
    const char* color;
    int monsterId; // 0 uses the protagonist's walking artwork.
};
constexpr std::array<TileTheme, Game2048Board::CharacterCount> themes{{
    {"要乐奈", "#749ca6", 1}, {"高松灯", "#84a5ca", 4},
    {"椎名立希", "#b687aa", 5}, {"若叶睦", "#96b799", 2},
    {"丰川祥子", "#969cdb", 3}, {"祐天寺若麦", "#c286c3", 6},
    {"八幡海铃", "#8fbeb8", 8}, {"三角初华", "#d8b694", 7},
    {"藤都子", "#d28cab", 13}, {"千早爱音", "#ef9dbc", 0},
    {"长崎素世", "#e9ba86", 34}, {"凑友希那", "#d6a4d3", 16},
    {"户山香澄", "#f3c87c", 17}, {"薇欧拉", "#c171dc", 11},
    {"藤都子SP", "#cab15d", 31}, {"峰月律SP", "#e373b0", 27},
    {"长崎素世SP", "#f5d683", 33},
}};
}

QString Game2048Board::characterName(int level) {
    return QString::fromUtf8(themes[qBound(0, level, CharacterCount - 1)].name);
}

QPixmap Game2048Board::characterImage(int level) {
    const int monsterId = themes[qBound(0, level, CharacterCount - 1)].monsterId;
    if (monsterId == 0) {
        const QPixmap sheet(":/images/characters/player_outfits/anon_reference_walk_8x8_hq.png");
        return sheet.copy(0, 0, sheet.width() / 8, sheet.height() / 8);
    }
    return QPixmap(QStringLiteral(":/images/characters/monsters/monster_%1.png")
        .arg(monsterId, 2, 10, QLatin1Char('0')));
}

Game2048Board::Game2048Board(QWidget* parent) : QWidget(parent) {
    setObjectName("game2048Board");
    setMinimumSize(400, 400);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    for (int i = 0; i < CharacterCount; ++i) m_characters[i] = characterImage(i);
    m_animation.setDuration(145);
    m_animation.setStartValue(0.0);
    m_animation.setEndValue(1.0);
    m_animation.setEasingCurve(QEasingCurve::OutCubic);
    connect(&m_animation, &QVariantAnimation::valueChanged, this, [this]() { update(); });
    connect(&m_animation, &QVariantAnimation::finished, this, [this]() { update(); });
}

QRectF Game2048Board::tileRect(int index) const {
    const qreal side = qMin(width(), height()) - 24;
    const qreal gap = 10;
    const qreal cell = (side - gap * 5) / 4;
    return QRectF((width() - side) / 2 + gap + (index % 4) * (cell + gap),
                  (height() - side) / 2 + gap + (index / 4) * (cell + gap), cell, cell);
}

bool Game2048Board::move(Direction direction) {
    if (m_paused || m_animation.state() == QAbstractAnimation::Running) return false;
    const auto slide = Game2048State::slide(m_state.tiles(), direction);
    const int highestBefore = m_state.highestTile();
    if (!m_state.move(direction)) return false;
    if (m_state.highestTile() > highestBefore && m_state.highestTile() >= 2048)
        GameAudio::play(GameAudio::Cue::MergeGoal);
    else if (slide.gain >= 64)
        GameAudio::play(GameAudio::Cue::MergeLarge);
    else if (slide.gain > 0)
        GameAudio::play(GameAudio::Cue::MergeSmall);
    else
        GameAudio::play(GameAudio::Cue::MergeMove);
    m_motions = slide.motions;
    QSettings settings(QSettings::defaultFormat(), QSettings::UserScope, "MyGO-Mota", "MyGO-Mota");
    if (settings.value("movementAnimation", true).toBool()) m_animation.start();
    update();
    emit changed();
    return true;
}

void Game2048Board::restart() {
    GameAudio::play(GameAudio::Cue::MenuSelect);
    m_animation.stop(); m_swiping = false; m_state.restart(); update(); emit changed();
}

void Game2048Board::undo() {
    if(m_paused)return;
    m_animation.stop();
    if (m_state.undo()) {
        GameAudio::play(GameAudio::Cue::Undo);
        update(); emit changed();
    }
}
void Game2048Board::setPaused(bool paused){m_paused=paused;m_animation.stop();m_swiping=false;update();}
QJsonObject Game2048Board::snapshot() const {return {{"state",QString::fromStdString(m_state.serialize())}};}
bool Game2048Board::restoreSnapshot(const QJsonObject& data){
    Game2048State candidate(0);if(!candidate.restore(data["state"].toString().toStdString()))return false;
    m_animation.stop();m_swiping=false;m_state=std::move(candidate);update();emit changed();return true;
}

void Game2048Board::drawTile(QPainter& painter, int value, const QRectF& target) {
    const int level = levelFor(value);
    const QColor accent(themes[qMin(level, CharacterCount - 1)].color);
    painter.setPen(QPen(accent, value >= 2048 ? 2.5 : 1));
    painter.setBrush(accent.darker(330));
    painter.drawRoundedRect(target, 12, 12);
    const QRectF pictureBox = target.adjusted(5, 3, -5, -target.height() * .23);
    const QPixmap& picture = m_characters[qMin(level, CharacterCount - 1)];
    if (!picture.isNull()) {
        const QSizeF fitted = QSizeF(picture.size()).scaled(pictureBox.size(), Qt::KeepAspectRatio);
        const QRectF imageBox(pictureBox.center() - QPointF(fitted.width()/2, fitted.height()/2), fitted);
        painter.drawPixmap(imageBox, picture, picture.rect());
    }
    painter.setPen(accent.lighter(140));
    QFont number("Segoe UI"); number.setBold(true); number.setPixelSize(qMax(18, int(target.width() * .19)));
    painter.setFont(number);
    painter.drawText(target.adjusted(3, target.height() * .73, -3, -2), Qt::AlignCenter, QString::number(value));
}

void Game2048Board::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    const auto bounds = tileRect(0).united(tileRect(15)).adjusted(-10, -10, 10, 10);
    painter.setPen(QPen(QColor("#785b77"), 1));
    painter.setBrush(QColor("#191622"));
    painter.drawRoundedRect(bounds, 19, 19);
    if(m_paused){painter.setFont(QFont("Microsoft YaHei",24,QFont::DemiBold));painter.setPen(QColor("#ffd0e2"));painter.drawText(bounds,Qt::AlignCenter,QString::fromUtf8("合奏暂停\n\n点击「继续」或按空格\n棋盘与撤销记录均已保留"));return;}
    for (int i = 0; i < 16; ++i) {
        painter.setPen(Qt::NoPen); painter.setBrush(QColor("#302635"));
        painter.drawRoundedRect(tileRect(i), 12, 12);
    }
    if (m_animation.state() == QAbstractAnimation::Running) {
        const qreal progress = m_animation.currentValue().toReal();
        for (const auto& motion : m_motions) {
            QRectF tile = tileRect(motion.from);
            tile.moveTopLeft(tile.topLeft() + (tileRect(motion.to).topLeft() - tile.topLeft()) * progress);
            drawTile(painter, motion.value, tile);
        }
    } else {
        for (int i = 0; i < 16; ++i) if (m_state.tiles()[i]) drawTile(painter, m_state.tiles()[i], tileRect(i));
    }
}

void Game2048Board::mousePressEvent(QMouseEvent* event) {
    if (m_paused || event->button() != Qt::LeftButton) return;
    m_press = event->position(); m_swiping = true; setFocus(); event->accept();
}

void Game2048Board::mouseReleaseEvent(QMouseEvent* event) {
    if (m_paused || event->button() != Qt::LeftButton || !m_swiping) return;
    m_swiping = false;
    const QPointF delta = event->position() - m_press;
    if (qMax(qAbs(delta.x()), qAbs(delta.y())) < 25) return;
    move(qAbs(delta.x()) > qAbs(delta.y()) ? (delta.x() > 0 ? Direction::Right : Direction::Left)
                                         : (delta.y() > 0 ? Direction::Down : Direction::Up));
}

Game2048Page::Game2048Page(QWidget* parent) : QWidget(parent),
    m_background(":/images/backgrounds/mujica_theater.png") {
    setObjectName("game2048Page");
    setStyleSheet(
        "QLabel { color:#f6e9ee; background:transparent; font:14px 'Microsoft YaHei'; }"
        "QFrame#mergePanel { background:#211a2c; border:1px solid #665068; border-radius:16px; }"
        "QPushButton { border-image:none; background:#33243e; border:1px solid #735777; border-radius:8px; color:#f9ebed; padding:9px 12px; font:600 15px 'Microsoft YaHei'; }"
        "QPushButton:hover { background:#51334e; border-color:#eaa5bf; }"
        "QPushButton:focus { border:2px solid #f2b3ca; }"
        "QPushButton#mergeRestart { background:#aa5a7d; }"
        "QPushButton:disabled { color:#786f83; border-color:#41394c; }"
        "QComboBox { background:#33243e; border:1px solid #735777; border-radius:7px; color:#f9ebed; padding:7px 10px; font:14px 'Microsoft YaHei'; }"
        "QComboBox QAbstractItemView { background:#211a2c; color:#f9ebed; selection-background-color:#74415f; }"
    );
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(36, 24, 36, 24); root->setSpacing(12);
    auto* header = new QHBoxLayout;
    auto* titles = new QVBoxLayout;
    auto* eyebrow = new QLabel("MYGO!!!!!  /  ENSEMBLE 2048", this);
    eyebrow->setStyleSheet("color:#d6b594; font:600 12px 'Segoe UI'; letter-spacing:2px;");
    auto* title = new QLabel(QString::fromUtf8("让相同的音符，相遇。"), this);
    title->setStyleSheet("font:700 30px 'Microsoft YaHei'; color:#fff0e7;");
    titles->addWidget(eyebrow); titles->addWidget(title); header->addLayout(titles); header->addStretch();
    auto* back = new QPushButton(QString::fromUtf8("←  返回菜单"), this);
    back->setObjectName("mergeBack");
    connect(back, &QPushButton::clicked, this, [this]{m_session->saveAuto();emit returnToMenu();});
    header->addWidget(back); root->addLayout(header);
    auto* content = new QHBoxLayout; content->setSpacing(24);
    m_board = new Game2048Board(this); content->addWidget(m_board, 1);
    auto* panel = new QFrame(this); panel->setObjectName("mergePanel"); panel->setFixedWidth(310);
    auto* controls = new QVBoxLayout(panel); controls->setContentsMargins(18, 16, 18, 16); controls->setSpacing(6);
    auto* caption = new QLabel(QString::fromUtf8("合奏 · 2048+"), panel);
    caption->setStyleSheet("color:#f2cba2; font:700 24px 'Microsoft YaHei';"); controls->addWidget(caption);
    auto* musicCredit = new QLabel(QString::fromUtf8("♪ 背景音乐 · 8-bit"), panel);
    musicCredit->setStyleSheet("color:#d6b594; font:12px 'Microsoft YaHei';");
    controls->addWidget(musicCredit);
    m_musicChoice = new QComboBox(panel);
    m_musicChoice->setObjectName("mergeMusicChoice");
    m_musicChoice->addItems({QString::fromUtf8("春日影 · CRYCHIC"),
                             QString::fromUtf8("KiLLKiSS · Ave Mujica")});
    m_musicChoice->setCurrentIndex(GameAudio::miniGameMusicTrack());
    m_musicChoice->setToolTip(QString::fromUtf8("选择小游戏背景音乐，切换后立即播放"));
    connect(m_musicChoice, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [](int track) { GameAudio::setMiniGameMusicTrack(track); });
    controls->addWidget(m_musicChoice);
    m_score = new QLabel(panel); m_score->setObjectName("mergeScore");
    m_bestLabel = new QLabel(panel); m_bestLabel->setObjectName("mergeBest");
    m_score->setStyleSheet("font:700 23px 'Microsoft YaHei'; color:#ffcbde;");
    controls->addWidget(m_score); controls->addWidget(m_bestLabel);
    m_status = new QLabel(panel); m_status->setWordWrap(true); m_status->setMinimumHeight(42);
    m_status->setObjectName("mergeStatus"); controls->addWidget(m_status);
    auto* actions = new QHBoxLayout;
    auto* restart = new QPushButton(QString::fromUtf8("重新开始"), panel); restart->setObjectName("mergeRestart");
    m_undo = new QPushButton(QString::fromUtf8("撤销"), panel); m_undo->setObjectName("mergeUndo");
    connect(restart, &QPushButton::clicked, this, [this]{m_session->resetProgress();m_board->restart();m_board->setFocus();});
    connect(m_undo, &QPushButton::clicked, m_board, &Game2048Board::undo);
    actions->addWidget(restart); actions->addWidget(m_undo); controls->addLayout(actions);
    auto* arrows = new QGridLayout;
    const Direction directions[] = {Direction::Up, Direction::Left, Direction::Down, Direction::Right};
    const char* labels[] = {"↑", "←", "↓", "→"};
    for (int i = 0; i < 4; ++i) {
        auto* button = new QPushButton(QString::fromUtf8(labels[i]), panel);
        button->setObjectName(QStringLiteral("mergeDirection%1").arg(i));
        button->setAccessibleName(QString::fromUtf8("向%1移动").arg(QString::fromUtf8("上左下右").mid(i, 1)));
        arrows->addWidget(button, 0, i);
        connect(button, &QPushButton::clicked, this, [this, direction=directions[i]]() { m_board->move(direction); });
    }
    controls->addLayout(arrows);
    auto* legendTitle = new QLabel(QString::fromUtf8("角色合成顺序"), panel);
    legendTitle->setStyleSheet("color:#f2cba2; font-weight:600;"); controls->addWidget(legendTitle);
    auto* legend = new QGridLayout; legend->setSpacing(3);
    for (int i = 0; i < Game2048Board::CharacterCount; ++i) {
        auto* row = new QWidget(panel); auto* layout = new QHBoxLayout(row); layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(5);
        auto* icon = new QLabel(row); icon->setFixedSize(24, 24);
        icon->setPixmap(Game2048Board::characterImage(i).scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        auto* label = new QLabel(QStringLiteral("%1  %2").arg(1 << (i+1)).arg(Game2048Board::characterName(i)), row);
        label->setStyleSheet("font:11px 'Microsoft YaHei'; color:#d8c9df;");
        layout->addWidget(icon); layout->addWidget(label); layout->addStretch(); legend->addWidget(row, i / 2, i % 2);
    }
    controls->addLayout(legend); controls->addStretch();
    auto* scroll=new QScrollArea(this);scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);scroll->setFixedWidth(332);scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);scroll->setStyleSheet("QScrollArea {background:transparent;border:none;}");scroll->setWidget(panel);content->addWidget(scroll);root->addLayout(content,1);
    auto* tip = new QLabel(QString::fromUtf8("方向键 / WASD：移动全部卡片  ·  鼠标按住棋盘滑动  ·  相同数字合并  ·  Ctrl+Z：撤销"), this);
    tip->setAlignment(Qt::AlignCenter); tip->setStyleSheet("color:#c8b4ca; font:13px 'Microsoft YaHei';"); root->addWidget(tip);
    QSettings settings(QSettings::defaultFormat(), QSettings::UserScope, "MyGO-Mota", "MyGO-Mota");
    m_best = settings.value("miniGames/2048/bestScore", 0).toInt();
    m_session=new MiniGameSession("merge",[this]{return m_board->snapshot();},[this](const QJsonObject& data){
        QSignalBlocker blocker(m_board);if(!m_board->restoreSnapshot(data))return false;refresh();return true;
    },[this](bool paused){m_board->setPaused(paused);m_undo->setEnabled(!paused && m_board->state().canUndo());},this);
    root->insertWidget(1,m_session);
    connect(m_board, &Game2048Board::changed, this, &Game2048Page::refresh);
    installEventFilter(this);
    for (auto* child : findChildren<QWidget*>()) child->installEventFilter(this);
    if(!m_session->restoreAuto())refresh();m_board->setFocus();
    connect(qApp,&QCoreApplication::aboutToQuit,this,[this]{m_session->saveAuto();});
}

Game2048Page::~Game2048Page(){if(m_session)m_session->saveAuto();}

void Game2048Page::refresh() {
    const auto& state = m_board->state();
    m_score->setText(QString::fromUtf8("得分  %1").arg(state.score()));
    if (state.score() > m_best) {
        m_best = state.score();
        QSettings settings(QSettings::defaultFormat(), QSettings::UserScope, "MyGO-Mota", "MyGO-Mota");
        settings.setValue("miniGames/2048/bestScore", m_best);
    }
    m_bestLabel->setText(QString::fromUtf8("最高分 %1    ·    %2 步").arg(m_best).arg(state.moves()));
    m_undo->setEnabled(state.canUndo() && !m_session->paused());
    m_status->setText(state.gameOver() ? QString::fromUtf8("已经没有可移动的位置。\n撤销一步，或重新开始。")
        : state.highestTile() >= 2048 ? QString::fromUtf8("合奏达成！已合成 %1。\n可以继续挑战更高数字。").arg(state.highestTile())
        : QString::fromUtf8("相同的角色相遇，合成下一位。\n一起向 2048 前进。"));
    m_session->changed(state.moves()>0,state.gameOver());
}

bool Game2048Page::eventFilter(QObject* watched, QEvent* event) {
    if(watched==this && event->type()==QEvent::WindowDeactivate)m_session->setPaused(true);
    if (qobject_cast<QComboBox*>(watched))
        return QWidget::eventFilter(watched, event);
    if (event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if(key->modifiers()==Qt::NoModifier && (key->key()==Qt::Key_Space || key->key()==Qt::Key_P || key->key()==Qt::Key_Escape)){if(!key->isAutoRepeat())m_session->togglePause();return true;}
        if(key->modifiers()==Qt::ControlModifier){if(key->key()==Qt::Key_S){m_session->saveSlot();return true;}if(key->key()==Qt::Key_L){m_session->loadSlot();return true;}}
        if (key->key() == Qt::Key_Z && key->modifiers() == Qt::ControlModifier) { m_board->undo(); return true; }
        if (key->modifiers() == Qt::NoModifier) {
            switch (key->key()) {
            case Qt::Key_Left: case Qt::Key_A: m_board->move(Direction::Left); return true;
            case Qt::Key_Right: case Qt::Key_D: m_board->move(Direction::Right); return true;
            case Qt::Key_Up: case Qt::Key_W: m_board->move(Direction::Up); return true;
            case Qt::Key_Down: case Qt::Key_S: m_board->move(Direction::Down); return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void Game2048Page::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor("#15101e"));
    if (!m_background.isNull()) {
        const QPixmap scaled = m_background.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        painter.drawPixmap((width()-scaled.width())/2, (height()-scaled.height())/2, scaled);
        painter.fillRect(rect(), QColor(17, 11, 26, 220));
    }
}
