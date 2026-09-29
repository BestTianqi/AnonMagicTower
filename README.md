# MYGO!!!!! × Ave Mujica：梦限大魔塔

基于 Qt 6 与 C++17 制作的二维魔塔 RPG。游戏以经典 50 层魔塔的地图、数值和机关流程为基础，角色与视觉主题替换为 BanG Dream!、MyGO!!!!!、Ave Mujica 和梦限大 Mewtype。

## 当前功能

- 0–50 层经典塔流程，并包含 44 层异空间规则。
- 34 种主题怪物，数值与经典怪物编号对应。
- 键盘逐格移动、连续行走动画以及可达区域鼠标移动。
- 红、蓝、黄门，独立花门机关，暗墙和楼层专属陷阱。
- 巫师靠近伤害、魔法警卫夹击、Boss 奖励和特殊楼层事件。
- 固定道具栏：每件装备和特殊道具有固定位置，点击图标查看或使用。
- 原版风格商店、NPC 一次性交易及 Galgame 对话界面。
- 剧情 CG、角色立绘、角色行走图和主题化地图素材。
- 10 个普通存档槽、即时存档、即时读档和多步撤销。
- 爱音手机楼层传送、上楼器、下楼器、对称飞行器等经典特殊道具。
- 游戏内修改器与管理员坐标传送，便于调试楼层事件。
- 主菜单内置 3×3 至 8×8 数字华容道，支持鼠标和方向键操作。

旧地图编辑器已经移除。经典塔数据统一由 `data/classic50_map.txt` 提供。

## 环境要求

- CMake 3.16 或更高版本
- Qt 6 Widgets
- 支持 C++17 的编译器
- Windows 推荐 MinGW 64-bit；MSVC、GCC 和 Clang 也可使用

## 构建

PowerShell 与 MinGW 示例：

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="D:\QT2\6.5.3\mingw_64"
cmake --build build --parallel 4
```

生成的程序位于：

```text
build/mota.exe
```

如果已经配置过构建目录，只需执行：

```powershell
cmake --build build --parallel 4
```

## 测试

项目包含经典机制、移动表现、战斗反馈、主窗口输入、剧情脚本和华容道六组测试：

```powershell
.\build\mota_classic_tests.exe
.\build\mota_motion_tests.exe
.\build\mota_battle_feedback_tests.exe
.\build\mota_mainwindow_input_tests.exe
.\build\mota_story_script_tests.exe
.\build\mota_puzzle_tests.exe
```

六项程序均以退出码 0 表示通过。

## 操作

| 操作 | 功能 |
| --- | --- |
| 方向键 | 按格移动，画面播放连续行走动画 |
| 鼠标点击地图 | 前往当前连通区域；途经危险或剧情点时逐格播放 |
| 点击道具图标 | 查看或直接使用道具 |
| F5 | 写入即时存档 |
| F9 | 读取即时存档 |
| Ctrl+Z | 撤销最近一次操作，可连续撤销 |

游戏内设置可以切换移动动画和战斗提示，也可以退出游戏。

## 主要机制

### 地图与战斗

逻辑位置始终按 15×15 格计算，动画只影响显示，不改变碰撞判定。怪物手册按当前楼层去重显示怪物，并给出预计损血和降低损血所需的攻击临界值。

### 道具

红蓝宝石和血瓶按照每 10 层一个数值档位计算。武器、防具和特殊道具进入固定道具栏；普通属性道具拾取后立即生效。可重复使用道具不会因使用而从栏位消失。

### 商店与 NPC

属性商店共享全局购买次数，价格按经典二阶等差序列增长。剧情 NPC、信息 NPC 和交易 NPC 使用统一的 Galgame 对话表现；一次性交易会随存档保存完成状态。

### 存档与撤销

普通存档、即时存档和撤销快照均保存完整玩家属性、背包、楼层状态、机关状态、剧情标记和已访问楼层。旧存档兼容字段仍由读取器处理。

## 项目结构

```text
mota/
├── main.cpp
├── CMakeLists.txt
├── resources.qrc
├── data/
│   └── classic50_map.txt       # 经典塔唯一地图数据源
├── Game/
│   ├── Game.h
│   ├── Game.cpp                # 地图、移动、战斗、机关和存档
│   └── BossEncounter.h
├── Entities/
│   ├── Player.h/.cpp
│   ├── Monster.h/.cpp
│   ├── MonsterDB.h
│   ├── Items.h/.cpp
│   └── NPC.h/.cpp
├── UI/
│   ├── MainWindow.h/.cpp       # HUD、剧情、商店、存档和道具栏
│   ├── MapWidget.h/.cpp        # 地图、人物和怪物动画渲染
│   ├── MenuWindow.h/.cpp       # 启动菜单
│   ├── ui_MainWindow.h
│   └── ui_MenuWindow.h
├── images/                     # 角色、怪物、道具、图块、背景和 CG
├── tests/                      # 六组回归测试
├── tools/                      # 素材验证及规范化脚本
└── docs/                       # 数值映射、素材规范和设计记录
```

## 素材约定

- 地图格逻辑尺寸为 60×60。
- 玩家行走图支持 8×8、共 64 帧的透明 PNG 精灵表。
- 怪物素材位于 `images/characters/monsters/`。
- Galgame 立绘位于 `images/characters/portraits/`。
- 游戏运行素材通过 `resources.qrc` 嵌入程序。

更详细的经典怪物和道具映射见 [docs/classic50-data-mapping.md](docs/classic50-data-mapping.md)。

## 许可

本项目仅供学习、交流和非商业同人创作使用。角色及相关设定的权利归原权利方所有。
