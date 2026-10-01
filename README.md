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
- 爱音／素世原创同人主线，共 31 个场景；含表情切换、米歇尔伏笔、营救进度对白与捷径终幕兼容。详见 [主线修订说明](docs/story-polish-20260930.md)。
- 10 个普通存档槽、即时存档、即时读档和多步撤销。
- 爱音手机楼层传送、上楼器、下楼器、对称飞行器等经典特殊道具。
- 游戏内修改器与管理员坐标传送，便于调试楼层事件。
- 主菜单内置 3×3 至 8×8 数字华容道，支持鼠标和方向键操作。
- 主菜单内置角色合奏 2048：角色图片卡片、滑动合并动画、撤销和本地最高分；支持方向键、WASD、鼠标滑动与屏幕方向按钮，达到 2048 后可继续挑战 4096、8192 等更高数字。
- 魔塔、数字华容道和 2048 均有原创操作音效；主菜单及游戏内设置共享音效开关和音量。
- 数字华容道和 2048 可选择循环播放《春日影》或《KiLLKiSS》的乐谱合成 8-bit 背景音乐；音乐有独立开关和音量。

旧地图编辑器已经移除。经典塔数据统一由 `data/classic50_map.txt` 提供。

## 环境要求

- CMake 3.16 或更高版本
- Qt 6 Widgets 与 Multimedia
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

项目包含经典机制、移动表现、战斗反馈、主窗口输入、剧情脚本、剧情语音、华容道和 2048 等测试：

```powershell
.\build\mota_classic_tests.exe
.\build\mota_motion_tests.exe
.\build\mota_battle_feedback_tests.exe
.\build\mota_mainwindow_input_tests.exe --story-only
.\build\mota_story_script_tests.exe
.\build\mota_story_voice_tests.exe
.\build\mota_puzzle_tests.exe
.\build\mota_2048_tests.exe
```

各程序以退出码 0 表示通过；主窗口的 `--story-only` 模式不操作游戏存档。

## 操作

| 操作 | 功能 |
| --- | --- |
| 方向键 | 按格移动，画面播放连续行走动画 |
| 鼠标点击地图 | 前往当前连通区域；途经危险或剧情点时逐格播放 |
| 点击道具图标 | 查看或直接使用道具 |
| F5 | 写入即时存档 |
| F9 | 读取即时存档 |
| Ctrl+Z | 撤销最近一次操作，可连续撤销 |

游戏内设置可以切换移动动画、战斗提示与音效，调整音效音量，也可以退出游戏。
设置中的“返回主菜单”可回到启动页，返回前请先保存进度。主菜单“读取存档”与游戏内共用即时存档和 10 个普通存档槽位。

音效源文件位于 `Audio/sfx/`，由 `tools/generate_sfx.py` 可重复生成，无需下载第三方音频。
两首小游戏背景音乐位于 `Audio/music/haruhikage_8bit.wav` 和 `Audio/music/killkiss_8bit.wav`。它们分别根据用户提供的六页钢琴乐谱，提取 135、146 小节音符和节奏，再用脉冲波与三角波从零合成；没有采样或处理原曲录音。《KiLLKiSS》按谱面前奏渐快和第 13 小节的速度标记演奏。音符数据保存在同目录的 `*_score_notes.json` 文件中。可用 `python tools/render_score_chiptune.py --notes Audio/music/killkiss_score_notes.json --output Audio/music/killkiss_8bit.wav` 重新渲染。主菜单、游戏内设置以及两个小游戏界面均可选择曲目，切换后立即播放并记住选择。

### 剧情语音

爱音、素世和旁白分别使用各自的 GPT-SoVITS 底音，并通过 `aiyi.pth`／`aiyi.index`、`sushi.pth`／`sushi.index`、`deng.pth`／`deng.index` 做 RVC 转换；旁白由高松灯声线配音。RVC 音高保持原调（**0 半音**），索引混合率为 0.65。成品音频位于 `Audio/voice/`；剧情翻页时自动播放对应录音，翻页或关闭剧情会停止上一条。语音使用游戏现有音效开关与音量。其他角色目前仍以文字呈现。

需要重新生成录音时，在项目根目录分别执行：

