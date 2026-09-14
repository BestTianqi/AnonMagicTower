#include "MapWidget.h"
#include "Entities/MonsterDB.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QImage>
#include <QColor>
#include <QtMath>
#include <algorithm>

namespace {
constexpr float kPlayerWalkSpeed = 260.0f;

struct MonsterCombatHint {
    int hpLoss = -1;
    int attackDelta = -1;
};

MonsterCombatHint monsterCombatHint(const Player& player, const Monster& monster)
{
    const std::string& name = monster.GetName();
    const bool vampireOrOrc = name.find("吸血") != std::string::npos ||
                              name.find("兽人") != std::string::npos;
    const bool dragon = name.find("魔龙") != std::string::npos ||
                        name.find("龙") != std::string::npos;
    const bool magicAttacker = name.find("法师") != std::string::npos ||
                               name.find("巫师") != std::string::npos ||
                               name.find("大法师") != std::string::npos ||
                               name.find("魔法") != std::string::npos;
    const int attackMultiplier = (player.hasCross && vampireOrOrc) ||
                                 (player.hasDragonSlayer && dragon) ? 2 : 1;
    int incoming = std::max(0, monster.GetATK() - player.def -
                            (player.tempShieldCharges > 0 ? 50 : 0));
    if (player.hasHolyShield && magicAttacker) incoming = 0;
    if (player.hasPenguinDoll &&
        (name.find("高松灯") != std::string::npos || name.find("企鹅") != std::string::npos))
        incoming /= 2;
    if (player.hasMatchaParfait &&
        (name.find("要乐奈") != std::string::npos || name.find("小猫") != std::string::npos))
        incoming /= 2;

    const auto roundsForAttack = [&](int rawAttack) {
        const int dealt = std::max(0, rawAttack * attackMultiplier - monster.GetDEF());
        return dealt > 0 ? (monster.GetHP() + dealt - 1) / dealt : -1;
    };
    const int currentRounds = roundsForAttack(player.atk);
    MonsterCombatHint hint;
    if (currentRounds > 0 && incoming >= 0)
        hint.hpLoss = (currentRounds > 1) ? (currentRounds - 1) * incoming : 0;

    // 找到使击杀回合数减少一回合的最小“额外攻击力”。
    if (currentRounds > 1) {
        for (int delta = 1; delta <= 100000; ++delta) {
            if (roundsForAttack(player.atk + delta) > 0 &&
                roundsForAttack(player.atk + delta) < currentRounds) {
                hint.attackDelta = delta;
                break;
            }
        }
    } else if (currentRounds < 0) {
        for (int delta = 1; delta <= 100000; ++delta) {
            if (roundsForAttack(player.atk + delta) > 0) {
                hint.attackDelta = delta;
                break;
            }
        }
    }
    return hint;
}

void drawMonsterCombatHint(QPainter& painter, const QRect& rect, const Player& player,
                           const Monster& monster)
{
    const MonsterCombatHint hint = monsterCombatHint(player, monster);
    const QRect panel = rect.adjusted(1, 34, -1, -1);
    painter.fillRect(panel, QColor(0, 0, 0, 190));
    QFont font;
    font.setPixelSize(9);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(hint.hpLoss >= 0 ? QColor("#ff9da9") : QColor("#ffce83"));
    painter.drawText(panel.adjusted(1, 0, -1, -10), Qt::AlignCenter,
                    hint.hpLoss >= 0 ? QString::fromUtf8("掉血 %1").arg(hint.hpLoss)
                                     : QString::fromUtf8("无法破防"));
    painter.setPen(QColor("#b9d6ff"));
    const QString threshold = hint.attackDelta > 0
        ? QString::fromUtf8("攻+%1减伤").arg(hint.attackDelta)
        : QString::fromUtf8("已最低伤害");
    painter.drawText(panel.adjusted(1, 10, -1, 0), Qt::AlignCenter, threshold);
}
}

MapWidget::MapWidget(Game* game, QWidget* parent)
    : QWidget(parent), m_game(game)
{
    setFixedSize(900, 900);
    generatePlaceholders();
    m_motionTimer.setInterval(16);
    m_motionClock.start();
    m_monsterMotionClock.start();
    connect(&m_motionTimer, &QTimer::timeout, this, &MapWidget::advancePlayerMotion);
    m_motionTimer.start();
}

static QPixmap makePixmap(const QColor& fill, const QColor& border,
                          const QString& text, const QColor& textColor = Qt::white,
                          int fontSize = 12)
{
    QPixmap px(TILE_SIZE, TILE_SIZE);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);
    QRect r(0, 0, TILE_SIZE, TILE_SIZE);
    QRect inner = r.adjusted(2, 2, -2, -2);
    p.setBrush(fill);
    p.setPen(QPen(border, 2));
    p.drawRoundedRect(inner, 4, 4);
    if (!text.isEmpty()) {
        QFont f;
        f.setPixelSize(fontSize);
        f.setBold(true);
        p.setFont(f);
        p.setPen(textColor);
        p.drawText(r, Qt::AlignCenter, text);
    }
    p.end();
    return px;
}

