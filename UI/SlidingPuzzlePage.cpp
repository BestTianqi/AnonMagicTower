#include "SlidingPuzzlePage.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QButtonGroup>
#include <QFrame>
#include <QRandomGenerator>
#include <QSettings>

SlidingPuzzleBoard::SlidingPuzzleBoard(const QPixmap& picture, QWidget* parent)
    : QWidget(parent), m_picture(picture)
{
    setObjectName("slidingPuzzleBoard");
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setMinimumSize(380, 380);
    m_animation.setStartValue(0.0);
    m_animation.setEndValue(1.0);
    m_animation.setDuration(135);
    m_animation.setEasingCurve(QEasingCurve::OutCubic);
    connect(&m_animation, &QVariantAnimation::valueChanged, this, [this]() { update(); });
    connect(&m_animation, &QVariantAnimation::finished, this, [this]() {
        m_animatedValue = 0;
        update();
        emit changed();
    });
    newPuzzle(3);
}

void SlidingPuzzleBoard::newPuzzle(int size)
{
    m_animation.stop();
    m_animatedValue = 0;
    m_state.reset(size);
    m_state.shuffle(QRandomGenerator::global()->generate());
    m_initial = m_state;
    m_hover = -1;
    update();
    emit changed();
}

void SlidingPuzzleBoard::restart()
{
    m_animation.stop();
    m_animatedValue = 0;
    m_state = m_initial;
    update();
    emit changed();
}

void SlidingPuzzleBoard::undo()
{
    if (m_animation.state() == QAbstractAnimation::Running || m_preview) return;
    if (m_state.undo()) { update(); emit changed(); }
    setFocus();
}

void SlidingPuzzleBoard::setPreview(bool on) { m_preview = on; update(); }
void SlidingPuzzleBoard::setNumbers(bool on) { m_numbers = on; update(); }

QRectF SlidingPuzzleBoard::boardRect() const
{
    const qreal side = qMin(width(), height()) - 24;
    return QRectF((width() - side) / 2, (height() - side) / 2, side, side);
}

QRectF SlidingPuzzleBoard::tileRect(int index) const
{
    const QRectF board = boardRect();
    const qreal cell = board.width() / m_state.size();
    return QRectF(board.x() + index % m_state.size() * cell,
                  board.y() + index / m_state.size() * cell, cell, cell);
}

int SlidingPuzzleBoard::indexAt(const QPointF& position) const
{
    const QRectF board = boardRect();
    if (!board.contains(position)) return -1;
    const qreal cell = board.width() / m_state.size();
    const int x = qMin(m_state.size() - 1, int((position.x() - board.x()) / cell));
    const int y = qMin(m_state.size() - 1, int((position.y() - board.y()) / cell));
    return y * m_state.size() + x;
}

