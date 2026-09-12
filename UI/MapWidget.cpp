#include "MapWidget.h"
#include "Entities/MonsterDB.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>

MapWidget::MapWidget(Game* game, QWidget* parent)
    : QWidget(parent), m_game(game)
{
    setFixedSize(900, 900);
    generatePlaceholders();
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
    m_tilePix[Tile_StairsUp] = makePixmap(QColor(180, 160, 50), QColor(140, 120, 30),
        QString::fromUtf8("↑"), Qt::black, 20);
    // 下楼
    m_tilePix[Tile_StairsDown] = makePixmap(QColor(160, 100, 180), QColor(120, 70, 140),
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
    // 绿门
    m_tilePix[Tile_DoorGreen] = makePixmap(QColor(50, 160, 70), QColor(30, 120, 40),
        QString::fromUtf8("绿门"), Qt::white, 10);
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
    if (qname == QString::fromUtf8("Red Key") || qname == QString::fromUtf8("红钥匙"))
        { fill = QColor(200, 45, 45); border = QColor(160, 20, 20);
          label = QString::fromUtf8("红钥"); textColor = QColor(255, 220, 100); return; }
    if (qname == QString::fromUtf8("Blue Key") || qname == QString::fromUtf8("蓝钥匙"))
        { fill = QColor(45, 60, 200); border = QColor(20, 30, 160);
          label = QString::fromUtf8("蓝钥"); textColor = QColor(255, 220, 100); return; }
    if (qname == QString::fromUtf8("Green Key") || qname == QString::fromUtf8("绿钥匙"))
        { fill = QColor(45, 180, 60); border = QColor(20, 140, 30);
          label = QString::fromUtf8("绿钥"); textColor = QColor(255, 220, 100); return; }
    if (qname == QString::fromUtf8("万能钥匙"))
        { fill = QColor(130, 60, 200); border = QColor(90, 30, 160);
          label = QString::fromUtf8("万能钥"); textColor = QColor(255, 220, 100); return; }

    // 属性类
    if (qname == QString::fromUtf8("Potion") || qname == QString::fromUtf8("生命药"))
        { fill = QColor(200, 60, 60); border = QColor(150, 30, 30);
          label = QString::fromUtf8("生命药"); textColor = Qt::white;
          desc = QString("+%1HP").arg(value); return; }
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

    // 特殊道具
    if (qname == QString::fromUtf8("匿名眼镜"))
        { fill = QColor(40, 180, 180); border = QColor(20, 130, 130);
          label = QString::fromUtf8("眼镜"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("破墙锤"))
        { fill = QColor(140, 100, 70); border = QColor(100, 70, 40);
          label = QString::fromUtf8("破墙锤"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("上楼器"))
        { fill = QColor(180, 170, 60); border = QColor(140, 130, 30);
          label = QString::fromUtf8("上楼器"); textColor = Qt::black; return; }
    if (qname == QString::fromUtf8("下楼器"))
        { fill = QColor(160, 110, 180); border = QColor(120, 80, 140);
          label = QString::fromUtf8("下楼器"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("临时护盾"))
        { fill = QColor(80, 160, 220); border = QColor(50, 120, 180);
          label = QString::fromUtf8("护盾"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("企鹅玩偶"))
        { fill = QColor(220, 130, 170); border = QColor(170, 80, 120);
          label = QString::fromUtf8("企鹅"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("抹茶芭菲"))
        { fill = QColor(140, 200, 100); border = QColor(90, 150, 50);
          label = QString::fromUtf8("芭菲"); textColor = Qt::white; return; }
    if (qname == QString::fromUtf8("幸运金币"))
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
    case Tile_DoorGreen:  return QString::fromUtf8("绿门");
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

    int w = m_game->width();
    int h = m_game->height();

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int t = m_game->map()[y * w + x];
            QRect r(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE);

            QPixmap* pix = nullptr;
            bool hasRealImage = false;
            std::string monsterName;

            // -- 怪物 --
            if (t == Tile_Monster) {
                Monster* m = m_game->monsterAt(x, y);
                if (m) {
                    monsterName = m->GetName();
                    auto it = m_monsterPix.find(monsterName);
                    if (it != m_monsterPix.end()) {
                        pix = &it->second;
                        hasRealImage = (m_hasMonsterImage.count(monsterName) > 0);
                    }
                }
                if (!pix) pix = &m_defaultMonsterPix;
            }

            // -- 道具（始终动态渲染）--
            if (t == Tile_Item) {
                const Item* item = m_game->itemAt(x, y);
                if (item) {
                    QColor fill, border, textColor;
                    QString label, desc;
                    itemAppearance(item->GetName(), item->GetValue(), fill, border, label, desc, textColor);

                    QPixmap px(TILE_SIZE, TILE_SIZE);
                    px.fill(Qt::transparent);
                    {
                        QPainter p(&px);
                        p.setRenderHint(QPainter::Antialiasing);
                        QRect inner(1, 1, TILE_SIZE - 2, TILE_SIZE - 2);

                        // 如果有加载的 Tile_Item 图片，用它做底
                        auto it = m_tilePix.find(Tile_Item);
                        if (it != m_tilePix.end() && m_hasTileImage.count(Tile_Item))
                            p.drawPixmap(0, 0, it->second);
                        else
                            p.fillRect(inner, fill);
                        p.setPen(QPen(QColor(50, 50, 50), 1));
                        p.drawRect(0, 0, TILE_SIZE - 1, TILE_SIZE - 1);

                        // 特殊形状道具
                        std::string iname = item->GetName();
                        QRect full(0, 0, TILE_SIZE, TILE_SIZE);
                        if (isKeyItem(iname)) {
                            drawKeyShape(p, full, fill);
                        } else if (iname == "Treasure" || iname == "金币") {
                            drawCoinShape(p, full, fill);
                            drawOverlayText(p, full, desc, 10, true);
                        } else if (iname == "Weapon" || iname == "武器") {
                            drawSwordShape(p, full, fill);
                            drawOverlayText(p, full, desc, 10, true);
                        } else if (iname == "Armor" || iname == "防具") {
                            drawShieldShape(p, full, fill);
                            drawOverlayText(p, full, desc, 10, true);
                        } else if (iname == "Potion" || iname == "生命药") {
                            drawPotionShape(p, full, fill);
                            drawOverlayText(p, full, desc, 10, true);
                        } else if (iname == "匿名眼镜") {
                            drawGlassesShape(p, full, fill);
                        } else if (iname == "破墙锤") {
                            drawHammerShape(p, full, fill);
                        } else if (iname == "上楼器") {
                            drawUpArrowShape(p, full, fill);
                        } else if (iname == "下楼器") {
                            drawDownArrowShape(p, full, fill);
                        } else if (iname == "临时护盾") {
                            drawTempShieldShape(p, full, fill);
                        } else if (iname == "企鹅玩偶") {
                            drawPenguinShape(p, full, fill);
                        } else if (iname == "抹茶芭菲") {
                            drawParfaitShape(p, full, fill);
                        } else if (iname == "幸运金币") {
                            drawLuckyCoinShape(p, full, fill);
                        } else if (desc.isEmpty()) {
                            QFont f;
                            f.setPixelSize(label.length() > 2 ? 11 : 14);
                            f.setBold(true);
                            p.setFont(f);
                            p.setPen(textColor);
                            p.drawText(QRect(0, 0, TILE_SIZE, TILE_SIZE),
                                Qt::AlignCenter, label);
                        } else {
                            QFont f;
                            f.setPixelSize(11);
                            f.setBold(true);
                            p.setFont(f);
                            p.setPen(textColor);
                            p.drawText(QRect(0, 2, TILE_SIZE, 20),
                                Qt::AlignHCenter | Qt::AlignTop, label);
                            f.setPixelSize(10);
                            f.setBold(false);
                            p.setFont(f);
                            p.drawText(QRect(2, 28, TILE_SIZE - 4, 28),
                                Qt::AlignHCenter | Qt::AlignTop, desc);
                        }
                    }
                    px.detach();
                    painter.drawPixmap(r, px);
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
                    hasRealImage = (m_hasTileImage.count(t) > 0);
                }
            }

            // 绘制底图
            if (pix && !pix->isNull())
                painter.drawPixmap(r, *pix);

            // 如果有真实图片，叠加文字（怪物除外，怪物信息在左侧面板显示）
            if (hasRealImage && t != Tile_Monster) {
                // NPC 特殊处理：显示 NPC 名字
                if (t == Tile_NPC) {
                    const NPC* npc = m_game->npcAt(x, y);
                    QString name = npc ? QString::fromStdString(npc->GetName()).left(5)
                                       : QString::fromUtf8("NPC");
                    drawOverlayText(painter, r, name, 11, true);
                } else {
                    drawOverlayText(painter, r, tileLabel(t), 11, true);
                }
            }
        }
    }

    // 玩家
    int px = m_game->player().x;
    int py = m_game->player().y;
    QRect pr(px * TILE_SIZE, py * TILE_SIZE, TILE_SIZE, TILE_SIZE);

    if (!m_playerPix.isNull()) {
        painter.drawPixmap(pr, m_playerPix);
    }
}