void MapWidget::generatePlaceholders()
{
    // 地板
    m_tilePix[Tile_Floor] = makePixmap(QColor(180, 170, 150), QColor(150, 140, 120), "");
    // 墙壁
    m_tilePix[Tile_Wall] = makePixmap(QColor(55, 55, 60), QColor(40, 40, 45), "");
    // 暗墙（无眼镜时和墙一样）
    m_tilePix[Tile_DarkWall] = makePixmap(QColor(55, 55, 60), QColor(40, 40, 45), "");
    // 暗墙（有眼镜时变浅）
    m_darkWallRevealed = makePixmap(QColor(100, 95, 85), QColor(75, 70, 60), "暗", QColor(180, 180, 160), 10);
    // 上楼
    m_tilePix[Tile_StairsUp] = makePixmap(QColor(110, 95, 25), QColor(70, 58, 12),
        QString::fromUtf8("↑"), Qt::black, 20);
    // 下楼
    m_tilePix[Tile_StairsDown] = makePixmap(QColor(95, 55, 125), QColor(55, 30, 80),
        QString::fromUtf8("↓"), Qt::white, 20);
    // 道具
    m_tilePix[Tile_Item] = makePixmap(QColor(60, 170, 60), QColor(40, 130, 40),
        QString::fromUtf8("✦"), QColor(255, 255, 100), 16);
    // 红门
    m_tilePix[Tile_DoorRed] = makePixmap(QColor(180, 60, 50), QColor(140, 30, 20),
        QString::fromUtf8("红门"), Qt::white, 10);
    // 蓝门
    m_tilePix[Tile_DoorBlue] = makePixmap(QColor(50, 70, 180), QColor(30, 40, 140),
        QString::fromUtf8("蓝门"), Qt::white, 10);
    // 原版黄门（沿用 Tile_DoorGreen 的存档数值以兼容旧存档）
    m_tilePix[Tile_DoorGreen] = makePixmap(QColor(220, 180, 40), QColor(150, 105, 20),
        QString::fromUtf8("黄门"), QColor(60, 35, 0), 10);
    m_tilePix[Tile_DoorMagic] = makePixmap(QColor(130, 60, 175), QColor(75, 30, 120),
        QString::fromUtf8("魔法门"), Qt::white, 9);
    m_tilePix[Tile_DoorIron] = makePixmap(QColor(100, 105, 115), QColor(45, 50, 60),
        QString::fromUtf8("铁门"), Qt::white, 10);
    m_tilePix[Tile_Lava] = makePixmap(QColor(220, 70, 20), QColor(130, 25, 10),
        QString::fromUtf8("岩浆"), QColor(255, 230, 80), 9);
    m_tilePix[Tile_StarRiver] = makePixmap(QColor(35, 40, 110), QColor(90, 100, 210),
        QString::fromUtf8("星河"), Qt::white, 9);
    // NPC
    m_tilePix[Tile_NPC] = makePixmap(QColor(200, 160, 60), QColor(160, 120, 30),
        QString::fromUtf8("NPC"), Qt::white, 10);
    // 商店
    m_tilePix[Tile_Shop] = makePixmap(QColor(240, 200, 20), QColor(200, 160, 10),
        QString::fromUtf8("商店"), QColor(80, 40, 0), 10);

    // 怪物（按名称生成占位图，显示名字+数值）
    for (auto& m : MonsterDB::all()) {
        QPixmap px(TILE_SIZE, TILE_SIZE);
        px.fill(Qt::transparent);
        {
            QPainter p(&px);
            p.setRenderHint(QPainter::Antialiasing);
            QRect inner(2, 2, TILE_SIZE - 4, TILE_SIZE - 4);
            p.setBrush(QColor(200, 80, 80));
            p.setPen(QPen(QColor(160, 50, 50), 2));
            p.drawRoundedRect(inner, 4, 4);

            QFont f;
            // 名字
            f.setPixelSize(12);
            f.setBold(true);
            p.setFont(f);
            p.setPen(Qt::white);
            p.drawText(QRect(0, 3, TILE_SIZE, 18), Qt::AlignHCenter | Qt::AlignTop,
                QString::fromStdString(m.GetName()));

            // 数值
            f.setPixelSize(9);
            f.setBold(false);
            p.setFont(f);
            p.setPen(QColor(240, 240, 200));
            p.drawText(QRect(2, 24, TILE_SIZE - 4, 16), Qt::AlignHCenter | Qt::AlignTop,
                QString("HP:%1 ATK:%2").arg(m.GetHP()).arg(m.GetATK()));
            p.drawText(QRect(2, 38, TILE_SIZE - 4, 16), Qt::AlignHCenter | Qt::AlignTop,
                QString("DEF:%1 G:%2").arg(m.GetDEF()).arg(m.GetGold()));
        }
        px.detach();
        m_monsterPix[m.GetName()] = px;
    }
    // 默认怪物（用于未匹配的怪物）
    m_defaultMonsterPix = makePixmap(QColor(200, 80, 80), QColor(160, 50, 50),
        QString::fromUtf8("怪"), Qt::white, 14);

    // 默认道具
    m_defaultItemPix = m_tilePix[Tile_Item];

    // 玩家
    m_playerPix = makePixmap(QColor(60, 130, 240), QColor(30, 80, 180),
        QString::fromUtf8("勇"), Qt::white, 18);
}

void MapWidget::loadTileImage(int tileType, const QString& path)
{
    QPixmap px(path);
    if (!px.isNull()) {
        if (tileType == Tile_StairsUp || tileType == Tile_StairsDown) {
            QImage image = px.toImage().convertToFormat(QImage::Format_ARGB32);
            for (int y = 0; y < image.height(); ++y) {
                for (int x = 0; x < image.width(); ++x) {
                    const QColor color = QColor::fromRgba(image.pixel(x, y));
                    image.setPixelColor(x, y, QColor(color.red() * 0.68,
                                                     color.green() * 0.68,
                                                     color.blue() * 0.68,
                                                     color.alpha()));
                }
            }
            px = QPixmap::fromImage(image);
        }
        // Pixel-art assets must remain crisp; nearest-neighbor scaling also avoids
        // filtering work on every custom tile during startup.
        m_tilePix[tileType] = px.scaled(TILE_SIZE, TILE_SIZE, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        m_hasTileImage.insert(tileType);
    }
}

void MapWidget::loadMonsterImage(const std::string& name, const QString& path)
{
    QPixmap px(path);
    if (!px.isNull()) {
        m_monsterPix[name] = px.scaled(TILE_SIZE, TILE_SIZE, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        m_hasMonsterImage.insert(name);
    }
}

void MapWidget::loadPlayerImage(const QString& path)
{
    QPixmap px(path);
    if (!px.isNull()) {
        m_playerPix = px.scaled(TILE_SIZE, TILE_SIZE, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        m_hasPlayerImage = true;
    }
}

void MapWidget::loadPlayerSpriteSheet(const QString& path)
{
    const QPixmap sheet(path);
    if (sheet.isNull() || sheet.width() < TILE_SIZE * 4 || sheet.height() < TILE_SIZE * 4)
        return;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col)
            m_playerFrames[row * 4 + col] = sheet.copy(col * TILE_SIZE, row * TILE_SIZE,
                                                        TILE_SIZE, TILE_SIZE);
    }
    m_hasPlayerSheet = true;
    update();
}

void MapWidget::setPlayerDirection(int dx, int dy)
{
    if (dx < 0) m_playerDirectionRow = 1;
    else if (dx > 0) m_playerDirectionRow = 2;
    else if (dy < 0) m_playerDirectionRow = 3;
    else if (dy > 0) m_playerDirectionRow = 0;
}

void MapWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_game) {
        // position() 已经是 MapWidget 的本地坐标；按实际绘制区域换算，
        // 避免窗口缩放、DPI 或布局边距导致整列/整行偏移。
        const QPointF local = event->position();
        const int mapWidth = std::max(1, width());
        const int mapHeight = std::max(1, height());
        if (local.x() >= 0.0 && local.y() >= 0.0 &&
            local.x() < mapWidth && local.y() < mapHeight) {
            const int tileX = std::clamp(static_cast<int>(local.x() * m_game->width() / mapWidth),
                                         0, m_game->width() - 1);
            const int tileY = std::clamp(static_cast<int>(local.y() * m_game->height() / mapHeight),
                                         0, m_game->height() - 1);
            emit tileClicked(tileX, tileY);
        }
    }
    QWidget::mousePressEvent(event);
}