void SlidingPuzzleBoard::drawTile(QPainter& p, int value, const QRectF& target, bool highlight)
{
    const QRectF tile = target.adjusted(1.5, 1.5, -1.5, -1.5);
    QPainterPath clip;
    clip.addRoundedRect(tile, 5, 5);
    p.save();
    p.setClipPath(clip);
    const qreal sourceSide = qreal(m_picture.width()) / m_state.size();
    const int sourceIndex = value - 1;
    p.drawPixmap(tile, m_picture, QRectF(sourceIndex % m_state.size() * sourceSide,
        sourceIndex / m_state.size() * sourceSide, sourceSide, sourceSide));
    p.restore();
    p.setPen(QPen(highlight ? QColor("#ffb5d1") : QColor(255, 235, 212, 75), highlight ? 2 : 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(tile, 5, 5);
    if (m_numbers) {
        const int fontSize = qBound(11, int(tile.width() * .17), 22);
        QFont font("Segoe UI", fontSize, QFont::DemiBold);
        p.setFont(font);
        const qreal badge = fontSize * 1.65;
        const QRectF label(tile.x() + 5, tile.y() + 5, badge + 5, badge);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(18, 13, 31, 195));
        p.drawRoundedRect(label, 5, 5);
        p.setPen(QColor("#fff4df"));
        p.drawText(label, Qt::AlignCenter, QString::number(value));
    }
}

void SlidingPuzzleBoard::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    const QRectF board = boardRect();
    p.setPen(QPen(QColor("#b99771"), 1.5));
    p.setBrush(QColor("#181120"));
    p.drawRoundedRect(board.adjusted(-7, -7, 7, 7), 13, 13);
    if (m_preview || (m_state.solved() && !m_animatedValue)) {
        QPainterPath clip; clip.addRoundedRect(board, 7, 7);
        p.setClipPath(clip);
        p.drawPixmap(board, m_picture, m_picture.rect());
        return;
    }
    const QRectF blank = tileRect(m_state.blank()).adjusted(3, 3, -3, -3);
    p.setPen(QPen(QColor("#5e4866"), 1, Qt::DashLine));
    p.setBrush(QColor("#211729"));
    p.drawRoundedRect(blank, 7, 7);
    p.setPen(QColor("#816987"));
    p.setFont(QFont("Segoe UI", qMax(16, int(blank.width() / 4))));
    p.drawText(blank, Qt::AlignCenter, QString::fromUtf8("♪"));
    for (int i = 0; i < static_cast<int>(m_state.tiles().size()); ++i) {
        const int value = m_state.tiles()[i];
        if (value == 0 || (m_animatedValue && i == m_to)) continue;
        drawTile(p, value, tileRect(i), i == m_hover && m_state.canMove(i));
    }
    if (m_animatedValue) {
        const qreal t = m_animation.currentValue().toReal();
        const QRectF from = tileRect(m_from), to = tileRect(m_to);
        drawTile(p, m_animatedValue, QRectF(from.topLeft() + (to.topLeft() - from.topLeft()) * t,
                                           from.size()), true);
    }
}

void SlidingPuzzleBoard::slide(int index)
{
    if (m_preview || m_state.solved() || m_animation.state() == QAbstractAnimation::Running ||
        !m_state.canMove(index)) return;
    m_from = index;
    m_to = m_state.blank();
    m_animatedValue = m_state.tiles()[index];
    m_state.move(index);
    const QSettings settings("MyGO-Mota", "MyGO-Mota");
    m_animation.setDuration(settings.value("movementAnimation", true).toBool() ? 135 : 0);
    m_animation.start();
    emit changed();
}

void SlidingPuzzleBoard::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) { setFocus(); slide(indexAt(e->position())); }
}
void SlidingPuzzleBoard::mouseMoveEvent(QMouseEvent* e)
{
    m_hover = indexAt(e->position());
    setCursor(m_state.canMove(m_hover) && !m_preview ? Qt::PointingHandCursor : Qt::ArrowCursor);
    update();
}
void SlidingPuzzleBoard::leaveEvent(QEvent*) { m_hover = -1; update(); }
bool SlidingPuzzleBoard::moveByArrow(int key)
{
    const int blank = m_state.blank(), size = m_state.size();
    int target = -1;
    // 方向键表示图片块滑入空格的方向，因此空格朝反方向移动。
    if (key == Qt::Key_Left && blank % size + 1 < size) target = blank + 1;
    if (key == Qt::Key_Right && blank % size > 0) target = blank - 1;
    if (key == Qt::Key_Up && blank / size + 1 < size) target = blank + size;
    if (key == Qt::Key_Down && blank / size > 0) target = blank - size;
    if (target >= 0) slide(target);
    return key == Qt::Key_Left || key == Qt::Key_Right ||
           key == Qt::Key_Up || key == Qt::Key_Down;
}

