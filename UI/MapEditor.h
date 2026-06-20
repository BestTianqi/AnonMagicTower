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
    // NPC 交易
    bool npcIsTrader = false;
    int  npcTradeGoldCost = 0;
    std::string npcTradeRewardItem;
    int  npcTradeRewardValue = 50;
    // Shop
    int shopPotionPrice = 0;
    int shopWeaponPrice = 0;
    int shopArmorPrice  = 0;
    int shopPotionValue = 200;
    int shopWeaponValue = 5;
    int shopArmorValue  = 8;
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
    void setCurrentNPC(const std::string& name, const std::vector<std::string>& dialog,
                       const std::string& rewardItem, int rewardValue) {
        m_currentNPC = name;
        m_currentNPCDialog = dialog;
        m_currentNPCRewardItem = rewardItem;
        m_currentNPCRewardValue = rewardValue;
    }
    void setCurrentNPCTrade(bool isTrader, int goldCost, const std::string& tradeItem, int tradeValue) {
        m_npcIsTrader = isTrader;
        m_npcTradeGoldCost = goldCost;
        m_npcTradeRewardItem = tradeItem;
        m_npcTradeRewardValue = tradeValue;
    }
    // 从已放置的图块加载数据到当前编辑状态
    void loadFromTile(int x, int y);
    void setCurrentShop(int potionPrice, int weaponPrice, int armorPrice,
                        int potionValue, int weaponValue, int armorValue) {
        m_shopPotionPrice = potionPrice;
        m_shopWeaponPrice = weaponPrice;
        m_shopArmorPrice  = armorPrice;
        m_shopPotionValue = potionValue;
        m_shopWeaponValue = weaponValue;
        m_shopArmorValue  = armorValue;
    }

    const std::vector<EditorTile>& tiles() const { return m_floor.tiles; }
    void setTiles(const std::vector<EditorTile>& t) { m_floor.tiles = t; update(); }
    void setFloor(const EditorFloor& f) { m_floor = f; update(); }
    EditorFloor floorData() const { return m_floor; }

    int playerX() const { return m_floor.playerX; }
    int playerY() const { return m_floor.playerY; }

    void clearFloor();

    // 读取当前编辑状态（用于面板反写）
    const std::string& currentMonster() const { return m_currentMonster; }
    const std::string& currentNPC() const { return m_currentNPC; }
    const std::vector<std::string>& currentNPCDialog() const { return m_currentNPCDialog; }
    const std::string& currentNPCRewardItem() const { return m_currentNPCRewardItem; }
    int currentNPCRewardValue() const { return m_currentNPCRewardValue; }
    bool currentNPCIsTrader() const { return m_npcIsTrader; }
    int  currentNPCTradeGoldCost() const { return m_npcTradeGoldCost; }
    const std::string& currentNPCTradeRewardItem() const { return m_npcTradeRewardItem; }
    int  currentNPCTradeRewardValue() const { return m_npcTradeRewardValue; }

signals:
    void tileChanged(int x, int y);
    void playerMoved(int x, int y);
    void tilePicked(int x, int y);

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
    std::vector<std::string> m_currentNPCDialog;
    std::string m_currentNPCRewardItem;
    int m_currentNPCRewardValue = 0;
    bool m_npcIsTrader = false;
    int  m_npcTradeGoldCost = 0;
    std::string m_npcTradeRewardItem;
    int  m_npcTradeRewardValue = 50;
    int m_shopPotionPrice = 0;
    int m_shopWeaponPrice = 0;
    int m_shopArmorPrice  = 0;
    int m_shopPotionValue = 200;
    int m_shopWeaponValue = 5;
    int m_shopArmorValue  = 8;
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
    void onSaveAsDefault();
    void onTestPlay();
    void onExportFloor();
    bool saveToPath(const QString& path);
    void onTileTypeChanged(int id);
    void addFloor();
    void removeFloor();
    void updatePanelForTile(int tileType);

    MapEditWidget* m_edit;
    QComboBox*     m_tileCombo;
    int            m_currentTileType = 1;  // Tile_Wall
    QString        m_selectedItemName;
    int            m_selectedItemValue = 50;
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
    QSpinBox*      m_npcRewardValueSpin;
    QCheckBox*     m_npcTradeCheck;
    QWidget*       m_npcTradePanel;
    QSpinBox*      m_npcTradeGoldSpin;
    QComboBox*     m_npcTradeRewardCombo;
    QSpinBox*      m_npcTradeRewardValueSpin;

    // 商店面板
    QWidget*       m_shopPanel;
    QSpinBox*      m_shopPotionPriceSpin;
    QSpinBox*      m_shopWeaponPriceSpin;
    QSpinBox*      m_shopArmorPriceSpin;
    QSpinBox*      m_shopPotionValueSpin;
    QSpinBox*      m_shopWeaponValueSpin;
    QSpinBox*      m_shopArmorValueSpin;

    // 多楼层数据
    std::unordered_map<int, EditorFloor> m_floors;
    int m_currentFloor = 1;

    void storeCurrentFloor();
    void switchToFloor(int floor, bool storeCurrent = true);
};
