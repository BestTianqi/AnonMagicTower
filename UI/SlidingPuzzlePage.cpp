#include "SlidingPuzzlePage.h"
#include "Audio/GameAudio.h"
#include "MiniGameSession.h"
#include <QApplication>
#include <QScrollArea>
#include <QFileDialog>
#include <QImageReader>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QBuffer>
#include <QIcon>
#include <QSignalBlocker>
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
#include <QComboBox>
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
    GameAudio::play(GameAudio::Cue::MenuSelect);
    m_animation.stop();
    m_animatedValue = 0;
    m_state = m_initial;
    update();
    emit changed();
}

void SlidingPuzzleBoard::undo()
{
    if (m_paused || m_animation.state() == QAbstractAnimation::Running || m_preview) return;
    if (m_state.undo()) {
        GameAudio::play(GameAudio::Cue::Undo);
        update(); emit changed();
    }
    setFocus();
}

void SlidingPuzzleBoard::setPreview(bool on) { m_preview = on; update(); }
void SlidingPuzzleBoard::setNumbers(bool on) { m_numbers = on; update(); }
void SlidingPuzzleBoard::setPicture(const QPixmap& picture){m_picture=picture;update();}
void SlidingPuzzleBoard::setPaused(bool paused){m_paused=paused;m_animation.stop();m_animatedValue=0;m_hover=-1;m_preview=false;update();}
QJsonObject SlidingPuzzleBoard::snapshot() const {
    return {{"state",QString::fromStdString(m_state.serialize())},{"initial",QString::fromStdString(m_initial.serialize())}};
}
bool SlidingPuzzleBoard::restoreSnapshot(const QJsonObject& data) {
    SlidingPuzzleState state,initial;
    if(!state.restore(data["state"].toString().toStdString()) || !initial.restore(data["initial"].toString().toStdString()) || state.size()!=initial.size() || initial.moves()!=0)return false;
    auto reversed=state;while(reversed.moves())reversed.undo();
    if(reversed.tiles()!=initial.tiles())return false;
    m_animation.stop();m_animatedValue=0;m_state=std::move(state);m_initial=std::move(initial);m_preview=false;m_hover=-1;update();emit changed();return true;
}

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
    if(m_paused){
        p.setPen(QColor("#ffd0e2"));p.setFont(QFont("Microsoft YaHei",24,QFont::DemiBold));
        p.drawText(board,Qt::AlignCenter,QString::fromUtf8("已暂停\n\n点击「继续」或按空格\n计时与操作已冻结"));return;
    }
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
    if (m_paused || m_preview || m_state.solved() || m_animation.state() == QAbstractAnimation::Running ||
        !m_state.canMove(index)) return;
    m_from = index;
    m_to = m_state.blank();
    m_animatedValue = m_state.tiles()[index];
    m_state.move(index);
    GameAudio::play(GameAudio::Cue::PuzzleSlide);
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
        "QComboBox { background:#33243e; border:1px solid #735777; border-radius:7px; color:#f9ebed; padding:7px 10px; font:14px 'Microsoft YaHei'; }"
        "QComboBox QAbstractItemView { background:#211a2c; color:#f9ebed; selection-background-color:#74415f; }"
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
    connect(back, &QPushButton::clicked, this, [this](){m_session->saveAuto();emit returnToMenu();});
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
    auto* caption = m_caption = new QLabel(QString::fromUtf8("夕照合影"), panel);
    caption->setStyleSheet("color:#f2cba2; font:700 23px 'Microsoft YaHei';");
    caption->setWordWrap(true);
    controls->addWidget(caption);
    auto* sub = new QLabel(QString::fromUtf8("数字华容道 · 卡面收藏"), panel);
    sub->setStyleSheet("color:#bdacc6; font:13px 'Microsoft YaHei';");
    controls->addWidget(sub);
    m_imageChoice=new QComboBox(panel);m_imageChoice->setObjectName("puzzleImageChoice");m_imageChoice->setIconSize(QSize(42,32));
    m_imageChoice->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_imageChoice->setMinimumContentsLength(12);
    m_imageChoice->setMaxVisibleItems(10);
    const auto addPicture = [this](const QString& name, const QString& path) {
        m_imageChoice->addItem(QIcon(QPixmap(path).scaled(84, 64, Qt::KeepAspectRatio,
            Qt::SmoothTransformation)), name, path);
        m_imageChoice->setItemData(m_imageChoice->count() - 1, name, Qt::ToolTipRole);
    };
    addPicture(QString::fromUtf8("夕照合影 · 爱音 & 素世"),
        QStringLiteral(":/images/backgrounds/anon_soyo_puzzle.jpg"));
    QFile gallery(QStringLiteral(":/data/puzzle_gallery.json"));
    if (gallery.open(QIODevice::ReadOnly)) {
        for (const auto& value : QJsonDocument::fromJson(gallery.readAll()).array()) {
            const auto card = value.toObject();
            const QString variant = card["variant"].toString() == "trained"
                ? QString::fromUtf8("特训") : QString::fromUtf8("初始");
            addPicture(QString::fromUtf8("#%1 %2 · %3").arg(card["id"].toInt())
                .arg(card["character"].toString(), variant), card["path"].toString());
        }
    }
    controls->addWidget(m_imageChoice);
    connect(m_imageChoice,&QComboBox::currentIndexChanged,this,&SlidingPuzzlePage::choosePicture);
    auto* import=new QPushButton(QString::fromUtf8("＋ 导入自己的图片"),panel);import->setObjectName("puzzleImport");controls->addWidget(import);
    connect(import,&QPushButton::clicked,this,[this]{
        bool wasPaused=m_session->paused();m_session->setPaused(true);
        QString path=QFileDialog::getOpenFileName(this,QString::fromUtf8("选择华容道图片"),{},QString::fromUtf8("图片 (*.png *.jpg *.jpeg *.webp *.bmp)"));
        if(!path.isEmpty())importPicture(path);
        if(!wasPaused)m_session->setPaused(false);
    });
    auto* musicCredit = new QLabel(QString::fromUtf8("♪ 背景音乐 · 8-bit"), panel);
    musicCredit->setStyleSheet("color:#d6b594; font:12px 'Microsoft YaHei';");
    controls->addWidget(musicCredit);
    m_musicChoice = new QComboBox(panel);
    m_musicChoice->setObjectName("puzzleMusicChoice");
    m_musicChoice->addItems({QString::fromUtf8("春日影 · CRYCHIC"),
                             QString::fromUtf8("KiLLKiSS · Ave Mujica")});
    m_musicChoice->setCurrentIndex(GameAudio::miniGameMusicTrack());
    m_musicChoice->setToolTip(QString::fromUtf8("选择小游戏背景音乐，切换后立即播放"));
    connect(m_musicChoice, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [](int track) { GameAudio::setMiniGameMusicTrack(track); });
    controls->addWidget(m_musicChoice);
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
            GameAudio::play(GameAudio::Cue::MenuSelect);
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
        GameAudio::play(GameAudio::Cue::MenuSelect);
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
    auto* numbers = m_numbers = new QCheckBox(QString::fromUtf8("显示数字辅助"), panel);
    numbers->setChecked(true);
    connect(numbers, &QCheckBox::toggled, m_board, &SlidingPuzzleBoard::setNumbers);
    controls->addWidget(numbers);
    controls->addStretch();
    auto* scroll=new QScrollArea(this);scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);scroll->setFixedWidth(312);scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);scroll->setStyleSheet("QScrollArea {background:transparent;border:none;}");scroll->setWidget(panel);content->addWidget(scroll);
    root->addLayout(content, 1);
    auto* tip = new QLabel(QString::fromUtf8("点击空格旁的图片滑动  ·  方向键推动相邻图片  ·  按数字从左到右、从上到下复原，右下角留空"), this);
    tip->setStyleSheet("color:#b8a8bf; font:13px 'Microsoft YaHei';");
    tip->setAlignment(Qt::AlignCenter);
    root->addWidget(tip);
    m_session=new MiniGameSession("puzzle",[this]{return snapshot();},[this](const QJsonObject& data){return restoreSnapshot(data);},[this](bool paused){m_board->setPaused(paused);m_undo->setEnabled(!paused && m_board->state().moves()>0);},this);
    root->insertWidget(1,m_session);
    connect(m_board, &SlidingPuzzleBoard::changed, this, &SlidingPuzzlePage::refresh);
    m_clock.setInterval(200);
    connect(&m_clock, &QTimer::timeout, this, [this]() {
        const qint64 seconds = m_session->elapsedMs() / 1000;
        m_time->setText(QStringLiteral("%1:%2").arg(seconds / 60, 2, 10, QLatin1Char('0'))
            .arg(seconds % 60, 2, 10, QLatin1Char('0')));
    });
    if(!m_session->restoreAuto())refresh();
    m_clock.start();
    connect(qApp,&QCoreApplication::aboutToQuit,this,[this]{m_session->saveAuto();});
    m_board->setFocus();
    installEventFilter(this);
    for (QWidget* child : findChildren<QWidget*>())
        child->installEventFilter(this);
}