void MapWidget::syncPlayerMotionTarget()
{
    if (!m_game) return;
    const int tileX = m_game->player().x;
    const int tileY = m_game->player().y;
    if (!m_motionInitialized) {
        m_playerMotion.snapTo(tileX, tileY);
        m_lastPlayerTileX = tileX;
        m_lastPlayerTileY = tileY;
        m_motionInitialized = true;
    } else if (!m_movementAnimationEnabled) {
        m_playerMotion.snapTo(tileX, tileY);
        m_playerFrame = 1;
        m_lastPlayerTileX = tileX;
        m_lastPlayerTileY = tileY;
    } else if (tileX != m_lastPlayerTileX || tileY != m_lastPlayerTileY) {
        const bool animated = m_playerMotion.beginGridStep(m_lastPlayerTileX, m_lastPlayerTileY,
                                                           tileX, tileY, kPlayerWalkSpeed);
        if (!animated) m_playerFrame = 1;
        m_lastPlayerTileX = tileX;
        m_lastPlayerTileY = tileY;
    }
}

void MapWidget::snapPlayerToGame()
{
    m_motionInitialized = false;
    m_monsterMotions.clear();
    m_monsterMotionActive = false;
    syncPlayerMotionTarget();
    update();
}

void MapWidget::advancePlayerMotion()
{
    const qint64 elapsedMs = std::clamp<qint64>(m_motionClock.restart(), 1, 50);
    syncPlayerMotionTarget();
    advanceMonsterMotion();
    if (!m_motionInitialized) return;
    const bool wasMoving = m_playerMotion.isMoving();
    if (wasMoving) {
        m_playerMotion.advance(static_cast<float>(elapsedMs));
        m_playerFrame = m_playerMotion.walkingFrame(4);
        const bool finished = !m_playerMotion.isMoving();
        if (finished) {
            m_playerFrame = 1;
            emit playerMotionFinished();
        }
        update();
    }
}

void MapWidget::playMonsterMovement(const std::vector<Game::MonsterMovementAnimation>& movements)
{
    m_monsterMotions.clear();
    for (const auto& movement : movements) {
        m_monsterMotions.push_back({
            movement.monster.GetName(),
            QPointF(movement.fromX, movement.fromY),
            QPointF(movement.toX, movement.toY)});
    }
    if (m_monsterMotions.empty()) {
        m_monsterMotionActive = false;
        return;
    }
    m_monsterMotionClock.restart();
    m_monsterMotionActive = true;
    update();
}

void MapWidget::advanceMonsterMotion()
{
    if (!m_monsterMotionActive) return;
    if (m_monsterMotionClock.elapsed() >= 320) {
        m_monsterMotions.clear();
        m_monsterMotionActive = false;
    }
    update();
}

void MapWidget::loadItemImage(const std::string& name, const QString& path)
{
    QPixmap px(path);
    if (!px.isNull())
        m_itemPix[name] = px.scaled(TILE_SIZE, TILE_SIZE, Qt::IgnoreAspectRatio,
                                    Qt::FastTransformation);
}

void MapWidget::loadNPCImage(const std::string& name, const QString& path)
{
    QPixmap px(path);
    if (!px.isNull())
        m_npcPix[name] = px.scaled(TILE_SIZE, TILE_SIZE, Qt::IgnoreAspectRatio,
                                   Qt::FastTransformation);
}

void MapWidget::loadDarkWallRevealedImage(const QString& path)
{
    QPixmap px(path);
    if (!px.isNull())
        m_darkWallRevealed = px.scaled(TILE_SIZE, TILE_SIZE, Qt::IgnoreAspectRatio,
                                       Qt::FastTransformation);
}

void MapWidget::loadBackgroundImage(const QString& path)
{
    QPixmap px(path);
    if (!px.isNull())
        m_backgroundPix = px;
}

QSize MapWidget::sizeHint() const
{
    return QSize(900, 900);
}

// 判断是否为钥匙类道具
static bool isKeyItem(const std::string& name)
{
    return name == "Red Key" || name == "红钥匙" ||
           name == "Blue Key" || name == "蓝钥匙" ||
           name == "Green Key" || name == "绿钥匙" ||
           name == "Yellow Key" || name == "黄钥匙" ||
           name == "万能钥匙";
}

// 绘制钥匙形状（竖直：上方圆形把手，下方杆+齿）
static void drawKeyShape(QPainter& p, const QRect& r, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);

    const int cx = r.center().x();
    const int top = r.top() + 4;
    const int bowCy = top + 12;      // 把手圆心 Y
    const int shaftTop = bowCy + 1;   // 杆顶部
    const int shaftBottom = r.bottom() - 6;

    // 钥匙外轮廓
    QPainterPath keyPath;
    // 圆形把手
    keyPath.addEllipse(QPointF(cx, bowCy), 11, 11);
    // 杆
    keyPath.addRect(QRectF(cx - 4, shaftTop, 8, shaftBottom - shaftTop));
    // 齿（向右伸出）
    keyPath.addRect(QRectF(cx + 3, shaftTop + 6, 8, 5));
    keyPath.addRect(QRectF(cx + 3, shaftBottom - 20, 6, 4));

    // 把手内孔
    QPainterPath hole;
    hole.addEllipse(QPointF(cx, bowCy), 5, 5);
    keyPath = keyPath.subtracted(hole);

    // 填充和描边
    p.setBrush(color.lighter(120));
    p.setPen(QPen(color.darker(130), 2));
    p.drawPath(keyPath);

    p.restore();
}

