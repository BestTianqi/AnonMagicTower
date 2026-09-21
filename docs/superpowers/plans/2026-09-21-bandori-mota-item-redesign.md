# BanG Dream! × 50层魔塔道具图标重绘 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers-deepseek-v4:subagent-driven-development (recommended) or superpowers-deepseek-v4:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 生成与当前57个运行时道具完整相对路径一一对应的60×60透明无边框候选图标，并提供可重复验证和整套对比图。

**Architecture:** 以manifest作为稳定语义映射，临时run-manifest管理分组生成与重试状态。在线图像生成服务输出四组图标板，经过连通背景抠除、逐格裁切和缩放形成候选；验证器阻止缺失、错位、背景残留或候选QRC引用。

**Tech Stack:** PowerShell 7 / System.Drawing、在线 image_gen、JSON manifest、Qt RCC资源清单

**Spec:** `docs/superpowers/specs/2026-09-21-bandori-mota-item-redesign-design.md`

---

### Task 1: 建立57项清单与验证器

**Files:**
- Create: `images/candidates/item_redesign_20260921/manifest.json`
- Create: `tools/verify_item_candidates.ps1`
- Create: `tools/test_item_candidates.ps1`
- Create: `tools/testdata/item-candidates/`

**Acceptance Criteria:**

Feature: 候选技术验证
  Scenario: 完整候选通过验证
    Given 源目录和候选目录具有完全相同的57个递归相对路径
    When 运行验证命令
    Then 命令退出0并输出 PASS: 57/57 item candidates

  Scenario: 非法源集合在写入前失败
    Given 源PNG少于57项、多于57项、相对路径重复或任一文件不可解码
    When 运行源预检
    Then 命令非零退出并报告具体相对路径和约束
    And 不创建或修改候选文件

  Scenario: 未批准套装不能替换
    Given manifest顶层status不是approved
    When 尝试替换运行时道具
    Then 操作非零退出且运行时文件保持不变

- [ ] **Step 1: 固定源清单** — 递归枚举 `images/runtime/items/**/*.png`，生成57条 `relative_path`、精确 `semantic_label`、`asset_group`、pending状态记录。
- [ ] **Step 2: 写失败夹具** — 覆盖缺失项、额外项、子目录错位、61×60尺寸、损坏PNG、非透明角、触及两像素边界、QRC候选引用、源PNG重复/不可解码且不写候选、未批准替换被阻止、批次三次失败后不生成对比图。
- [ ] **Step 3: 运行RED** — `powershell -NoProfile -File tools/test_item_candidates.ps1`，确认每个规则在未实现时按预期失败。
- [ ] **Step 4: 实现验证器与门禁** — 递归比较完整相对路径；解码并检查60×60 RGBA、透明角、非空主体、保护区以及QRC不含候选路径；实现源预检、非approved替换拒绝、failed批次阻止对比图。
- [ ] **Step 5: 运行GREEN** — 测试脚本退出0并打印所有夹具通过。
- [ ] **Step 6: 提交** — `git add tools/verify_item_candidates.ps1 tools/test_item_candidates.ps1 tools/testdata/item-candidates images/candidates/item_redesign_20260921/manifest.json && git commit -m "feat(items): add candidate manifest and verifier"`

### Task 2: 构建分组参考与生成提示

**Files:**
- Create temporarily: `tmp/item-redesign-20260921/<run_id>/reference/*.png`
- Create temporarily: `tmp/item-redesign-20260921/<run_id>/run-manifest.json`
- Create temporarily: `tmp/item-redesign-20260921/item-redesign.lock`

**Acceptance Criteria:**

Feature: 生成批次映射
  Scenario: 每个道具只属于一个确定单元格
    Given manifest中的57个相对路径
    When 创建四个生成批次
    Then 每项恰好映射到一个batch_id和一个唯一行列

- [ ] **Step 1: 分成四组** — 基础消耗品、装备与塔内工具、乐队成员增益、剧情特殊道具；每组不超过16项。
- [ ] **Step 2: 获取排他锁** — 锁覆盖参考、生成、后处理、验证和发布全过程；竞争时立即失败，finally释放；仅在记录PID不存在时清理遗留锁。
- [ ] **Step 3: 生成参考板** — 将现用图标按run-manifest行列绘制到临时4×4参考板，单元格之间保留明显间隔；64格中7格显式标为unused并保持纯背景。
- [ ] **Step 4: 写四份精确提示** — 明确每格语义、颜色差异、无边框、无文字、无地板、纯色抠图背景与统一左上舞台光；unused格禁止生成主体。
- [ ] **Step 5: 自检** — 验证57个路径均映射一次、7个unused不映射，三色钥匙、三档药水、上下楼器和三色票券分别可区分。

### Task 3: 在线生成、抠图和裁切57个候选

