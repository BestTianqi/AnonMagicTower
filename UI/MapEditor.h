#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSpinBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QLabel>
#include <QStackedWidget>
#include <QPixmap>
#include <QCheckBox>
#include <unordered_map>
#include <vector>
#include <string>

// 编辑器图块数据
struct EditorTile {
    int type = 1;  // Tile_Wall
    // 怪物
    std::string monsterName;
    // 道具
    std::string itemName;
    int itemValue = 0;
    // NPC
    std::string npcName;
    std::vector<std::string> npcDialog;
    std::string npcRewardItem;
    int npcRewardValue = 0;
};

// 楼层数据
struct EditorFloor {
    std::vector<EditorTile> tiles;  // 225
    int playerX = 2;
    int playerY = 3;
};

class MapEditWidget : public QWidget {
    Q_OBJECT
public:
    explicit MapEditWidget(QWidget* parent = nullptr);

    void setCurrentTile(int type) { m_currentTile = type; }
    int  currentTile() const { return m_currentTile; }
    void setCurrentMonster(const std::string& name) { m_currentMonster = name; }
    void setCurrentItem(const std::string& name, int value) { m_currentItem = name; m_currentItemValue = value; }
    void setCurrentNPC(const std::string& name) { m_currentNPC = name; }

    const std::vector<EditorTile>& tiles() const { return m_floor.tiles; }
    void setTiles(const std::vector<EditorTile>& t) { m_floor.tiles = t; update(); }
    void setFloor(const EditorFloor& f) { m_floor = f; update(); }
    EditorFloor floorData() const { return m_floor; }

    int playerX() const { return m_floor.playerX; }
    int playerY() const { return m_floor.playerY; }

    void clearFloor();

signals:
    void tileChanged(int x, int y);
    void playerMoved(int x, int y);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void placeTile(int x, int y);
    int  indexAt(int x, int y) const { return y * 15 + x; }

    EditorFloor m_floor;
    int m_currentTile = 1;
    std::string m_currentMonster;
    std::string m_currentItem;
    int m_currentItemValue = 0;
    std::string m_currentNPC;
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
    void onNewMap();
    void onTestPlay();
    void onTileTypeChanged(int id);
    void onFloorChanged(int floor);
    void addFloor();
    void removeFloor();
    void updatePanelForTile(int tileType);

    MapEditWidget* m_edit;
    QButtonGroup*  m_tileGroup;
    QSpinBox*      m_floorSpin;
    QLabel*        m_floorCountLabel;
    QLabel*        m_statusLabel;
    QPushButton*   m_removeFloorBtn;

    // 条件面板
    QComboBox*     m_monsterCombo;
    QWidget*       m_itemPanel;
    QComboBox*     m_itemCombo;
    QSpinBox*      m_itemValueSpin;
    QLabel*        m_itemValueLabel;
    QWidget*       m_npcPanel;
    QLineEdit*     m_npcNameEdit;
    QTextEdit*     m_npcDialogEdit;
    QComboBox*     m_npcRewardCombo;

    // 多楼层数据
    std::unordered_map<int, EditorFloor> m_floors;
    int m_currentFloor = 1;

    void storeCurrentFloor();
    void switchToFloor(int floor);
};