// 绘制金币（带内圆的硬币）
static void drawCoinShape(QPainter& p, const QRect& r, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    int cx = r.center().x(), cy = r.center().y();
    int outerR = 18;
    p.setBrush(color.lighter(130));
    p.setPen(QPen(color.darker(140), 2));
    p.drawEllipse(QPoint(cx, cy), outerR, outerR);
    // 内圈
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(color.darker(120), 2));
    p.drawEllipse(QPoint(cx, cy), outerR - 5, outerR - 5);
    // 中心 "G" 符号
    QFont f;
    f.setPixelSize(14);
    f.setBold(true);
    p.setFont(f);
    p.setPen(color.darker(180));
    p.drawText(QRect(cx - outerR, cy - outerR, outerR * 2, outerR * 2),
        Qt::AlignCenter, "G");
    p.restore();
}

// 绘制武器（竖直剑：剑身+护手+剑柄+剑首）
static void drawSwordShape(QPainter& p, const QRect& r, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    int cx = r.center().x(), cy = r.center().y();
    QPainterPath sword;
    // 剑身（上宽下窄的菱形/三角形）
    sword.moveTo(cx, cy - 20);           // 剑尖
    sword.lineTo(cx + 5, cy - 4);        // 右下
    sword.lineTo(cx + 3, cy - 4);        // 护手右
    sword.lineTo(cx + 12, cy);           // 护手右端
    sword.lineTo(cx + 3, cy + 2);        // 护手右下
    sword.lineTo(cx + 3, cy + 14);       // 剑柄右
    sword.lineTo(cx + 5, cy + 19);       // 剑首右
    sword.lineTo(cx - 5, cy + 19);       // 剑首左
    sword.lineTo(cx - 3, cy + 14);       // 剑柄左
    sword.lineTo(cx - 3, cy + 2);        // 护手左下
    sword.lineTo(cx - 12, cy);           // 护手左端
    sword.lineTo(cx - 3, cy - 4);        // 护手左上
    sword.lineTo(cx - 5, cy - 4);        // 左下
    sword.closeSubpath();
    p.setBrush(color.lighter(120));
    p.setPen(QPen(color.darker(140), 2));
    p.drawPath(sword);
    p.restore();
}

// 绘制防具（盾牌轮廓）
static void drawShieldShape(QPainter& p, const QRect& r, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    int cx = r.center().x(), cy = r.center().y();
    QPainterPath shield;
    // 盾牌：上平下尖
    shield.moveTo(cx - 14, cy - 18);     // 左上
    shield.lineTo(cx + 14, cy - 18);     // 右上
    shield.lineTo(cx + 14, cy + 2);      // 右侧
    shield.quadTo(cx + 10, cy + 12, cx, cy + 20);  // 右下弧到尖端
    shield.quadTo(cx - 10, cy + 12, cx - 14, cy + 2); // 尖端到左下弧
    shield.closeSubpath();
    p.setBrush(color.lighter(120));
    p.setPen(QPen(color.darker(140), 2));
    p.drawPath(shield);
    // 盾面十字装饰
    p.setPen(QPen(color.darker(110), 1.5));
    p.drawLine(cx, cy - 14, cx, cy + 14);
    p.drawLine(cx - 10, cy - 6, cx + 10, cy - 6);
    p.restore();
}

// 判断是否为有特殊形状的道具
// 绘制生命药（药水瓶：圆底+细颈+瓶口）
static void drawPotionShape(QPainter& p, const QRect& r, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    int cx = r.center().x(), cy = r.center().y();
    QPainterPath bottle;
    // 瓶口
    bottle.addRect(QRectF(cx - 5, cy - 18, 10, 6));
    // 瓶颈
    bottle.addRect(QRectF(cx - 2, cy - 12, 4, 6));
    // 瓶身（圆角矩形）
    bottle.addRoundedRect(QRectF(cx - 10, cy - 6, 20, 22), 6, 6);
    p.setBrush(color.lighter(120));
    p.setPen(QPen(color.darker(140), 2));
    p.drawPath(bottle);
    // 高光
    p.setPen(QPen(color.lighter(180), 1.5));
    p.drawLine(cx - 6, cy + 2, cx - 6, cy + 10);
    p.restore();
}

// 绘制眼镜（两个圆+鼻梁）
static void drawGlassesShape(QPainter& p, const QRect& r, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    int cx = r.center().x(), cy = r.center().y();
    // 左镜片
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(color.darker(130), 3));
    p.drawEllipse(QPoint(cx - 8, cy + 2), 9, 8);
    // 右镜片
    p.drawEllipse(QPoint(cx + 8, cy + 2), 9, 8);
    // 鼻梁
    p.drawLine(cx - 1, cy + 2, cx + 1, cy + 2);
    // 镜腿
    p.drawLine(cx - 17, cy, cx - 8, cy + 2);
    p.drawLine(cx + 17, cy, cx + 8, cy + 2);
    p.restore();
}

// 绘制破墙锤（锤头+手柄）
static void drawHammerShape(QPainter& p, const QRect& r, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    int cx = r.center().x(), cy = r.center().y();
    // 手柄
    p.setBrush(QColor(120, 90, 60));
    p.setPen(QPen(QColor(90, 60, 30), 2));
    p.drawRoundedRect(QRectF(cx - 3, cy + 2, 6, 22), 2, 2);
    // 锤头
    p.setBrush(color.lighter(110));
    p.setPen(QPen(color.darker(140), 2));
    p.drawRoundedRect(QRectF(cx - 14, cy - 16, 28, 18), 4, 4);
    p.restore();
}

