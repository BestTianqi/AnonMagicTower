# Brainstorming: Continuous Player Movement

**Date Started:** 2026-09-12
**Status:** Done
**Current Phase:** finalizing
**Based On:** 2026-09-11-bandori-mota-art-assets-brainstorm.md
**Final Spec:** docs/superpowers/specs/2026-09-12-continuous-player-movement-design.md
**Last Updated:** 2026-09-12

## Original User Request

> 修改人物移动逻辑，不是从格子间跳过，而要做成连续移动，应用移动的帧动画

## Phase A: Alignment Decision Log

### Q1: 移动表现与判定方式
**Options Presented:**
- A: 方向键按住后以 180px/s 平滑插值移动，碰撞和战斗仍在格点边界判定
- B: 只增加帧动画，角色位置仍瞬移到下一个格子
- C: 纯像素自由移动并重写整套碰撞地图
**Decision:** A
**Rationale:** 用户确认采用连续移动并应用移动帧动画；保留原版魔塔格点规则可避免战斗、门和道具逻辑漂移。
**Timestamp:** 2026-09-12

### Q2: 动画资源与兼容策略
**Options Presented:**
- A: 使用现有 240×240 Anon 4×4 精灵表，静态 PNG 作为回退
- B: 重新制作 8 方向动画资源
- C: 只用程序绘制人物
**Decision:** A
**Rationale:** 项目已有符合规范的 pilot 精灵表；按方向取四帧循环，资源缺失时回退现有静态图。
**Timestamp:** 2026-09-12

### Phase A → B Transition Confirmation
**Alignment Summary:**
- 玩家位置在屏幕像素坐标中平滑插值，速度默认 180px/s。
- 输入按住时持续移动，松开后停止；单次移动仍以目标格点为单位。
- 碰撞、拾取、开门、战斗和楼梯逻辑在到达目标格点时执行，保持原版规则。
- 使用 Anon 240×240 精灵表，行顺序 down/left/right/up，每行 4 帧；移动时循环，静止时显示每行第 2 帧。
- 未加载精灵表时回退现有 `images/player.png`。

**User Confirmation:** ✓ Confirmed

## Phase B: Spec Writing Status

- [✓] Initial draft complete
- [✓] One review round complete
- [✓] Final sign-off
