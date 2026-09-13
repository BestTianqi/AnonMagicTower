# NPC 角色小人

本项目在地图和 NPC 对话中使用两张 60×60 RGBA 小人：

| 角色 | 用途 | 资源 | 参考来源 |
| --- | --- | --- | --- |
| 月岛麻里奈 | 普通 NPC、引导/剧情对话 | `images/characters/portraits/marina.png` | [BanG Dream! ガルパ☆ピコ 官方 STAFF&CAST](https://anime.bang-dream.com/pico/staff_cast/) |
| 真次凛々子 | 商店 NPC、交易对话 | `images/characters/portraits/ririko.png` | [BanG Dream! 第1期 LIVE HOUSE SPACE 官方角色页](https://anime.bang-dream.com/1st/character/live-house-space/) |
| 米歇尔 | 所有“小偷” NPC 的地图形象与对话头像 | `images/characters/portraits/michelle.png` | [BanG Dream! 官方米歇尔角色页](https://bang-dream.com/artist/hello-happy-world/michelle/) |

## 制作约束

- 角色脸型、发型、服装主色和姿态依据官方立绘/角色页，统一为本作 60×60 像素小人风格。
- 米歇尔素材取官方粉色熊立绘的头像/上半身区域，保留粉色熊头、星形眼睛、白色口鼻和舞台帽等识别特征后缩放为 60×60。
- 资源使用透明 RGBA；地图载入采用最近邻缩放，避免 UI 中出现模糊边缘。
- 普通 NPC 与商店图块、地图编辑器预览、NPC 对话头像共用同一文件，避免显示不一致。
