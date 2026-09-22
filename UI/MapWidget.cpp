#include "MapWidget.h"
#include "Entities/MonsterDB.h"
#include <QPainter>
#include <QFont>
#include <QImage>
#include <QColor>
#include <QtMath>
#include <algorithm>

namespace {
constexpr float kPlayerWalkSpeed = 360.0f;
// 逻辑格仍保持 60×60；贴图向相邻格轻微溢出，覆盖素材透明边缘造成的缝隙。
constexpr int kTileRenderBleed = 6;
constexpr int kTileRenderSize = TILE_SIZE + kTileRenderBleed * 2;

QRect tileRenderRect(int x, int y)
{
    return QRect(x * TILE_SIZE - kTileRenderBleed,
                 y * TILE_SIZE - kTileRenderBleed,
                 kTileRenderSize, kTileRenderSize);
}

QColor tileSeamColor(int tileType)
{
    switch (tileType) {
    case Tile_Wall:
        return QColor(18, 22, 32);       // 墙：深色，强化边界
    case Tile_DarkWall:
        return QColor(58, 62, 72);       // 暗墙：比普通墙略浅，仍保持墙体辨识度
    case Tile_Lava:
        return QColor(112, 30, 18);
    case Tile_StarRiver:
        return QColor(20, 25, 72);
    case Tile_Floor:
    case Tile_Item:
    case Tile_Monster:
    case Tile_NPC:
    case Tile_Shop:
    case Tile_StairsUp:
    case Tile_StairsDown:
    case Tile_DoorRed:
    case Tile_DoorBlue:
    case Tile_DoorGreen:
    case Tile_DoorMagic:
    case Tile_DoorIron:
        return QColor(196, 190, 177);     // 地板与可行走格：浅色连续底
    default:
        return QColor(28, 31, 43);
    }
}

struct MonsterCombatHint {
    int hpLoss = -1;
    int attackDelta = -1;
};

MonsterCombatHint monsterCombatHint(const Player& player, const Monster& monster)
{
    const std::string& name = monster.GetName();
    const bool vampireOrOrc = MonsterDB::isVampireOrOrc(name);
    const bool dragon = MonsterDB::isDragon(name);
    const int attackMultiplier = (player.hasCross && vampireOrOrc) ||
                                 (player.hasDragonSlayer && dragon) ? 2 : 1;
    int incoming = std::max(0, monster.GetATK() - player.def -
                            (player.tempShieldCharges > 0 ? 50 : 0));
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
    // 暗墙始终比普通墙略浅，便于识别但不会误认为地板。
    m_tilePix[Tile_DarkWall] = makePixmap(QColor(82, 84, 92), QColor(60, 62, 70), "");
    // 暗墙（有眼镜时变浅）
    m_darkWallRevealed = makePixmap(QColor(100, 95, 85), QColor(75, 70, 60), "暗", QColor(180, 180, 160), 10);
    // 上楼
    m_tilePix[Tile_StairsUp] = makePixmap(QColor(72, 58, 12), QColor(210, 175, 40),
        QString::fromUtf8("↑"), Qt::white, 20);
    // 下楼
    m_tilePix[Tile_StairsDown] = makePixmap(QColor(52, 28, 85), QColor(150, 90, 220),
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
                    image.setPixelColor(x, y, QColor(color.red() * 0.52,
                                                     color.green() * 0.52,
                                                     color.blue() * 0.52,
                                                     color.alpha()));
                }
            }
            // 楼梯素材本身带透明边缘，铺一层深色舞台底板并加高对比边框，
            // 避免在深色地图背景中与普通地板混淆。
            QImage plate(image.size(), QImage::Format_ARGB32_Premultiplied);
            const bool up = tileType == Tile_StairsUp;
            plate.fill(up ? QColor(48, 38, 8) : QColor(31, 19, 48));
            QPainter platePainter(&plate);
            platePainter.drawImage(0, 0, image);
            platePainter.setPen(QPen(up ? QColor(220, 185, 58) : QColor(165, 105, 225), 2));
            platePainter.drawRect(1, 1, plate.width() - 3, plate.height() - 3);
            platePainter.end();
            px = QPixmap::fromImage(plate);
        } else if (tileType == Tile_DarkWall) {
            QImage image = px.toImage().convertToFormat(QImage::Format_ARGB32);
            for (int y = 0; y < image.height(); ++y) {
                for (int x = 0; x < image.width(); ++x) {
                    const QColor color = QColor::fromRgba(image.pixel(x, y));
                    const QColor lighter = color.lighter(145);
                    image.setPixelColor(x, y, QColor(lighter.red(), lighter.green(),
                                                     lighter.blue(), color.alpha()));
                }
            }
            px = QPixmap::fromImage(image);
        }
        // Pixel-art assets must remain crisp; nearest-neighbor scaling also avoids
        // filtering work on every custom tile during startup.
        m_tilePix[tileType] = px.scaled(kTileRenderSize, kTileRenderSize,
                                        Qt::IgnoreAspectRatio, Qt::FastTransformation);
        m_hasTileImage.insert(tileType);
    }
}