// 绘制上楼器（圆形+上箭头）
static void drawUpArrowShape(QPainter& p, const QRect& r, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    int cx = r.center().x(), cy = r.center().y();
    // 圆底
    p.setBrush(color.lighter(120));
    p.setPen(QPen(color.darker(140), 2));
    p.drawEllipse(QPoint(cx, cy), 16, 16);
    // 上箭头
    QPainterPath arrow;
    arrow.moveTo(cx, cy - 10);
    arrow.lineTo(cx + 7, cy + 2);
    arrow.lineTo(cx + 2, cy + 2);
    arrow.lineTo(cx + 2, cy + 8);
    arrow.lineTo(cx - 2, cy + 8);
    arrow.lineTo(cx - 2, cy + 2);
    arrow.lineTo(cx - 7, cy + 2);
    arrow.closeSubpath();
    p.setBrush(color.darker(150));
    p.setPen(Qt::NoPen);
    p.drawPath(arrow);
    p.restore();
}

// 绘制下楼器（圆形+下箭头）
static void drawDownArrowShape(QPainter& p, const QRect& r, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    int cx = r.center().x(), cy = r.center().y();
    p.setBrush(color.lighter(120));
    p.setPen(QPen(color.darker(140), 2));
    p.drawEllipse(QPoint(cx, cy), 16, 16);
    QPainterPath arrow;
    arrow.moveTo(cx, cy + 10);
    arrow.lineTo(cx + 7, cy - 2);
    arrow.lineTo(cx + 2, cy - 2);
    arrow.lineTo(cx + 2, cy - 8);
    arrow.lineTo(cx - 2, cy - 8);
    arrow.lineTo(cx - 2, cy - 2);
    arrow.lineTo(cx - 7, cy - 2);
    arrow.closeSubpath();
    p.setBrush(color.darker(150));
    p.setPen(Qt::NoPen);
    p.drawPath(arrow);
    p.restore();
}

// 绘制临时护盾（盾形+T字）
static void drawTempShieldShape(QPainter& p, const QRect& r, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    int cx = r.center().x(), cy = r.center().y();
    QPainterPath shield;
    shield.moveTo(cx, cy - 18);
    shield.quadTo(cx + 16, cy - 10, cx + 14, cy + 2);
    shield.quadTo(cx, cy + 6, cx, cy + 18);
    shield.quadTo(cx, cy + 6, cx - 14, cy + 2);
    shield.quadTo(cx - 16, cy - 10, cx, cy - 18);
    shield.closeSubpath();
    p.setBrush(color.lighter(120));
    p.setPen(QPen(color.darker(140), 2));
    p.drawPath(shield);
    // T 字
    p.setPen(QPen(color.darker(160), 2.5));
    p.drawLine(cx, cy - 8, cx, cy + 10);
    p.drawLine(cx - 8, cy - 6, cx + 8, cy - 6);
    p.restore();
}

// 绘制企鹅玩偶（简笔企鹅）
static void drawPenguinShape(QPainter& p, const QRect& r, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    int cx = r.center().x(), cy = r.center().y();
    // 身体（椭圆）
    p.setBrush(QColor(40, 40, 55));
    p.setPen(QPen(QColor(20, 20, 35), 2));
    p.drawEllipse(QPoint(cx, cy + 4), 12, 16);
    // 白肚皮
    p.setBrush(QColor(230, 230, 240));
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPoint(cx, cy + 6), 7, 10);
    // 头
    p.setBrush(QColor(40, 40, 55));
    p.setPen(QPen(QColor(20, 20, 35), 2));
    p.drawEllipse(QPoint(cx, cy - 10), 9, 9);
    // 眼睛
    p.setBrush(Qt::white);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(cx - 3., cy - 10.), 2.5, 3.);
    p.drawEllipse(QPointF(cx + 3., cy - 10.), 2.5, 3.);
    p.setBrush(QColor(20, 20, 20));
    p.drawEllipse(QPointF(cx - 3., cy - 10.), 1., 1.5);
    p.drawEllipse(QPointF(cx + 3., cy - 10.), 1., 1.5);
    // 喙
    p.setBrush(QColor(240, 150, 30));
    p.setPen(Qt::NoPen);
    QPainterPath beak;
    beak.moveTo(cx - 3, cy - 7);
    beak.lineTo(cx, cy - 4);
    beak.lineTo(cx + 3, cy - 7);
    beak.closeSubpath();
    p.drawPath(beak);
    p.restore();
}

// 绘制抹茶芭菲（高脚杯+分层）
static void drawParfaitShape(QPainter& p, const QRect& r, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    int cx = r.center().x(), cy = r.center().y();
    // 杯身（倒三角）
    QPainterPath glass;
    glass.moveTo(cx - 12, cy - 14);
    glass.lineTo(cx + 12, cy - 14);
    glass.lineTo(cx + 4, cy + 4);
    glass.lineTo(cx - 4, cy + 4);
    glass.closeSubpath();
    p.setBrush(color.lighter(150));
    p.setPen(QPen(color.darker(120), 2));
    p.drawPath(glass);
    // 杯脚
    p.drawRect(QRectF(cx - 1, cy + 4, 2, 8));
    // 底座
    p.drawRoundedRect(QRectF(cx - 8, cy + 12, 16, 4), 2, 2);
    // 分层线
    p.setPen(QPen(color.darker(100), 1));
    p.drawLine(cx - 10, cy - 6, cx + 10, cy - 6);
    p.drawLine(cx - 7, cy + 0, cx + 7, cy + 0);
    // 顶部奶油/樱桃
    p.setBrush(QColor(255, 220, 220));
    p.setPen(QPen(QColor(200, 100, 100), 1));
    p.drawEllipse(QPoint(cx, cy - 14), 5, 4);
    p.restore();
}

// 绘制幸运金币（带四叶草的金币）
static void drawLuckyCoinShape(QPainter& p, const QRect& r, const QColor& color)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    int cx = r.center().x(), cy = r.center().y();
    int outerR = 18;
    // 外圈
    p.setBrush(color.lighter(130));
    p.setPen(QPen(color.darker(140), 2));
    p.drawEllipse(QPoint(cx, cy), outerR, outerR);
    // 内圈
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(color.lighter(160), 1.5));
    p.drawEllipse(QPoint(cx, cy), outerR - 4, outerR - 4);
    // 四叶草符号
    p.setPen(Qt::NoPen);
    p.setBrush(color.darker(150));
    int d = 5;
    for (int i = 0; i < 4; ++i) {
        double angle = i * 3.14159 / 2;
        int ox = cx + (int)(d * cos(angle));
        int oy = cy - (int)(d * sin(angle));
        p.drawEllipse(QPoint(ox, oy), 3, 3);
    }
    p.restore();
}