**Files:**
- Create: `images/candidates/item_redesign_20260921/items/**/*.png`
- Modify: `images/candidates/item_redesign_20260921/manifest.json`
- Create: `tools/process_item_candidate_sheet.ps1`
- Create temporarily: `tmp/item-redesign-20260921/<run_id>/staging/**/*.png`

**Acceptance Criteria:**

Feature: 图标候选生成
  Scenario: 四个批次全部完成
    Given 四张参考板和四份生成提示
    When 在线生成、抠图和逐格裁切完成
    Then 57个候选均为60×60透明PNG且无方形边框

- [ ] **Step 1: 写后处理脚本** — `process_item_candidate_sheet.ps1 -Sheet <png> -RunManifest <json> -BatchId <id> -Attempt <dir> -StagingRoot <dir>`；成功退出0并记录逐项结果，失败非零并写run-manifest错误。
- [ ] **Step 2: 写入前重新预检** — 立即重新验证57个唯一可解码源PNG；失败时保持正式候选目录与永久manifest不变。
- [ ] **Step 3: 并行生成四组** — 每组单独调用在线 `image_gen`，输出到当前run_id的全新attempt目录，不直接写永久候选。
- [ ] **Step 4: attempt循环** — 校验板布局、unused格、连通背景抠除、裁切、透明边缘与语义；任一后处理或视觉失败都废弃整个该批次attempt并新建attempt，最多3次，耗尽标记failed并停止。
- [ ] **Step 5: 暂存57项** — 后处理脚本仅向当前run_id的staging写入，按run-manifest镜像完整相对目录，适配到60×60并保持两像素透明边界。
- [ ] **Step 6: 原子发布** — 仅当四组和57项自动验证全部通过时，将完整staging一次性切换为候选items并更新永久manifest；失败或中断不得修改永久候选。
- [ ] **Step 7: 更新状态** — 自动通过项写 `validation_status=passed`、`review_status=pending`，顶层status保持pending。
- [ ] **Step 6: 提交** — `git add images/candidates/item_redesign_20260921/items images/candidates/item_redesign_20260921/manifest.json && git commit -m "feat(items): generate Bandori mota item candidates"`

### Task 4: 全套验证与对比交付

**Files:**
- Create: `images/candidates/item_redesign_20260921/comparison_all.png`
- Modify: `images/candidates/item_redesign_20260921/manifest.json`

**Acceptance Criteria:**

Feature: 候选整套交付
  Scenario: 57项候选可供整套选择
    Given 自动验证和逐项视觉清单全部通过
    When 生成全套对比图
    Then 对比图按完整相对路径并排展示57对现用与候选图标
    And 当前运行时文件与resources.qrc保持不变

  Scenario: 逐项视觉语义全部合格
    Given 57项候选在100%缩放下逐项检查
    When 核对精确semantic_label、同类差异、轮廓、乐队特征和禁用元素
    Then 每项均匹配精确含义且同类可区分
    And 每项至少有一个乐队或舞台特征
    And 每项都没有文字、边框、地板、投影、色边、孔洞、裁切或背景残留
    And 任一失败项被标记rejected并阻止对比图与整套批准

  Scenario: 批准门禁
    Given 57项自动与视觉检查全部通过且comparison_all.png完整
    When 用户批准整套
    Then manifest顶层status变为approved
    When manifest顶层status不是approved且尝试替换运行时文件
    Then 替换被阻止且运行时文件保持不变

- [ ] **Step 1: 运行验证器** — 执行spec中的完整命令，要求退出0并输出 `PASS: 57/57 item candidates`。
- [ ] **Step 2: 逐项视觉检查并持久化** — 100%缩放检查精确语义、同类差异、轮廓、乐队特征、透明边缘、无文字/边框/地板/投影；逐项写approved/rejected和失败原因。候选哈希变化或复验失败时对应项及顶层状态恢复pending。
- [ ] **Step 3: 执行视觉硬门禁** — 任一项pending或rejected立即停止，不创建或更新comparison_all.png；失败项回到对应批次的新attempt流程。
- [ ] **Step 4: 生成对比图** — 仅在57项全部approved后，每行显示相对路径、现用图标和候选图标，覆盖57项并保持原生60×60预览。
- [ ] **Step 5: 运行隔离检查** — `rg -n "item_redesign_20260921" resources.qrc UI core main.cpp` 必须无命中；`git diff -- images/runtime/items resources.qrc` 必须为空。
- [ ] **Step 6: 保持待批准** — 顶层 `status=pending`，向用户交付对比图；只有后续用户批准动作才可改为approved，本计划不替换运行时。
- [ ] **Step 7: 提交** — `git add images/candidates/item_redesign_20260921 && git commit -m "docs(items): add complete candidate comparison"`