void MapWidget::loadMonsterImage(const std::string& name, const QString& path)
{
    QPixmap px(path);
    if (!px.isNull()) {
        // Keep the high-resolution source.  Scaling it down here with nearest
        // neighbour permanently discarded facial and costume detail before
        // the map was painted.
        m_monsterPix[name] = px;
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
    if (sheet.isNull())
        return;

    // 新素材是 8×8（64 帧），旧素材仍按 4×4 读取，保持向后兼容。
    const bool isEightByEight = sheet.width() >= TILE_SIZE * 8 &&
                                sheet.height() >= TILE_SIZE * 8;
    const int columns = isEightByEight ? 8 : 4;
    const int rows = isEightByEight ? 8 : 4;
    if (sheet.width() < TILE_SIZE * columns || sheet.height() < TILE_SIZE * rows)
        return;

    const int frameWidth = sheet.width() / columns;
    const int frameHeight = sheet.height() / rows;
    if (frameWidth <= 0 || frameHeight <= 0)
        return;

    const QImage source = sheet.toImage().convertToFormat(QImage::Format_ARGB32);
    for (auto& frame : m_playerFrames)
        frame = QPixmap();
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            const int index = row * columns + col;
            QImage frame = source.copy(col * frameWidth, row * frameHeight,
                                       frameWidth, frameHeight);
            // Keep a hard transparent guard around every frame. This prevents
            // a foot at the bottom of one cell from bleeding into the head of
            // the next cell when a generated sheet is scaled or filtered.
            const int guard = std::min(2, std::min(frame.width(), frame.height()) / 2);
            for (int y = 0; y < frame.height(); ++y) {
                for (int x = 0; x < frame.width(); ++x) {
                    if (x < guard || y < guard ||
                        x >= frame.width() - guard || y >= frame.height() - guard)
                        frame.setPixel(x, y, qRgba(0, 0, 0, 0));
                }
            }
            m_playerFrames[static_cast<size_t>(index)] = QPixmap::fromImage(frame);
        }
    }
    m_playerSheetColumns = columns;
    m_playerSheetRows = rows;
    m_hasPlayerSheet = true;
    update();
}

void MapWidget::loadPlayerOutfitSpriteSheet(const QString& outfitId, const QString& path)
{
    const std::string id = outfitId.toStdString();
    // *_actions sheets contain isolated gesture poses rather than a
    // directional walk cycle; never expose them as movement outfits.
    if (id.empty() || path.isEmpty() ||
        outfitId.endsWith(QStringLiteral("_actions"), Qt::CaseInsensitive))
        return;
    m_playerOutfitPaths[id] = path;
    if (m_activePlayerOutfit.empty())
        setPlayerOutfit(outfitId);
}

bool MapWidget::setPlayerOutfit(const QString& outfitId)
{
    const auto it = m_playerOutfitPaths.find(outfitId.toStdString());
    if (it == m_playerOutfitPaths.end())
        return false;
    loadPlayerSpriteSheet(it->second);
    if (!m_hasPlayerSheet)
        return false;
    m_activePlayerOutfit = it->first;
    return true;
}

void MapWidget::setPlayerDirection(int dx, int dy)
{
    if (dx < 0) m_playerDirectionRow = 1;
    else if (dx > 0) m_playerDirectionRow = 2;
    else if (dy < 0) m_playerDirectionRow = 3;
    else if (dy > 0) m_playerDirectionRow = 0;
}

void MapWidget::playPlayerPath(const std::vector<std::pair<int, int>>& path)
{
    m_scriptedPlayerPath.clear();
    m_scriptedPlayerPathIndex = 0;
    if (!m_game || path.size() <= 1 || !m_movementAnimationEnabled) {
        snapPlayerToGame();
        return;
    }

    m_scriptedPlayerPath = path;
    m_scriptedPlayerPathIndex = 1;
    m_motionInitialized = true;
    m_playerMotion.snapTo(path.front().first, path.front().second);
    m_playerFrame = 1;
    update();
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
    if (!m_scriptedPlayerPath.empty()) return;
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
    m_scriptedPlayerPath.clear();
    m_scriptedPlayerPathIndex = 0;
    m_motionInitialized = false;
    m_monsterMotions.clear();
    m_monsterMotionIndex = 0;
    m_monsterMotionActive = false;
    syncPlayerMotionTarget();
    update();
}