bool SlidingPuzzlePage::eventFilter(QObject* watched, QEvent* event)
{
    if(watched==this && event->type()==QEvent::WindowDeactivate)m_session->setPaused(true);
    if (qobject_cast<QComboBox*>(watched))
        return QWidget::eventFilter(watched, event);
    if (event->type() == QEvent::KeyPress &&
        (watched == this || isAncestorOf(qobject_cast<QWidget*>(watched)))) {
        auto* key = static_cast<QKeyEvent*>(event);
        if((key->key()==Qt::Key_Space || key->key()==Qt::Key_P || key->key()==Qt::Key_Escape) && key->modifiers()==Qt::NoModifier){if(!key->isAutoRepeat())m_session->togglePause();return true;}
        if(key->modifiers()==Qt::ControlModifier){if(key->key()==Qt::Key_S){m_session->saveSlot();return true;}if(key->key()==Qt::Key_L){m_session->loadSlot();return true;}if(key->key()==Qt::Key_Z){m_board->undo();return true;}}
        if (key->modifiers() == Qt::NoModifier && m_board->moveByArrow(key->key())) {
            key->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SlidingPuzzlePage::resetClock()
{
    if(m_session)m_session->resetProgress();m_finished = false;
    m_time->setText("00:00");
}

void SlidingPuzzlePage::refresh()
{
    const auto& state = m_board->state();
    m_moves->setText(QString::fromUtf8("%1 步").arg(state.moves()));
    m_undo->setEnabled(state.moves() > 0 && !m_session->paused());
    if (state.solved()) {
        if (!m_finished && state.moves() > 0)
            GameAudio::play(GameAudio::Cue::PuzzleSolved);
        m_finished = true;
        m_status->setText(QString::fromUtf8("合影复原了！\n这一刻，终于又在一起。"));
    } else {
        m_finished = false;
        m_status->setText(QString::fromUtf8("每次移动一块相邻图片。\n第一次移动后开始计时。"));
    }
    m_session->changed(state.moves()>0,state.solved());
}

SlidingPuzzlePage::~SlidingPuzzlePage(){if(m_session)m_session->saveAuto();}
void SlidingPuzzlePage::choosePicture(int index){
    const QString path=m_imageChoice->itemData(index).toString();
    QPixmap image=path=="custom"?m_customPicture:QPixmap(path);if(image.isNull())return;
    int side=qMin(image.width(),image.height());
    m_picture=image.copy(int((image.width()-side)*(index==0?.62:.5)),(image.height()-side)/2,side,side).scaled(1600,1600,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    m_pictureKey=path;m_board->setPicture(m_picture);m_caption->setText(m_imageChoice->itemText(index));update();
    if(m_session)m_session->changed(m_board->state().moves()>0,m_board->state().solved());
}
bool SlidingPuzzlePage::importPicture(const QString& path){
    QImageReader reader(path);reader.setAutoTransform(true);const QSize size=reader.size();
    if(size.width()>4096 || size.height()>4096)reader.setScaledSize(size.scaled(4096,4096,Qt::KeepAspectRatio));
    QImage image=reader.read();if(image.isNull()){m_session->notify(QString::fromUtf8("无法读取该图片，原图和进度未改变"));return false;}
    int side=qMin(image.width(),image.height());image=image.copy((image.width()-side)/2,(image.height()-side)/2,side,side).scaled(1600,1600,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    m_picture=QPixmap::fromImage(image);m_customPicture=m_picture;m_pictureKey="custom";
    m_customImageData.clear();QBuffer imageBuffer(&m_customImageData);imageBuffer.open(QIODevice::WriteOnly);m_picture.save(&imageBuffer,"PNG");
    {QSignalBlocker blocker(m_imageChoice);int index=m_imageChoice->findData("custom");if(index<0){m_imageChoice->addItem(QString::fromUtf8("自定义图片"),"custom");index=m_imageChoice->count()-1;}m_imageChoice->setItemIcon(index,QIcon(m_picture.scaled(84,64,Qt::KeepAspectRatio,Qt::SmoothTransformation)));m_imageChoice->setCurrentIndex(index);}
    m_board->setPicture(m_picture);m_caption->setText(QString::fromUtf8("自定义合影"));update();m_session->changed(m_board->state().moves()>0,m_board->state().solved());return true;
}
QJsonObject SlidingPuzzlePage::snapshot() const {
    QJsonObject data=m_board->snapshot();data["picture"]=m_pictureKey;data["numbers"]=m_numbers->isChecked();
    if(m_pictureKey=="custom")data["customImage"]=QString::fromLatin1(m_customImageData.toBase64());
    return data;
}
bool SlidingPuzzlePage::restoreSnapshot(const QJsonObject& data){
    QString key=data["picture"].toString();
    // Removed gallery backgrounds must not reappear when an older save is loaded.
    const QStringList retiredPictures = {
        ":/images/backgrounds/bangdream_gbp_cover.jpg", ":/images/backgrounds/mygo_stage.png",
        ":/images/backgrounds/mujica_theater.png", ":/images/backgrounds/yumemita_dream.png",
        ":/images/backgrounds/tower_hub.png"
    };
    if (retiredPictures.contains(key)) key = QStringLiteral(":/images/backgrounds/anon_soyo_puzzle.jpg");
    int index=m_imageChoice->findData(key);QPixmap image;
    if(key=="custom")image.loadFromData(QByteArray::fromBase64(data["customImage"].toString().toLatin1()),"PNG");
    else if(index>=0)image.load(key);
    if(image.isNull())return false;
    {QSignalBlocker blocker(m_board);if(!m_board->restoreSnapshot(data))return false;}
    int side=qMin(image.width(),image.height());m_picture=image.copy(int((image.width()-side)*(index==0?.62:.5)),(image.height()-side)/2,side,side).scaled(1600,1600,Qt::KeepAspectRatio,Qt::SmoothTransformation);m_pictureKey=key;
    if(key=="custom"){m_customPicture=m_picture;m_customImageData=QByteArray::fromBase64(data["customImage"].toString().toLatin1());}
    {QSignalBlocker blocker(m_imageChoice);if(index<0){m_imageChoice->addItem(QString::fromUtf8("自定义图片"),"custom");index=m_imageChoice->count()-1;}m_imageChoice->setCurrentIndex(index);}
    m_caption->setText(m_imageChoice->itemText(index));m_board->setPicture(m_picture);m_numbers->setChecked(data["numbers"].toBool(true));
    if(auto* choice=findChild<QPushButton*>(QStringLiteral("puzzleDifficulty%1").arg(m_board->state().size())))choice->setChecked(true);
    m_finished=m_board->state().solved();refresh();update();return true;
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
