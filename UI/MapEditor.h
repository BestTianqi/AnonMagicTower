#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLabel>
#include <QPixmap>
#include <unordered_map>
#include <vector>
#include <string>

// 编辑器内部数据
struct EditorTile {
    int type = 1;  // Tile_Wall
    std::string monsterName;  // 仅 Tile_Monster 时有效
};

class MapEditWidget : public QWidget {
    Q_OBJECT
public:
    explicit MapEditWidget(QWidget* parent = nullptr);

    void setCurrentTile(int type) { m_currentTile = type; }
    int  currentTile() const { return m_currentTile; }
    void setCurrentMonster(const std::string& name) { m_currentMonster = name; }

    const std::vector<EditorTile>& tiles() const { return m_tiles; }
    void setTiles(const std::vector<EditorTile>& t) { m_tiles = t; update(); }

    int playerX() const { return m_playerX; }
    int playerY() const { return m_playerY; }
    void setPlayerPos(int x, int y) { m_playerX = x; m_playerY = y; update(); }

    void clearMap();

signals:
    void tileChanged(int x, int y);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void placeTile(int x, int y);
    int  indexAt(int x, int y) const { return y * 15 + x; }

    std::vector<EditorTile> m_tiles;  // 225
    int m_currentTile = 1;  // Tile_Wall
    std::string m_currentMonster;
    int m_playerX = 1;
    int m_playerY = 2;
    int m_hoverX = -1;
    int m_hoverY = -1;
};

class MapEditor : public QWidget {
    Q_OBJECT
public:
    explicit MapEditor(QWidget* parent = nullptr);

private:
    void onSave();
    void onLoad();
    void onTileTypeChanged(int id);
    void updateMonsterCombo();

    MapEditWidget* m_edit;
    QButtonGroup*  m_tileGroup;
    QComboBox*     m_monsterCombo;
    QLabel*        m_statusLabel;
};
