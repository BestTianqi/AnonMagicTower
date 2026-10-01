# Brainstorming: 魔塔主线剧情润色

**Date Started:** 2026-09-30
**Status:** Done
**Current Phase:** finalizing
**Final Spec:** ../specs/2026-09-30-main-story-polish-design.md
**Last Updated:** 2026-09-30

## Original User Request

> 优化魔塔主游戏剧情

## Alignment Decision Log

- 沿用用户已确定的爱音／素世感情主线、米歇尔＝素世、幼年素世记忆形象与 50 层揭面；不重建世界观或另开支线。
- 用户此前要求继续实施、不重复确认、只做一轮审核；本轮直接采用限定范围方案，不重新启动逐项审批或多轮评审。
- 在主目录工作，不创建 worktree；保留已有小游戏等未提交改动。
- 对比方向：只润色开场不足以处理后续矛盾；大改分支会影响地图兼容。因此选择润色现有 31 场对白，补进度分支与立绘衔接。
- 采用现有同人 AU 设定而非新增官方剧情断言。减少抽象说教，增加口语、具体行动与配角区别。

## Design / 验收依据

见 `../specs/2026-09-30-main-story-polish-design.md`。既有剧情草稿保留作来源，不将未实现分镜写成新增游戏机制。

## 一轮检查

- 31 场对白及 16 种进度组合通过脚本检查：本地化道具、暗墙提示、无提前揭面、短页长度与资源引用。
- 修正 32 层营救顺序与 50 层捷径衔接；替换原来 42 层并未实现的“爱音关灯”动作描述。
- 为当前角色接入情绪立绘，对话中保留另一侧角色；35 层实际界面验证爱音讲话时仍为米歇尔，截图核对完成。
- `mota_story_script_tests`、`mota_battle_feedback_tests`、`mota_classic_tests` 通过。
- `mota_mainwindow_input_tests --story-only` 通过开场、35 层、50 层揭面及保存／恢复一次性标记测试；临时数据在项目内，不运行 AppData 即时存档测试。
- 原路径 `build/mota.exe` 构建成功；没有修改地图、数值和存档格式，没有提交或推送其他工作。