static bool hasCustomShape(const std::string& name)
{
    return isKeyItem(name) ||
           name == "Treasure" || name == "金币" ||
           name == "Weapon" || name == "武器" ||
           name == "Armor" || name == "防具" ||
           name == "Potion" || name == "生命药" ||
           name == "小血瓶" || name == "大血瓶" ||
           name == "红宝石" || name == "蓝宝石" ||
           name == "铁剑" || name == "银剑" || name == "骑士剑" || name == "圣剑" || name == "神圣剑" ||
           name == "铁盾" || name == "银盾" || name == "骑士盾" || name == "圣盾" || name == "神圣盾" ||
           name == "圣水" || name == "镐" || name == "炸弹" || name == "地震卷轴" ||
           name == "十字架" || name == "屠龙匕" || name == "冰冻魔法" || name == "飞行魔杖" ||
           name == "对称飞行器" || name == "记事本" ||
           name == "MyGO应援红章" || name == "Mujica应援蓝章" ||
           name == "爱音拨片" || name == "立希鼓棒" || name == "乐奈猫爪" || name == "灯的麦克风" || name == "睦的贝斯" ||
           name == "素世谱架" || name == "海铃节拍器" || name == "初华舞台耳返" || name == "祥子黑色乐谱" || name == "Mujica终幕面具" ||
           name == "立希水壶" || name == "灯的热牛奶" || name == "爱音能量饮" || name == "红色Live票" || name == "蓝色Live票" || name == "黄色Live票" ||
           name == "爱音自拍眼镜" || name == "睦的镐子" || name == "Mujica烟雾弹" || name == "Mujica舞台震响卷" || name == "MyGO和解徽章" ||
           name == "祥子指挥棒" || name == "海铃冷静指令" || name == "爱音手机" || name == "楼层传送器" || name == "Mujica镜面舞台票" || name == "灯的歌词本" ||
           name == "后台万能通行证" || name == "舞台升降卡" || name == "撤场通行卡" || name == "乐队护盾贴" || name == "立希企鹅挂件" ||
           name == "乐奈抹茶芭菲" || name == "乐奈幸运硬币" ||
           name == "匿名眼镜" ||
           name == "破墙锤" ||
           name == "上楼器" ||
           name == "下楼器" ||
           name == "临时护盾" ||
           name == "企鹅玩偶" ||
           name == "抹茶芭菲" ||
           name == "幸运金币";
}