SlidingPuzzlePage::SlidingPuzzlePage(QWidget* parent) : QWidget(parent)
{
    setObjectName("slidingPuzzlePage");
    const QPixmap original(":/images/backgrounds/anon_soyo_puzzle.jpg");
    const int side = qMin(original.width(), original.height());
    m_picture = original.copy(int((original.width() - side) * .62),
                              (original.height() - side) / 2, side, side);
    setStyleSheet(
        "QWidget#slidingPuzzlePage { color:#f6e9ee; }"
        "QLabel { color:#f6e9ee; background:transparent; }"
        "QFrame#puzzlePanel { background:#211a2c; border:1px solid #665068; border-radius:16px; }"
        "QPushButton { border-image:none; background:#33243e; border:1px solid #735777; border-radius:8px; color:#f9ebed; padding:9px 12px; font:600 15px 'Microsoft YaHei'; }"
        "QPushButton:hover { background:#51334e; border-color:#eaa5bf; }"
        "QPushButton:focus { border:2px solid #f2b3ca; }"
        "QPushButton:checked, QPushButton#puzzleShuffle { background:#aa5a7d; border-color:#e4a7bd; color:white; }"
        "QPushButton:pressed { background:#74415f; }"
        "QPushButton:disabled { color:#786f83; border-color:#41394c; background:#292333; }"
        "QCheckBox { color:#ddc8dd; spacing:8px; font:14px 'Microsoft YaHei'; }"
        "QCheckBox::indicator { width:18px; height:18px; }"
    );
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(36, 24, 36, 24);
    root->setSpacing(14);
    auto* header = new QHBoxLayout;
    auto* titles = new QVBoxLayout;
    auto* eyebrow = new QLabel(QString::fromUtf8("MYGO!!!!!  /  MEMORY SESSION"), this);
    eyebrow->setStyleSheet("color:#d6b594; font:600 12px 'Segoe UI'; letter-spacing:2px;");
    auto* title = new QLabel(QString::fromUtf8("把这一刻，拼回来。"), this);
    title->setStyleSheet("font:700 30px 'Microsoft YaHei'; color:#fff0e7;");
    titles->addWidget(eyebrow); titles->addWidget(title);
    header->addLayout(titles); header->addStretch();
    auto* back = new QPushButton(QString::fromUtf8("←  返回菜单"), this);
    back->setObjectName("puzzleBackButton");
    connect(back, &QPushButton::clicked, this, &SlidingPuzzlePage::returnToMenu);
    header->addWidget(back);
    root->addLayout(header);

    auto* content = new QHBoxLayout;
    content->setSpacing(28);
    m_board = new SlidingPuzzleBoard(m_picture, this);
    content->addWidget(m_board, 1);
    auto* panel = new QFrame(this);
    panel->setObjectName("puzzlePanel");
    panel->setFixedWidth(290);
    auto* controls = new QVBoxLayout(panel);
    controls->setContentsMargins(18, 18, 18, 18);
    controls->setSpacing(8);
    auto* caption = new QLabel(QString::fromUtf8("夕照合影"), panel);
    caption->setStyleSheet("color:#f2cba2; font:700 23px 'Microsoft YaHei';");
    controls->addWidget(caption);
    auto* sub = new QLabel(QString::fromUtf8("数字华容道 · 爱音 & 素世"), panel);
    sub->setStyleSheet("color:#bdacc6; font:13px 'Microsoft YaHei';");
    controls->addWidget(sub);
    controls->addWidget(new QLabel(QString::fromUtf8("选择难度"), panel));
    auto* difficulties = new QGridLayout;
    auto* group = new QButtonGroup(this);
    for (int size = 3; size <= 8; ++size) {
        auto* choice = new QPushButton(QString::fromUtf8("%1 × %1").arg(size), panel);
        choice->setObjectName(QStringLiteral("puzzleDifficulty%1").arg(size));
        choice->setCheckable(true);
        choice->setChecked(size == 3);
        group->addButton(choice, size);
        difficulties->addWidget(choice, (size - 3) / 3, (size - 3) % 3);
        connect(choice, &QPushButton::clicked, this, [this, size]() {
            resetClock(); m_board->newPuzzle(size); m_board->setFocus();
        });
    }
    controls->addLayout(difficulties);
    auto* stats = new QHBoxLayout;
    m_moves = new QLabel(panel); m_time = new QLabel("00:00", panel);
    m_moves->setObjectName("puzzleMoves"); m_time->setObjectName("puzzleTime");
    m_moves->setStyleSheet("font:600 22px 'Segoe UI'; color:#ffcbde;");
    m_time->setStyleSheet("font:600 22px 'Segoe UI'; color:#f2cba2;");
    stats->addWidget(m_moves); stats->addStretch(); stats->addWidget(m_time);
    controls->addLayout(stats);
    m_status = new QLabel(panel);
    m_status->setWordWrap(true);
    m_status->setMinimumHeight(44);
    m_status->setStyleSheet("color:#cbb8d1; font:13px 'Microsoft YaHei';");
    controls->addWidget(m_status);
    auto* shuffle = new QPushButton(QString::fromUtf8("重新洗牌"), panel);
    shuffle->setObjectName("puzzleShuffle");
    connect(shuffle, &QPushButton::clicked, this, [this]() {
        resetClock(); m_board->newPuzzle(m_board->state().size()); m_board->setFocus();
    });
    controls->addWidget(shuffle);
    auto* actions = new QHBoxLayout;
    m_undo = new QPushButton(QString::fromUtf8("撤销"), panel);
    m_undo->setObjectName("puzzleUndo");
    auto* restart = new QPushButton(QString::fromUtf8("重试本局"), panel);
    connect(m_undo, &QPushButton::clicked, m_board, &SlidingPuzzleBoard::undo);
    connect(restart, &QPushButton::clicked, this, [this]() {
        resetClock(); m_board->restart(); m_board->setFocus();
    });
    actions->addWidget(m_undo); actions->addWidget(restart); controls->addLayout(actions);
    auto* preview = new QPushButton(QString::fromUtf8("按住查看原图"), panel);
    connect(preview, &QPushButton::pressed, this, [this]() { m_board->setPreview(true); });
    connect(preview, &QPushButton::released, this, [this]() { m_board->setPreview(false); m_board->setFocus(); });
    controls->addWidget(preview);
    auto* numbers = new QCheckBox(QString::fromUtf8("显示数字辅助"), panel);
    numbers->setChecked(true);
    connect(numbers, &QCheckBox::toggled, m_board, &SlidingPuzzleBoard::setNumbers);
    controls->addWidget(numbers);
    controls->addStretch();
    content->addWidget(panel);
    root->addLayout(content, 1);
    auto* tip = new QLabel(QString::fromUtf8("点击空格旁的图片滑动  ·  方向键推动相邻图片  ·  按数字从左到右、从上到下复原，右下角留空"), this);
    tip->setStyleSheet("color:#b8a8bf; font:13px 'Microsoft YaHei';");
    tip->setAlignment(Qt::AlignCenter);
    root->addWidget(tip);
    connect(m_board, &SlidingPuzzleBoard::changed, this, &SlidingPuzzlePage::refresh);
    m_clock.setInterval(200);
    connect(&m_clock, &QTimer::timeout, this, [this]() {
        const qint64 seconds = m_elapsed.elapsed() / 1000;
        m_time->setText(QStringLiteral("%1:%2").arg(seconds / 60, 2, 10, QLatin1Char('0'))
            .arg(seconds % 60, 2, 10, QLatin1Char('0')));
    });
    refresh();
    m_board->setFocus();
    installEventFilter(this);
    for (QWidget* child : findChildren<QWidget*>())
        child->installEventFilter(this);
}

