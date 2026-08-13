# 魔塔 代码说明文档

## 目录

- [入口](#入口)
- [Game 核心逻辑](#game-核心逻辑)
- [Entities 实体层](#entities-实体层)
- [UI 用户界面](#ui-用户界面)
- [资源文件](#资源文件)

---

## 入口

### main.cpp

应用程序入口。创建 `QApplication`，显示 `MenuWindow` 主菜单，进入 Qt 事件循环。

```
main() → QApplication → MenuWindow → 事件循环
```

---

## Game 核心逻辑

### Game/Game.h

**游戏主类**，管理所有运行时状态。

| 成员 | 作用 |
|------|------|
| `FloorData` 结构体 | 单层数据：地图网格 `map`、怪物表 `monsters`、道具表 `items`、NPC 表 `npcs`、商店表 `shops` |
| `TileType` 枚举 | 13 种图块：空(0)、墙、地板、上楼、下楼、怪物、道具、红/蓝/绿门、NPC、商店、暗墙 |
| `MoveResult` 枚举 | 移动结果：正常、阻塞、拾取、遭遇、NPC、楼梯、死亡、门锁、商店 |
| `FightResult` 枚举 | 战斗结果：玩家胜、玩家死、游戏通关、僵持 |
| `ShopData` 结构体 | 商店配置：三种商品的价格和数值 |

关键方法：
- `tryMovePlayer(x, y)` — 移动核心：检测目标图块 → 墙壁/暗墙/门/道具/怪物/NPC/商店/楼梯各分支处理
- `fightAt(x, y, log)` — 回合制战斗：循环执行"玩家攻击→怪物攻击→吸收盾消耗"，返回结果
- `goUpFloor/goDownFloor` — 楼层切换 + 楼梯定位
- `saveToFile/loadFromFile` — 存档序列化（文本格式，每层标记 `LAYER`）
- `loadDefaultMap()` — 从嵌入的 `:/map.txt` 加载默认地图
- `initFloor(n)` — 初始化楼层（两圈外墙 + 全地板）
- `createItemByName()` — 工厂方法：根据名称字符串创建道具对象

### Game/Game.cpp

`Game.h` 的全部实现，约 560 行。包括：
- 存档 `MOTA2` 格式读写
- 战斗回合计算（含吸收盾、被动减伤、幸运金币翻倍）
- 楼层切换的楼梯搜索算法

---

## Entities 实体层

### Entities/Player.h / Player.cpp

**玩家类**。

| 字段 | 说明 |
|------|------|
| `x, y` | 当前坐标 |
| `hp, atk, def, gold` | 基础属性 |
| `hasGlasses` | 匿名眼镜 → 可看怪物信息 + 暗墙可见 |
| `hasPenguinDoll` | 企鹅玩偶 → 对"高松灯"/"企鹅"伤害减半 |
| `hasMatchaParfait` | 抹茶芭菲 → 对"要乐奈"/"小猫"伤害减半 |
| `hasLuckyCoin` | 幸运金币 → 金币翻倍 |
| `magicKeyUses` | 万能钥匙剩余次数（3 次） |
| `tempShieldCharges` | 临时护盾剩余战斗场次 |
| `wallBreakerUsed` | 破墙锤激活标记 |
| `stairUpUsed/ stairDownUsed` | 上楼器/下楼器激活标记 |
| `shopUseCount` | 商店购买次数（影响售价） |

钥匙管理：`m_keys` 哈希表 → `AddKey / HasKey / UseKey / KeyCount`  
物品背包：`m_items` 向量 → `AddItem / UseItem / GetItem`

---

### Entities/Items.h / Items.cpp

**道具体系**。基类 `Item`（名称+数值），派生类按用途分类：

| 类 | 拾取方式 | 效果 |
|----|----------|------|
| `Potion` | 拾取即用 | 恢复 HP |
| `Weapon` | 拾取即用 | 增加 ATK |
| `Armor` | 拾取即用 | 增加 DEF |
| `Treasure` | 拾取即用 | 获得金币 |
| `Key` | 拾取即用 | 对应颜色钥匙 +1 |
| `MagicKey` | 拾取即用 | 万能钥匙 +3 次 |
| `AnonGlasses` | 被动永久 | 查看怪物属性 + 暗墙可见 |
| `PenguinDoll` | 被动永久 | 对企鹅/高松灯减伤 50% |
| `MatchaParfait` | 被动永久 | 对要乐奈/小猫减伤 50% |
| `LuckyCoin` | 被动永久 | 金币翻倍 |
| `TempShield` | 放入背包手动用 | 下 N 场战斗防御 +50 |
| `StairUpper` | 放入背包手动用 | 下一次移动上楼（不找楼梯） |
| `StairLower` | 放入背包手动用 | 下一次移动下楼（不找楼梯） |
| `WallBreaker` | 放入背包手动用 | 下一次移动破墙 |

---

### Entities/Monster.h / Monster.cpp

**怪物类**。纯数据对象：名称、HP、ATK、DEF、金币。提供 `TakeDamage` / `TakeDamageRaw` 扣血方法、`IsDead` 判断。

### Entities/MonsterDB.h

**18 种怪物预设数据表**。提供 `MonsterDB::all()` 返回全部怪物列表，`MonsterDB::get(name)` 按名称查找，`MonsterDB::indexOf(name)` 按名称获取索引（用于匹配图片 monster_XX.png）。

怪物难度递进：从小猫(25HP/8ATK)到长崎素世(69696HP/969ATK)。

---

### Entities/NPC.h / NPC.cpp

**NPC 类**。支持三种交互：

| 类型 | 配置 | 行为 |
|------|------|------|
| 普通 NPC | `dialog` + 可选 `reward` | 对话 → 获得奖励（一次性） |
| 交易 NPC | `isTrader=true` + `tradeGoldCost` + `tradeReward` | 对话 → 扣金币 → 获得物品 |

`Interact(Player)` 方法根据类型返回对应回复文本。

---

## UI 用户界面

### UI/MenuWindow.h / MenuWindow.cpp

**主菜单窗口**。四个按钮：
- **新游戏** → 创建 `Game` → `loadDefaultMap()` → `enterGame()`
- **读取存档** → 文件对话框 → `loadFromFile()` → `enterGame()`
- **地图编辑器** → 打开 `MapEditor` 独立窗口
- **设置** → 弹出操作说明

`enterGame()` 创建 `MainWindow`，调用 `loadAssets()` 加载图片，隐藏菜单。

### UI/ui_MenuWindow.h

菜单窗口的 **UI 布局代码**（Qt Designer 风格）。四个按钮垂直排列，暗色主题样式。

---

### UI/MainWindow.h / MainWindow.cpp

**游戏主窗口**（核心 UI，约 800 行）。

| 方法 | 功能 |
|------|------|
| `keyPressEvent()` | 方向键处理 → `tryMovePlayer` → 根据结果弹窗/战斗/对话 |
| `loadAssets()` | 从 Qt 资源加载玩家/NPC/怪物图片到 MapWidget |
| `updateHUD()` | 刷新右侧面板：楼层、HP/ATK/DEF/金币/钥匙/背包 |
| `updateMonsterPanel()` | 刷新左侧面板：列出本层所有怪物（图片+属性） |
| `showInventory()` | 背包对话框：列表展示 + 双击使用 + 查看被动效果 |
| `showNPCDialog(x,y)` | NPC 对话/交易对话框 |
| `showShopDialog(x,y)` | 商店对话框：三商品 + 递增价格 + 购买 |
| `showModifier()` | 修改器：属性标签页(HP/ATK/DEF/金币/钥匙) + 道具标签页(14种道具按钮+效果切换) |
| `gameOver()` | 死亡处理：重新开始 / 返回菜单 |
| `gameWin()` | 通关处理：恭喜 / 重新开始 / 返回菜单 |

战斗预判逻辑（`keyPressEvent` 中 `Move_Encounter` 分支）：
1. 计算伤害 → 回合数 → 总承受伤害
2. 有眼镜 或 无法取胜 → 弹出详细信息确认对话框
3. 僵持（双方伤害为 0）→ 无法战斗提示
4. 确认战斗 → `fightAt()` → 根据结果处理

---

### UI/ui_MainWindow.h

主窗口的 **UI 布局代码**。三段式布局：
```
[左侧怪物面板 240px] [地图 900x900] [右侧面板 280px]
```

右侧面板：楼层标签 → 属性（HP/ATK/DEF）→ 金币/钥匙 → 背包物品 → 按钮（背包/保存/读取/编辑器/修改器）

左侧怪物面板：QScrollArea 内嵌 QVBoxLayout，标题"本层怪物" + 分隔线 + 怪物条目（图片 + 属性文字）

---

### UI/MapWidget.h / MapWidget.cpp

**地图渲染组件**（QWidget，约 500 行）。

核心常量：`TILE_SIZE = 60`（每个格 60×60 像素）

| 功能 | 说明 |
|------|------|
| `generatePlaceholders()` | 生成所有占位色块：墙壁(灰)、地板(米黄)、楼梯(↑↓)、门(红蓝绿)、NPC、商店、怪物模板、玩家 |
| `paintEvent()` | 主绘制循环：遍历 `game->map()` → 每个格子根据类型选择 Pixmap → 道具动态渲染形状 → 叠加文字 |
| `loadTileImage()` | 加载指定图块类型的 PNG 替换占位色块 |
| `loadMonsterImage()` | 加载指定怪物的 PNG |
| `loadPlayerImage()` | 加载玩家 PNG |

**道具绘制函数**（静态，各约 20-30 行）：
- `drawKeyShape` — 钥匙（圆把手 + 杆 + 齿）
- `drawCoinShape` — 金币（圆币 + 内圈 + "G"）
- `drawSwordShape` — 武器（竖直剑 + 护手 + 剑柄）
- `drawShieldShape` — 防具（盾牌 + 十字装饰）
- `drawPotionShape` — 生命药（药水瓶 + 高光）
- `drawGlassesShape` — 眼镜（两圆 + 鼻梁 + 镜腿）
- `drawHammerShape` — 破墙锤（锤头 + 木柄）
- `drawUpArrowShape / drawDownArrowShape` — 上楼/下楼器（圆底 + 箭头）
- `drawTempShieldShape` — 护盾（盾形 + "T"）
- `drawPenguinShape` — 企鹅玩偶（简笔企鹅）
- `drawParfaitShape` — 抹茶芭菲（高脚杯 + 分层）
- `drawLuckyCoinShape` — 幸运金币（金币 + 四叶草）

文字叠加：`drawOverlayText()` 在图片底部绘制半透明黑底白字条。

**怪物面板**：屏幕左侧显示本层所有怪物缩略图及属性，由 MainWindow 调用更新。

---

### UI/MapEditor.h / MapEditor.cpp

**地图编辑器**（独立 QWidget 窗口，约 1600 行）。

布局：左侧工具面板(280px) + 右侧地图编辑区(900px) + 顶部工具栏

| 功能模块 | 说明 |
|----------|------|
| 图块按钮 | 13 种图块的彩色按钮网格，点击切换当前画笔 |
| `mousePressEvent` | 左键放置图块 / 右键擦除 / Shift+点击放置玩家起点 |
| 属性面板 | 选中图块后显示对应编辑面板（怪物数值/NPC对话交易/商店售价），`m_populating` 防止信号循环 |
| `saveToPath()` | 导出 `.txt`：MOTA2 头 → 每层 LAYER 块（地图 + 道具 + 怪物 + NPC + 商店 + 楼梯） |
| `onLoad()` | 导入 `.txt` / `.map`，解析各层数据 |
| `onTestPlay()` | 导出临时文件 → 创建 Game → `loadFromFile` → 启动 MainWindow |
| `m_floorPanel` | QScrollArea 内的楼层列表，点击切换编辑楼层，支持添加/删除楼层 |

---

## 资源文件

### resources.qrc

Qt 资源系统清单。将以下内容嵌入 exe：
- `images/` 下 28 个 PNG 文件（地砖/门/NPC/商店/玩家/18种怪物）
- `map.txt`（默认地图）

所有资源使用 `:/` 前缀访问，例如 `:/images/player.png`、`:/map.txt`。

### CMakeLists.txt

| 配置 | 说明 |
|------|------|
| `CMAKE_CXX_STANDARD 17` | C++17 |
| `CMAKE_AUTOMOC ON` | Qt MOC 自动处理 |
| `CMAKE_AUTORCC ON` | Qt RCC 自动处理（编译 .qrc） |
| `find_package(Qt6 Widgets)` | Qt6 Widgets 模块 |
| `SRC` 列表 | 13 个源文件 + resources.qrc |
| `target_link_libraries(Qt6::Widgets)` | 链接 Qt |

### map.txt

默认地图文件（文本格式，嵌入 exe）。格式：
```
MOTA2              ← 魔塔2.0标记
15 15              ← 地图宽高
<floor_number>     ← 楼层号
<map_grid>         ← 15×15 数字网格（0=空 1=墙 2=地板 ...）
<item_count>       ← 道具数量
<x y 名称 值>      ← 道具行
<monster_count>    ← 怪物数量
<index 名称 hp atk def gold>  ← 怪物行（index = y*width+x）
<npc_count>        ← NPC 数量
... (NPC数据)
<shop_count>       ← 商店数量
... (商店数据)
<stairs: upx upy downx downy>
LAYER              ← 分隔符，重复上述结构
```