// 根据道具名称返回对应颜色、标签和数值描述
static void itemAppearance(const std::string& name, int value, QColor& fill, QColor& border,
                           QString& label, QString& desc, QColor& textColor)
{
    QString qname = QString::fromStdString(name);

    // 钥匙类
    if (qname == QString::fromUtf8("Red Key") || qname == QString::fromUtf8("红钥匙") || qname == QString::fromUtf8("红色Live票"))
        { fill = QColor(200, 45, 45); border = QColor(160, 20, 20);
          label = QString::fromUtf8("红钥"); textColor = QColor(255, 220, 100); return; }
    if (qname == QString::fromUtf8("Blue Key") || qname == QString::fromUtf8("蓝钥匙") || qname == QString::fromUtf8("蓝色Live票"))
        { fill = QColor(45, 60, 200); border = QColor(20, 30, 160);
          label = QString::fromUtf8("蓝钥"); textColor = QColor(255, 220, 100); return; }
    if (qname == QString::fromUtf8("Green Key") || qname == QString::fromUtf8("绿钥匙") ||
        qname == QString::fromUtf8("Yellow Key") || qname == QString::fromUtf8("黄钥匙") || qname == QString::fromUtf8("黄色Live票"))
        { fill = QColor(225, 185, 40); border = QColor(155, 110, 20);
          label = QString::fromUtf8("黄钥"); textColor = QColor(70, 35, 0); return; }
    if (qname == QString::fromUtf8("万能钥匙") || qname == QString::fromUtf8("后台万能通行证"))
        { fill = QColor(130, 60, 200); border = QColor(90, 30, 160);
          label = QString::fromUtf8("万能钥"); textColor = QColor(255, 220, 100); return; }

    // 属性类
    if (qname == QString::fromUtf8("Potion") || qname == QString::fromUtf8("生命药") ||
        qname == QString::fromUtf8("小血瓶") || qname == QString::fromUtf8("大血瓶") ||
        qname == QString::fromUtf8("现场补给") || qname == QString::fromUtf8("灯的热牛奶") || qname == QString::fromUtf8("爱音能量饮"))
        { fill = QColor(200, 60, 60); border = QColor(150, 30, 30);
          label = (qname == QString::fromUtf8("现场补给")) ? QString::fromUtf8("补给") : qname; textColor = Qt::white;
          desc = QString("+%1HP").arg(value); return; }
    if (qname == QString::fromUtf8("红宝石") || qname == QString::fromUtf8("Ruby Gem") || qname == QString::fromUtf8("MyGO应援红章"))
        { fill = QColor(220, 40, 55); border = QColor(125, 15, 25);
          label = (qname == QString::fromUtf8("MyGO应援红章")) ? QString::fromUtf8("红章") : QString::fromUtf8("红宝石"); textColor = Qt::white;
          desc = QString("ATK+%1").arg(value); return; }
    if (qname == QString::fromUtf8("蓝宝石") || qname == QString::fromUtf8("Sapphire Gem") || qname == QString::fromUtf8("Mujica应援蓝章"))
        { fill = QColor(50, 100, 220); border = QColor(20, 50, 145);
          label = (qname == QString::fromUtf8("Mujica应援蓝章")) ? QString::fromUtf8("蓝章") : QString::fromUtf8("蓝宝石"); textColor = Qt::white;
          desc = QString("DEF+%1").arg(value); return; }
    if (qname == QString::fromUtf8("Weapon") || qname == QString::fromUtf8("武器"))
        { fill = QColor(210, 140, 40); border = QColor(160, 100, 20);
          label = QString::fromUtf8("武器"); textColor = Qt::white;
          desc = QString("ATK+%1").arg(value); return; }
    if (qname == QString::fromUtf8("Armor") || qname == QString::fromUtf8("防具"))
        { fill = QColor(60, 120, 200); border = QColor(30, 80, 160);
          label = QString::fromUtf8("防具"); textColor = Qt::white;
          desc = QString("DEF+%1").arg(value); return; }
    if (qname == QString::fromUtf8("Treasure") || qname == QString::fromUtf8("金币"))
        { fill = QColor(220, 180, 40); border = QColor(170, 130, 20);
          label = QString::fromUtf8("金币"); textColor = QColor(100, 60, 0);
          desc = QString("%1G").arg(value); return; }
    if (qname == QString::fromUtf8("爱音拨片") || qname == QString::fromUtf8("立希鼓棒") ||
        qname == QString::fromUtf8("乐奈猫爪") || qname == QString::fromUtf8("灯的麦克风") || qname == QString::fromUtf8("睦的贝斯") ||
        qname == QString::fromUtf8("铁剑") || qname == QString::fromUtf8("银剑") ||
        qname == QString::fromUtf8("骑士剑") || qname == QString::fromUtf8("圣剑") ||
        qname == QString::fromUtf8("神圣剑"))
        { fill = QColor(210, 140, 40); border = QColor(160, 100, 20);
          label = qname; textColor = Qt::white; desc = QString("ATK+%1").arg(value); return; }
    if (qname == QString::fromUtf8("素世谱架") || qname == QString::fromUtf8("海铃节拍器") || qname == QString::fromUtf8("初华舞台耳返") ||
        qname == QString::fromUtf8("祥子黑色乐谱") || qname == QString::fromUtf8("Mujica终幕面具") ||
        qname == QString::fromUtf8("铁盾") || qname == QString::fromUtf8("银盾") ||
        qname == QString::fromUtf8("骑士盾") || qname == QString::fromUtf8("圣盾") ||
        qname == QString::fromUtf8("神圣盾"))
        { fill = QColor(60, 120, 200); border = QColor(30, 80, 160);
          label = qname; textColor = Qt::white; desc = QString("DEF+%1").arg(value); return; }
    if (qname == QString::fromUtf8("立希水壶") || qname == QString::fromUtf8("圣水"))
        { fill = QColor(80, 180, 240); border = QColor(30, 120, 190);
          label = qname; textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("睦的镐子") || qname == QString::fromUtf8("Mujica烟雾弹") || qname == QString::fromUtf8("Mujica舞台震响卷") ||
        qname == QString::fromUtf8("镐") || qname == QString::fromUtf8("炸弹") ||
        qname == QString::fromUtf8("地震卷轴"))
        { fill = QColor(140, 100, 70); border = QColor(100, 70, 40);
          label = qname; textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("MyGO和解徽章") || qname == QString::fromUtf8("祥子指挥棒") ||
        qname == QString::fromUtf8("海铃冷静指令") || qname == QString::fromUtf8("爱音手机") || qname == QString::fromUtf8("楼层传送器") ||
        qname == QString::fromUtf8("Mujica镜面舞台票") || qname == QString::fromUtf8("灯的歌词本") ||
        qname == QString::fromUtf8("十字架") || qname == QString::fromUtf8("屠龙匕") ||
        qname == QString::fromUtf8("冰冻魔法") || qname == QString::fromUtf8("飞行魔杖") ||
        qname == QString::fromUtf8("对称飞行器") || qname == QString::fromUtf8("记事本"))
        { fill = QColor(150, 90, 190); border = QColor(100, 50, 140);
          label = qname; textColor = Qt::white; return; }

    // 特殊道具
    if (qname == QString::fromUtf8("匿名眼镜") || qname == QString::fromUtf8("爱音自拍眼镜"))
        { fill = QColor(40, 180, 180); border = QColor(20, 130, 130);
          label = QString::fromUtf8("眼镜"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("破墙锤"))
        { fill = QColor(140, 100, 70); border = QColor(100, 70, 40);
          label = QString::fromUtf8("破墙锤"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("上楼器") || qname == QString::fromUtf8("舞台升降卡"))
        { fill = QColor(180, 170, 60); border = QColor(140, 130, 30);
          label = QString::fromUtf8("上楼器"); textColor = Qt::black; return; }
    if (qname == QString::fromUtf8("下楼器") || qname == QString::fromUtf8("撤场通行卡"))
        { fill = QColor(160, 110, 180); border = QColor(120, 80, 140);
          label = QString::fromUtf8("下楼器"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("临时护盾") || qname == QString::fromUtf8("乐队护盾贴"))
        { fill = QColor(80, 160, 220); border = QColor(50, 120, 180);
          label = QString::fromUtf8("护盾"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("企鹅玩偶") || qname == QString::fromUtf8("立希企鹅挂件"))
        { fill = QColor(220, 130, 170); border = QColor(170, 80, 120);
          label = QString::fromUtf8("企鹅"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("抹茶芭菲") || qname == QString::fromUtf8("乐奈抹茶芭菲"))
        { fill = QColor(140, 200, 100); border = QColor(90, 150, 50);
          label = QString::fromUtf8("芭菲"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("幸运金币") || qname == QString::fromUtf8("乐奈幸运硬币"))
        { fill = QColor(240, 200, 20); border = QColor(200, 150, 10);
          label = QString::fromUtf8("幸运币"); textColor = QColor(80, 40, 0); return; }

    // fallback
    fill = QColor(60, 170, 60); border = QColor(40, 130, 40);
    label = QString::fromUtf8("宝"); textColor = QColor(255, 255, 100);
}

// 返回图块类型的文字标签
static QString tileLabel(int tileType)
{
    switch (tileType) {
    case Tile_StairsUp:   return QString::fromUtf8("↑"); // ↑
    case Tile_StairsDown: return QString::fromUtf8("↓"); // ↓
    case Tile_DoorRed:    return QString::fromUtf8("红门");
    case Tile_DoorBlue:   return QString::fromUtf8("蓝门");
    case Tile_DoorGreen:  return QString::fromUtf8("黄门");
    case Tile_DoorMagic:  return QString::fromUtf8("魔法门");
    case Tile_DoorIron:   return QString::fromUtf8("铁门");
    case Tile_Lava:       return QString::fromUtf8("岩浆");
    case Tile_StarRiver:  return QString::fromUtf8("星河");
    case Tile_NPC:        return QString::fromUtf8("NPC");
    case Tile_Shop:       return QString::fromUtf8("商店");
    default:              return {};
    }
}

// 在指定矩形上绘制文字（带半透明底条提升可读性）
static void drawOverlayText(QPainter& painter, const QRect& r, const QString& text,
                            int fontSize = 12, bool bold = true)
{
    if (text.isEmpty()) return;
    // 半透明背景条
    QRect textRect = r.adjusted(2, r.height() - 22, -2, -2);
    painter.fillRect(textRect, QColor(0, 0, 0, 140));
    QFont f;
    f.setPixelSize(fontSize);
    f.setBold(bold);
    painter.setFont(f);
    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter, text);
}

void MapWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    // Keep sprite edges sharp and reduce per-frame filtering overhead.
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    if (!m_game) return;
    syncPlayerMotionTarget();

    if (!m_backgroundPix.isNull()) {
        // Cover the map viewport while preserving the scene's aspect ratio.
        if (m_backgroundViewport != size()) {
            m_backgroundScaled = m_backgroundPix.scaled(size(), Qt::KeepAspectRatioByExpanding,
                                                         Qt::FastTransformation);
            m_backgroundViewport = size();
        }
        const int ox = (m_backgroundScaled.width() - width()) / 2;
        const int oy = (m_backgroundScaled.height() - height()) / 2;
        painter.save();
        painter.setOpacity(0.34);
        painter.drawPixmap(0, 0, m_backgroundScaled, ox, oy, width(), height());
        painter.restore();
    }

    int w = m_game->width();
    int h = m_game->height();

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int t = m_game->map()[y * w + x];
            QRect r(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE);

            // 所有可行走对象先使用素材地板打底，透明角色和道具不再漏出背景图。
            if (t != Tile_Wall && t != Tile_DarkWall && t != Tile_Lava &&
                t != Tile_StarRiver && t != Tile_Empty) {
                auto floor = m_tilePix.find(Tile_Floor);
                if (floor != m_tilePix.end()) painter.drawPixmap(r, floor->second);
            }

            QPixmap* pix = nullptr;
            std::string monsterName;

            // -- 怪物 --
            if (t == Tile_Monster) {
                Monster* m = m_game->monsterAt(x, y);
                if (m) {
                    monsterName = m->GetName();
                    bool animatedTarget = false;
                    if (m_monsterMotionActive) {
                        for (const auto& motion : m_monsterMotions) {
                            if (qRound(motion.to.x()) == x && qRound(motion.to.y()) == y) {
                                animatedTarget = true;
                                break;
                            }
                        }
                    }
                    if (!animatedTarget) {
                        auto it = m_monsterPix.find(monsterName);
                        if (it != m_monsterPix.end()) {
                            pix = &it->second;
                        }
                    }
                }
                if (!pix) pix = &m_defaultMonsterPix;
                if (m_monsterMotionActive && pix == &m_defaultMonsterPix) {
                    for (const auto& motion : m_monsterMotions)
                        if (qRound(motion.to.x()) == x && qRound(motion.to.y()) == y) {
                            pix = nullptr;
                            break;
                        }
                }
            }

            // NPC 图块按角色名称选择素材；普通 NPC 仍回退到 Tile_NPC 默认图。
            if (t == Tile_NPC) {
                if (NPC* npc = m_game->npcAt(x, y)) {
                    auto npcImage = m_npcPix.find(npc->GetName());
                    if (npcImage != m_npcPix.end())
                        pix = &npcImage->second;
                }
            }

            // -- 道具：全部使用预制像素素材，不再由 QPainter 绘制形状 --
            if (t == Tile_Item) {
                const Item* item = m_game->itemAt(x, y);
                if (item) {
                    auto icon = m_itemPix.find(item->GetName());
                    if (icon == m_itemPix.end()) icon = m_itemPix.find("ClassicArtifact");
                    if (icon != m_itemPix.end()) painter.drawPixmap(r, icon->second);
                    continue;
                }
            }

            // -- 暗墙（有眼镜时）--
            if (t == Tile_DarkWall && m_game->player().hasGlasses)
                pix = &m_darkWallRevealed;

            // -- 普通图块 --
            if (!pix) {
                auto it = m_tilePix.find(t);
                if (it != m_tilePix.end()) {
                    pix = &it->second;
                }
            }

            // 绘制底图
            if (pix && !pix->isNull())
                painter.drawPixmap(r, *pix);

            // 每个怪物格显示当前属性下的预计掉血，以及减少一回合伤害所需的
            // 最小额外攻击力；计算公式与 Game::fightAt 保持一致。
            if (t == Tile_Monster) {
                if (const Monster* monster = m_game->monsterAt(x, y))
                    drawMonsterCombatHint(painter, r, m_game->player(), *monster);
            }

            // 门、楼梯、商店等均由素材本身表达，不再叠加代码绘制的标签底条。
        }
    }

    // 十层包围事件：逻辑坐标已切换到目标格，画面用插值把怪物从第三/第四排
    // 移动到侧翼，避免出现瞬移。目标格的静态怪物在上方循环中暂时隐藏。
    if (m_monsterMotionActive) {
        const qreal progress = std::clamp<qreal>(m_monsterMotionClock.elapsed() / 320.0, 0.0, 1.0);
        const qreal eased = progress * progress * (3.0 - 2.0 * progress);
        for (const auto& motion : m_monsterMotions) {
            const qreal x = motion.from.x() + (motion.to.x() - motion.from.x()) * eased;
            const qreal y = motion.from.y() + (motion.to.y() - motion.from.y()) * eased;
            auto it = m_monsterPix.find(motion.name);
            const QPixmap* pix = it != m_monsterPix.end() ? &it->second : &m_defaultMonsterPix;
            painter.drawPixmap(QPointF(x * TILE_SIZE, y * TILE_SIZE), *pix);
        }
    }

    // 玩家
    const int px = qRound(m_playerMotion.x() * TILE_SIZE);
    const int py = qRound(m_playerMotion.y() * TILE_SIZE);
    QRect pr(px, py, TILE_SIZE, TILE_SIZE);

    if (m_hasPlayerSheet && !m_playerFrames[m_playerDirectionRow * 4 + m_playerFrame].isNull()) {
        painter.drawPixmap(pr, m_playerFrames[m_playerDirectionRow * 4 + m_playerFrame]);
    } else if (!m_playerPix.isNull()) {
        painter.drawPixmap(pr, m_playerPix);
    }
}