bool SlidingPuzzlePage::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress &&
        (watched == this || isAncestorOf(qobject_cast<QWidget*>(watched)))) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (key->modifiers() == Qt::NoModifier && m_board->moveByArrow(key->key())) {
            key->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SlidingPuzzlePage::resetClock()
{
    m_clock.stop(); m_started = false; m_finished = false;
    m_time->setText("00:00");
}

void SlidingPuzzlePage::refresh()
{
    const auto& state = m_board->state();
    m_moves->setText(QString::fromUtf8("%1 步").arg(state.moves()));
    m_undo->setEnabled(state.moves() > 0);
    if (!m_started && state.moves() > 0) {
        m_elapsed.start(); m_clock.start(); m_started = true;
    }
    if (state.solved()) {
        m_clock.stop(); m_finished = true;
        m_status->setText(QString::fromUtf8("合影复原了！\n这一刻，终于又在一起。"));
    } else {
        if (m_finished) { m_clock.start(); m_finished = false; }
        m_status->setText(QString::fromUtf8("每次移动一块相邻图片。\n第一次移动后开始计时。"));
    }
}

void SlidingPuzzlePage::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), QColor("#100d1b"));
    p.setOpacity(.13);
    const auto backdrop = m_picture.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    p.drawPixmap((width() - backdrop.width()) / 2, (height() - backdrop.height()) / 2, backdrop);
    p.setOpacity(1);
    QLinearGradient glow(0, 0, width(), height());
    glow.setColorAt(0, QColor(62, 33, 68, 115));
    glow.setColorAt(1, QColor(14, 12, 28, 210));
    p.fillRect(rect(), glow);
}