void MapWidget::advancePlayerMotion()
{
    const qint64 elapsedMs = std::clamp<qint64>(m_motionClock.restart(), 1, 50);
    if (!m_scriptedPlayerPath.empty()) {
        float remainingMs = static_cast<float>(elapsedMs);
        while (remainingMs > 0.0f) {
            if (!m_playerMotion.isMoving() &&
                m_scriptedPlayerPathIndex < m_scriptedPlayerPath.size()) {
                const auto [fromX, fromY] = m_scriptedPlayerPath[m_scriptedPlayerPathIndex - 1];
                const auto [toX, toY] = m_scriptedPlayerPath[m_scriptedPlayerPathIndex];
                setPlayerDirection(toX - fromX, toY - fromY);
                m_playerMotion.beginGridStep(fromX, fromY, toX, toY, kPlayerWalkSpeed);
                ++m_scriptedPlayerPathIndex;
            }
            if (!m_playerMotion.isMoving()) break;
            remainingMs = m_playerMotion.advanceWithOverflow(remainingMs);
            if (!m_playerMotion.isMoving() &&
                m_scriptedPlayerPathIndex >= m_scriptedPlayerPath.size()) {
                const auto [lastX, lastY] = m_scriptedPlayerPath.back();
                m_lastPlayerTileX = lastX;
                m_lastPlayerTileY = lastY;
                m_scriptedPlayerPath.clear();
                m_scriptedPlayerPathIndex = 0;
                m_playerFrame = 1;
                emit playerMotionFinished();
                break;
            }
        }
        if (m_playerMotion.isMoving())
            m_playerFrame = m_playerMotion.walkingFrame(m_playerSheetColumns == 8 ? 8 : 4);
        update();
        return;
    }
    syncPlayerMotionTarget();
    advanceMonsterMotion();
    if (!m_motionInitialized) return;
    if (m_playerMotion.isMoving()) {
        float remainingMs = static_cast<float>(elapsedMs);
        while (remainingMs > 0.0f && m_playerMotion.isMoving()) {
            remainingMs = m_playerMotion.advanceWithOverflow(remainingMs);
            if (m_playerMotion.isMoving()) break;
            emit playerMotionFinished();
            // 同步槽可能已提交下一格逻辑位置。立即衔接并消费本帧剩余
            // 时间，避免每到格子边界固定停一帧。
            syncPlayerMotionTarget();
            if (!m_playerMotion.isMoving()) {
                m_playerFrame = 1;
                break;
            }
        }
        if (m_playerMotion.isMoving())
            m_playerFrame = m_playerMotion.walkingFrame(m_playerSheetColumns == 8 ? 8 : 4);
        update();
    }
}

