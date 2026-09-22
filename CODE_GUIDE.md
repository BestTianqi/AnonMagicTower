# 魔塔代码指南

本文说明当前代码结构和主要运行流程。项目功能、构建及操作方式见 [README.md](README.md)。

## 总体结构

```text
main.cpp
  └─ MenuWindow
       └─ MainWindow
            ├─ Game
            └─ MapWidget
```

- `Game` 保存权威游戏状态并处理规则。
- `MapWidget` 只负责地图、角色和怪物的绘制与动画。
- `MainWindow` 连接输入、剧情表现、商店、存档、道具栏和 HUD。
- `MenuWindow` 提供新游戏、读档和设置入口。
- 旧地图编辑器已经移除；管理员调试功能位于游戏内修改器。

## 入口与构建

### main.cpp

创建 `QApplication` 和 `MenuWindow`，随后进入 Qt 事件循环。

### CMakeLists.txt

构建四个目标：

- `mota`：游戏程序。
- `mota_classic_tests`：经典地图、道具、机关、剧情和存档测试。
- `mota_motion_tests`：连续移动插值测试。
- `mota_battle_feedback_tests`：战斗提示和 Boss 流程测试。

项目使用 C++17、Qt Widgets、AUTOMOC 和 AUTORCC。UI 布局由仓库中的 `ui_MainWindow.h` 与 `ui_MenuWindow.h` 维护，不使用 AUTOUIC。

## Game 核心层

### FloorData

每层包含：

- `map`：15×15 图块数组。
- `monsters`：坐标到怪物的映射。
- `items`：坐标到道具的映射。
- `npcs`：坐标到 NPC 的映射。
- `shops`：坐标到商店配置的映射。

坐标统一通过 `posKey(x, y)` 转为一维键。

### 地图初始化

`generateClassicTower()` 读取资源 `:/data/classic50_map.txt`，建立经典 50 层基础地图，再应用项目中的主题化机关、剧情和奖励修正。

`loadDefaultMap()` 调用同一初始化流程，因此主菜单新游戏、死亡重开和通关重开不会使用不同地图。

### 移动流程

键盘移动：

```text
MainWindow::keyPressEvent
  → Game::tryMovePlayer
  → 根据 MoveResult 处理战斗、拾取、NPC、商店或楼梯
  → MapWidget 更新显示
```

鼠标移动：

```text
Game::beginTeleportPlayerTo
  → BFS 生成可达路径
  → 必要时 MapWidget::playPlayerPath
  → Game::completeTeleportPlayerTo
  → 到达目标格后执行交互
```

只有路径包含危险或剧情触发点时才逐格播放；普通连通路径直接到达。权威坐标始终是整数格，动画不会改变碰撞规则。

### 战斗与危险

- `bossEncounterStateAt()` 判断 Boss 是否满足挑战条件。
- `fightAt()` 处理回合伤害、先攻、特殊武器、奖励和战后机关。
- `approachHazardsAt()` 是巫师靠近伤害和魔法警卫夹击的唯一计算入口。
- `applyApproachHazardsAt()` 写入实际生命变化。
- `previewApproachHazardDamageAt()` 为地图伤害数字提供只读预览。
- 神圣盾会免疫上述场地伤害。

### 机关与剧情状态

机关门由 `openMechanismDoorsIfReady()` 和楼层专属事件处理。剧情只触发一次的键保存在 `m_storyOnceKeys` 中，并随存档写入。

32、33、35、38、41、42、48、49 和 50 层等专属流程各有明确状态字段，避免通过画面状态反推剧情进度。

### 存档

`saveToFile()` 和 `loadFromFile()` 保存或恢复：

- 玩家属性、钥匙和道具。
- 当前楼层与坐标。
- 所有楼层的地图、怪物、道具、NPC 和商店。
- 已访问楼层。
- 机关、Boss、NPC 奖励和一次性剧情状态。

读取器保留必要的旧存档兼容分支；新游戏数据只使用当前格式。

## Entities 实体层

### Player

`Player` 保存属性、钥匙、装备效果和特殊道具状态。

- 钥匙接口：`AddKey / HasKey / UseKey / KeyCount`
- 道具接口：`AddItem / Inventory / UseItem / GetItem`

道具栏是背包数据的固定位置视图，不存在独立的弹窗背包状态。

### Items

`Item` 是道具基类。派生类分为：

- 拾取即生效：宝石、血瓶、钥匙、普通属性奖励。
- 固定栏被动道具：武器、防具、怪物手册、幸运金币等。
- 固定栏主动道具：上楼器、下楼器、炸弹、大黄门钥匙、对称飞行器等。
- 可重复使用道具：爱音手机、冷静雪花徽章等。

`classicItemTierForFloor()` 计算每十层的宝石和血瓶档位；`classicShopOfferForFloor()` 计算商店属性与全局价格。

### Monster 与 MonsterDB

`Monster` 保存名称、HP、ATK、DEF 和金币。

`MonsterDB::all()` 按经典怪物编号顺序返回 34 种主题怪物。图片编号与这个顺序一致：

```text
images/characters/monsters/monster_01.png
...
images/characters/monsters/monster_34.png
```

### NPC

`NPC` 保存对白、一次性奖励和一次性交易状态。UI 负责 Galgame 对话表现，实体负责奖励与交易完成状态。

## UI 表现层

### MenuWindow

提供新游戏、读取存档和设置。游戏窗口作为同一个顶层窗口的子页面显示，关闭游戏页后恢复菜单。

### MainWindow

主要职责：

- 处理键盘和鼠标交互结果。
- 显示 Galgame 对话与全屏剧情 CG。
- 驱动 Boss 战前表现。
- 管理商店、NPC、修改器、普通存档和即时存档。
- 保存多步撤销快照。
- 刷新怪物手册和固定道具栏。
- 统一处理失败、通关和重新开始流程。

### MapWidget

主要职责：

- 按 60×60 逻辑格绘制地图。
- 加载图块、怪物、NPC、道具和玩家素材。
- 切分 4×4 或 8×8 玩家精灵表。
- 播放玩家逐格移动和剧情怪物移动。
- 在格子上显示预计战斗损血、攻击临界值和场地危险伤害。

## 资源

`resources.qrc` 是运行资源清单。主要目录：

- `data/classic50_map.txt`：唯一经典地图数据源。
- `images/runtime/tiles/`：运行时图块。
- `images/runtime/items/`：道具图标。
- `images/runtime/cg/`：剧情与 Boss CG。
- `images/characters/monsters/`：怪物小人。
- `images/characters/portraits/`：Galgame 立绘。
- `images/characters/player_outfits/`：玩家行走精灵表。

代码通过 `:/...` 资源路径访问这些文件。

## 测试原则

修改下列内容后至少运行三组测试：

```powershell
cmake --build build --parallel 4
.\build\mota_classic_tests.exe
.\build\mota_motion_tests.exe
.\build\mota_battle_feedback_tests.exe
```

提交前还应执行：

```powershell
git diff --check
```

所有命令通过后，才能确认结构调整没有破坏当前机制。
