# Brainstorming: Boss Battle CG

**Date Started:** 2026-09-19
**Status:** Done
**Current Phase:** finalizing
**Based On:** 2026-09-11-bandori-mota-art-assets-brainstorm.md
**Final Spec:** docs/superpowers/specs/2026-09-19-boss-battle-cg-design.md
**Last Updated:** 2026-09-19 19:44

## Original User Request

> 大法师添加剧情CG，50层添加剧情CG，每一个boss战（和boss战斗时）前添加剧情CG

---

## Phase A: Alignment Decision Log

### Q1: 设计基础与会话标识
**Options Presented:**
- A: 基于既有角色美术规范，新建 `boss-battle-cg` 设计并统一采用全窗口CG、点击继续后开战
- B: 不复用既有美术规范，重新定义CG画风与交互
**Decision:** A
**Rationale:** 用户确认沿用现有美术规范和推荐的统一Boss战前CG方案。
**Timestamp:** 2026-09-19 19:10

### Q2: Boss战CG覆盖范围
**Options Presented:**
- A: 仅覆盖10、20、24、35、40、49、50层的核心Boss
- B: 覆盖10、15、20、24、32、35、40、48、49、50层全部剧情Boss与精英
- C: 采用B，但排除48层藤都子SP；42层夹击仍使用现有剧情CG，不另算Boss战
**Decision:** C
**Rationale:** 用户明确表示48层不需要，其他列出的Boss都需要战前CG。
**Timestamp:** 2026-09-19 19:14

### Q3: CG播放次数与战斗衔接
**Options Presented:**
- A: 每次碰撞Boss都播放，包含同一存档中的重复尝试
- B: 每个Boss首次交战前播放一次；撤销到播放前或读取较早存档后可再次播放
- C: 整个游戏安装周期只播放一次，不受存档和撤销影响
**Decision:** B
**Rationale:** 用户确认沿用一次性剧情原则，同时要求撤销和旧存档能正确恢复对应剧情状态；CG关闭前锁定移动和战斗。
**Timestamp:** 2026-09-19 19:17

### Q4: CG素材生产方案
**Options Presented:**
- A: 全部Boss复用现有角色立绘和通用背景
- B: 10、32层复用并调整现有剧情CG；15、20、24、35、40、49、50层各生成独立16:9全屏CG
- C: 全部Boss重新生成独立CG，包括已有10、32层素材
**Decision:** B
**Rationale:** 用户选择继续推荐方案；CG采用接近邦多利手游剧情插画的动画赛璐璐风格，图内不写文字，对白由底部Galgame界面显示。
**Timestamp:** 2026-09-19 19:20

### Phase A → B Transition Confirmation [2026-09-19 19:20]
**Alignment Summary (compiled by ds):**
- 基于既有 BanG Dream! 魔塔美术规范扩展Boss战前CG。
- Boss范围为10层八幡海铃、15层大章鱼、20层凑友希那·吸血鬼、24层户山香澄·大法师、32层幼年长崎素世·骑士队长、35层薇欧拉SP·魔龙、40层Boss、49层假魔王、50层长崎素世最终Boss。
- 48层藤都子SP不播放Boss战CG；42层只保留现有夹击剧情CG。
- 每个Boss在同一存档中首次交战前播放一次；撤销到播放前或读取较早存档后可以再次播放。
- CG显示期间锁定移动、鼠标与战斗；玩家点击继续并关闭CG后才执行原本的战斗。
- 10层、32层复用并调整现有剧情CG；15、20、24、35、40、49、50层各生成独立16:9全屏CG。
- CG采用接近邦多利手游剧情插画的动画赛璐璐风格，图片内无文字；对白继续使用底部Galgame对话框。
- 一次性播放状态进入游戏存档和撤销快照，不能使用仅存在于界面内存的临时标记。

**User Confirmation:** ✓ Confirmed

---

## Phase B: Spec Writing Status

- [✓] Initial draft complete (time: 2026-09-19 19:29)
- [✓] Round 1 revision (time: 2026-09-19 19:38)
- [ ] Round 2 revision
- [ ] Round 3 revision
- [✓] Final sign-off (time: 2026-09-19 19:44)

## Phase B Review Progress

> Spec drafts must include ## Acceptance Scenarios (Gherkin) after Design Principles.

**Sample Matching:** No sample index was available in the installed skill package; selected 0 exemplars. Use the six fixed reviewers only.

### Round 1 [✓ complete]

**Dispatched reviewers (6):** architect | red-team | edge-cases | yagni-gatekeeper | bdd-reviewer | tdd-reviewer

**Receipt Status:** architect ✓ | red-team ✓ | edge-cases ✓ | yagni-gatekeeper ✓ | bdd-reviewer ✓ | tdd-reviewer ✓

**Round metadata:** dispatched_count: 6 | successful_receipt_count: 6 | excluded_roles: none

**Findings:**

| ID | Sev | Location | Reviewer | Problem | Arbiter | Status |
|----|-----|----------|----------|---------|---------|--------|
| F1.1 | I | Design Principles / Shared coordinator | architect | Presentation helper was not the actual combat boundary. | KEEP | ✓ FIXED |
| F1.2 | I | Registry / eligibility | architect | UI Boss identity lacked a Game-layer combat eligibility contract. | KEEP | ✓ FIXED |
| F1.3 | B | Floor 49 integration | red-team | Sealed phantom could consume its CG before a real fight. | KEEP | ✓ FIXED |
| F1.4 | B | Mandatory dialog | red-team | Escape/reject could mark the key and start combat early. | KEEP | ✓ FIXED |
| F1.5 | B | Mandatory dialog | edge-cases | Duplicate early-rejection failure. | DEDUP_DISCARDED into F1.4 | DEDUP_DISCARDED |
| F1.6 | I | Floor 32 registry | yagni-gatekeeper | New key duplicated the existing floor 32 pre-fight story key. | KEEP | ✓ FIXED |
| F1.7 | B | Acceptance scenarios | bdd-reviewer | Lower dialogue box requirement had no observable acceptance step. | KEEP | ✓ FIXED |
| F1.8 | I | Testing Strategy | tdd-reviewer | Test-first RED/GREEN execution was not explicit. | KEEP | ✓ FIXED |

**Arbiter Output:**
- counts: raw=8 → dedup=7 → after_filter=7 (B=3, I=4, N=0)
- degradation_check: N/A
- convergence_status: CONTINUE
- arbiter_rationale: Early modal rejection findings were deduplicated; floor 49 eligibility remains distinct from coordinator ownership and Game-layer eligibility boundaries. Per the user's one-round review constraint, all retained findings were fixed directly and no second review round is run.

### Appendix (NITs)

- None.

---

## Phase B User Intervention Decisions