void MapWidget::playMonsterMovement(const std::vector<Game::MonsterMovementAnimation>& movements)
{
    m_monsterMotions.clear();
    m_monsterMotionIndex = 0;
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
        ++m_monsterMotionIndex;
        if (m_monsterMotionIndex >= m_monsterMotions.size()) {
            m_monsterMotions.clear();
            m_monsterMotionIndex = 0;
            m_monsterMotionActive = false;
            emit monsterMotionFinished();
        } else {
            m_monsterMotionClock.restart();
        }
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
        m_darkWallRevealed = px.scaled(kTileRenderSize, kTileRenderSize, Qt::IgnoreAspectRatio,
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
            const QRect tileRect = tileRenderRect(x, y);

            // 素材多为带透明边缘的舞台贴图；先铺连续底色，避免格子间透出背景。
            if (t != Tile_Empty)
                painter.fillRect(r, tileSeamColor(t));

            // 所有可行走对象先使用素材地板打底，透明角色和道具不再漏出背景图。
            if (t != Tile_Wall && t != Tile_DarkWall && t != Tile_Lava &&
                t != Tile_StarRiver && t != Tile_Empty) {
                auto floor = m_tilePix.find(Tile_Floor);
                if (floor != m_tilePix.end()) painter.drawPixmap(tileRect, floor->second);
            }

            QPixmap* pix = nullptr;
            std::string monsterName;
            bool actorSprite = false;

            // -- 怪物 --
            if (t == Tile_Monster) {
                // 怪物统一延后到玩家之后绘制，保证完全不透明并位于最上层。
                continue;
            }

            // NPC 图块按角色名称选择素材；普通 NPC 仍回退到 Tile_NPC 默认图。
            if (t == Tile_NPC) {
                actorSprite = true;
                if (NPC* npc = m_game->npcAt(x, y)) {
                    auto npcImage = m_npcPix.find(npc->IsTrader() ? "麻里奈" : npc->GetName());
                    if (npcImage != m_npcPix.end())
                        pix = &npcImage->second;
                }
            }

            if (t == Tile_Shop) {
                actorSprite = true;
                if (ShopData* shop = m_game->shopAt(x, y)) {
                    const bool attributeShop = shop->classicShopFloor > 0 && shop->classicShopFloor != 28;
                    const auto shopImage = m_npcPix.find(attributeShop ? "弦卷心" : "麻里奈");
                    if (shopImage != m_npcPix.end()) pix = &shopImage->second;
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
                painter.drawPixmap(actorSprite ? r : tileRect, *pix);

            // 门、楼梯、商店等均由素材本身表达，不再叠加代码绘制的标签底条。
        }
    }

    // 在会实际触发魔法领域/警卫夹击的可行走格上显示当前生命损失。
    // 数值随当前 HP 变化；神圣盾生效时 Game 返回 0，因此标记立即清除。
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int tile = m_game->tileAt(x, y);
            const bool walkable = tile == Tile_Floor || tile == Tile_Item ||
                                  tile == Tile_NPC || tile == Tile_Shop ||
                                  tile == Tile_StairsUp || tile == Tile_StairsDown;
            if (!walkable) continue;
            const int damage = m_game->previewApproachHazardDamageAt(x, y);
            if (damage <= 0) continue;
            drawOverlayText(painter,
                            QRect(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE),
                            QStringLiteral("-%1").arg(damage), 11, true);
        }
    }

    // 玩家
    const int px = qRound(m_playerMotion.x() * TILE_SIZE);
    const int py = qRound(m_playerMotion.y() * TILE_SIZE);
    QRect pr(px, py, TILE_SIZE, TILE_SIZE);

    int playerFrameIndex = 0;
    if (m_playerSheetColumns == 8 && m_playerSheetRows == 8) {
        // 8×8 行走图：每个方向占两行，静止/预备行 + 行走动作行。
        const int actionRow = m_playerDirectionRow * 2 + (m_playerMotion.isMoving() ? 1 : 0);
        playerFrameIndex = actionRow * 8 + std::clamp(m_playerFrame, 0, 7);
    } else {
        playerFrameIndex = m_playerDirectionRow * 4 + std::clamp(m_playerFrame, 0, 3);
    }
    if (m_hasPlayerSheet && playerFrameIndex >= 0 &&
        playerFrameIndex < static_cast<int>(m_playerFrames.size()) &&
        !m_playerFrames[static_cast<size_t>(playerFrameIndex)].isNull()) {
        painter.drawPixmap(pr, m_playerFrames[static_cast<size_t>(playerFrameIndex)]);
    } else if (!m_playerPix.isNull()) {
        painter.drawPixmap(pr, m_playerPix);
    }

    // 静态怪物、战斗提示和移动怪物全部位于玩家之上，且不施加透明度。
    painter.save();
    painter.setOpacity(1.0);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (m_game->map()[y * w + x] != Tile_Monster) continue;
            if (m_game->floor32MonstersHidden()) continue;
            if (m_monsterMotionActive && m_monsterMotionIndex < m_monsterMotions.size()) {
                const auto& motion = m_monsterMotions[m_monsterMotionIndex];
                if (qRound(motion.to.x()) == x && qRound(motion.to.y()) == y)
                    continue;
            }
            const Monster* monster = m_game->monsterAt(x, y);
            const QPixmap* pix = &m_defaultMonsterPix;
            if (monster) {
                const auto it = m_monsterPix.find(monster->GetName());
                if (it != m_monsterPix.end()) pix = &it->second;
            }
            const QRect rect(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE);
            painter.drawPixmap(rect, *pix);
            if (monster && m_game->player().HasMonsterBook())
                drawMonsterCombatHint(painter, rect, m_game->player(), *monster);
        }
    }
    if (m_monsterMotionActive && m_monsterMotionIndex < m_monsterMotions.size()) {
        const qreal progress = std::clamp<qreal>(m_monsterMotionClock.elapsed() / 320.0, 0.0, 1.0);
        const qreal eased = progress * progress * (3.0 - 2.0 * progress);
        const auto& motion = m_monsterMotions[m_monsterMotionIndex];
        const qreal x = motion.from.x() + (motion.to.x() - motion.from.x()) * eased;
        const qreal y = motion.from.y() + (motion.to.y() - motion.from.y()) * eased;
        const auto it = m_monsterPix.find(motion.name);
        const QPixmap* pix = it != m_monsterPix.end() ? &it->second : &m_defaultMonsterPix;
        painter.drawPixmap(QRectF(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE), *pix,
                           QRectF(0, 0, pix->width(), pix->height()));
    }
    painter.restore();
}