```powershell
& 'D:\GPT_SOVITS\GPT-SoVITS-v3lora-20250228\runtime\python.exe' tools/generate_story_tts.py --speaker aiyi --force
& 'D:\GPT_SOVITS\GPT-SoVITS-v3lora-20250228\runtime\python.exe' tools/generate_story_tts.py --speaker sushi --force
& 'D:\GPT_SOVITS\GPT-SoVITS-v3lora-20250228\runtime\python.exe' tools/generate_story_tts.py --speaker deng --force
& 'D:\RVC\RVC20240604Nvidia\runtime\python.exe' tools/generate_story_voices.py --speaker aiyi --force
& 'D:\RVC\RVC20240604Nvidia\runtime\python.exe' tools/generate_story_voices.py --speaker sushi --force
& 'D:\RVC\RVC20240604Nvidia\runtime\python.exe' tools/generate_story_voices.py --speaker deng --force
```

生成脚本读取 `UI/StoryScript.h` 中的对白，并补充主窗口中的固定事件旁白；GPT-SoVITS 的底音缓存在 `build/story_voice_tmp/gpt_base/`，RVC 模型及索引分别从 `D:\RVC\爱音\爱音\`、`D:\RVC\素世\素世\`、`D:\RVC\灯\灯\` 读取。`--force` 覆盖已有的同名录音。构建时将已生成的 WAV 复制到 `build/Audio/voice/`。运行游戏只需要这些 WAV，不需要启动两套语音引擎；分发时须保留 exe 旁的 `Audio/voice/` 目录。

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
├── tests/                      # 七组回归测试
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
## 小游戏暂停、存档与图片

数字华容道和角色合奏 2048 共用窗口内的暂停／存取工具栏，但进度各自独立：

- 空格、P 或 Esc 暂停／继续；切到其他窗口也自动暂停。暂停冻结棋盘、撤销、计时和背景音乐，返回后需手动继续。
- 每局自动保存，返回菜单和退出程序时也保存。重新进入会恢复上次进度，先处于暂停状态。
- 每个小游戏有 3 个手动存档位；Ctrl+S 保存当前所选存档位，Ctrl+L 读取。读档保留棋盘、步数、计时和完整撤销记录；2048 还保留得分及随机生成状态。
- 华容道保留最初的爱音／素世夕照合影，另加入用户指定 `bandori-library` 中的 20 张不同编号卡面，共 21 张内置图片，包含 **#947 冰川日菜（特训）**。以 MyGO 成员卡面为主，选图列表显示卡号、人物和初始／特训版本；清单及原文件来源见 `data/puzzle_gallery.json`，图片通过 `puzzle_cards.qrc` 随程序打包。
- 原先额外加入的封面、舞台等 5 张图片已从华容道图库移除；读取引用这些图片的旧存档时换回夕照合影，保留棋盘进度。切图不重置棋盘，也可导入 PNG/JPG/WebP/BMP，自动裁成正方形。
- 自定义图片直接保存在小游戏存档内，原文件移动后仍可继续。存档位互不覆盖自动存档，且与魔塔的存档独立。
- 存档位显示「空／已保存」，保存失败或损坏存档会在工具栏提示，不以空局覆盖当前进度。存档位于应用数据目录下的 `minigames/`。

## 下落式音游：音符之间

主菜单进入「音符之间 · 下落音游」。四轨使用 **D / F / J / K**，也可点击轨道；空格暂停／继续，失去窗口焦点自动暂停（包括准备倒计时）。开场有 2 秒准备，恢复有 1 秒准备；「结束选曲」可中途切歌且不会保存未完成成绩。支持三档难度、独立的五档音符流速、连击、Perfect/Great/Good/Miss 判定、EARLY/LATE 毫秒提示、准确率、结算判定分布、分曲目难度最高分、音量及 ±300ms 延迟校准。每档难度均覆盖四条轨道，同轨音符按先后顺序判定。

六首用户提供的歌曲均已生成短曲和基于音频起音点的初版谱面。当前主歌／副歌截取时间为**待校听的初版边界**，不代表已经人工确认精确乐段：碧天伴走 00:13–01:48、春日影 00:17–01:57、栞 00:13–01:56、迷星叫 00:15–01:48、壱雫空 00:12–01:35、影色舞 00:10–01:41。时间配置在 `tools/prepare_rhythm_music.py`；素材在 `Audio/rhythm/`，构建时复制到程序旁同名目录。分发时须保留这个目录，仅复制 exe 不包含音游歌曲。
