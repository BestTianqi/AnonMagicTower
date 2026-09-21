# BanG Dream! × 50层魔塔道具图标重绘

## Problem

游戏目前有 57 个运行时道具图标，来源与画风不完全一致。需要在不改变文件名、道具含义或机制的前提下，制作一套同时具备经典魔塔辨识度和 BanG Dream 乐队舞台特色的统一图标。

## Goals

- 重绘 `images/runtime/items` 中全部 57 个 PNG。
- 每个图标保持 60×60、透明背景和清晰轮廓。
- 钥匙、宝石、药水、武器、防具、楼层器等保持原版魔塔的即时辨识度。
- 用乐器、拨片、麦克风、音符、舞台灯、成员配色和演出道具表达 BanG Dream 主题。
- 先交付完整候选套装与对比图，确认整套一致后再统一替换运行时文件。

## Non-Goals

- 不修改道具逻辑、数值、名称、存档字段或地图配置。
- 不增加或删除道具类型。
- 不使用文字标签、人物头像、地板底图或统一方框边框。
- 不在候选生成阶段修改 `resources.qrc`。

## Design Principles

1. **用途优先。** 玩家必须先看出钥匙、药水、宝石或特殊工具，再感受到乐队主题。
2. **无框透明。** 图标只包含道具本体，不含卡片框、地板、投影或场景。
3. **统一材质。** 金属用深蓝黑描边与银色高光；魔法用红、蓝、金舞台灯色；布料和票券使用高饱和演出色。
4. **小尺寸清晰。** 主体占 46–54 像素，四周至少保留 2 像素透明保护区。
5. **一一对应。** 候选输出路径与现有 57 个文件名完全一致，不允许语义错位。

## Acceptance Scenarios

```gherkin
Feature: 完整候选套装
  Scenario: 57个运行时道具均有候选图标
    Given 当前 images/runtime/items 中的 PNG 文件列表
    When 候选套装生成完成
    Then 每个源文件都有一个相对于 images/runtime/items 完整路径相同的候选 PNG
    And mygo 等子目录结构被原样保留
    And 递归相对路径集合恰好为57项且没有额外、缺失或重复项

Feature: 图标技术约束
  Scenario: 每个候选图标可直接用于游戏
    Given 任意候选道具图标
    When 检查尺寸和透明通道
    Then 图像尺寸为 60×60
    And 四角像素完全透明
    And 图标不包含方形边框、地板或文字

Feature: 视觉语义
  Scenario: 所有类别同时表达魔塔用途与乐队主题
    Given 钥匙、药水、宝石、武器、防具、楼层器、机关工具和特殊道具候选
    When 评审者在100%缩放的60×60尺寸逐项对照manifest中的语义标签
    Then 57项的主体轮廓均连续且关键部件可区分
    And 57项均能在不读取文件名的情况下与各自manifest中的精确semantic_label相符
    And 同一用途类别内机制、方向、大小或颜色含义不同的道具均可相互区分
    And 57项均至少有一个可指出的乐器、音符、舞台灯、演出配色或乐队配件特征
    And 任一项失败都将该项标记为rejected并阻止整套批准

Feature: 安全替换
  Scenario: 候选生成不影响当前游戏
    Given 候选套装尚未整体验收
    When 检查 resources.qrc 和 images/runtime/items
    Then 当前运行时路径保持不变
    And 游戏不会引用候选目录

  Scenario: 完整套装通过批准门禁
    Given 57项均通过自动验证和逐项视觉清单
    And comparison_all.png 以现用图标在左、同路径候选在右的方式覆盖全部57项
    When 用户对完整套装作出approved决定
    Then manifest顶层status为approved
    And 后续任务才允许一次性替换运行时文件

  Scenario: 未批准套装不能替换运行时文件
    Given manifest顶层status不是approved
    When 尝试替换任意运行时道具文件
    Then 替换操作被阻止
    And 所有运行时道具文件保持不变
```

## Design

### Visual Language

- 视角：轻微俯视的正面道具图标，保持经典 RPG 物品栏阅读方式。
- 轮廓：2–3 像素深色彩色轮廓，不使用纯黑粗框。
- 光照：左上方舞台聚光灯，高光集中且不模糊。
- 色彩：红、蓝、金对应魔塔关键色；粉、紫、青作为 BanG Dream 舞台辅色。
- 透明：道具外区域为透明，禁止光晕触及画布边缘。

### Asset Groups

1. 钥匙、宝石、药水和基础宝物。
2. 武器、防具、楼层器与塔内机关工具。
3. MyGO、Ave Mujica 与梦限大成员/乐队增益道具。
4. 剧情、商店和终局特殊道具。

### Manifest Contract

`manifest.json` 顶层包含 `version`、`status`、`source_root`、`candidate_root` 和 `items`。`status` 是唯一的整套批准字段，允许值为 `pending | approved | rejected`；初始为 `pending`，只有57项自动验证与视觉清单全通过且用户批准整套后才能变成 `approved`，任一候选变化或复验失败都恢复为 `pending`。每个 `items` 记录必须包含：

- `relative_path`：相对于 `images/runtime/items` 的规范化完整路径，作为唯一标识。
- `semantic_label`、`asset_group`：道具含义与分组。
- `validation_status`：`pending | passed | failed`。
- `review_status`：`pending | approved | rejected`。

源与候选实际路径分别由顶层根目录与 `relative_path` 推导。批次、单元格、尝试编号、处理状态与错误只记录在唯一运行目录的临时 `run-manifest.json` 中，不进入永久清单。任何失败、重复路径、缺失单元格或未验证项都会阻止对比图生成和整套批准。

### Delivery Layout

```text
images/candidates/item_redesign_20260921/
  items/                 # 57个60×60同名候选图标
  comparison_all.png     # 现用/候选对照
  manifest.json          # 文件映射、分组、生成状态
```

生成板和参考板只作为 `tmp/item-redesign-20260921/` 中的可清理中间文件，不属于永久交付。

## Implementation Phases

1. 在写入候选前固定递归相对路径快照，验证恰好57个唯一、可解码的源PNG，否则立即中止且不写候选。
2. 每次运行使用唯一 `run_id` 临时目录和排他锁；使用在线图像生成服务按组生成无边框图标板。每组尝试写入新的attempt目录，最多重试3次；重试不读取旧attempt产物。耗尽后在临时运行清单中标记failed并停止，不生成对比图。
3. 使用不与该组主体配色冲突的纯色背景；仅从画布边缘连通区域生成alpha蒙版，保留主体内部同色像素，再逐格裁切和缩放为60×60。
4. 逐项核对文件语义、尺寸、透明边界和缺失项。
5. 生成全套对比图；本阶段不替换运行时资源。

## Testing Strategy

- 新增并运行：

  ```powershell
  powershell -NoProfile -File tools/verify_item_candidates.ps1 -Source images/runtime/items -Candidate images/candidates/item_redesign_20260921/items -Manifest images/candidates/item_redesign_20260921/manifest.json -Resources resources.qrc
  ```

- 成功必须退出0并输出 `PASS: 57/57 item candidates`；失败必须非零并指出相对路径和失败约束。
- 验证器在任何候选写入前检查57个唯一、可解码源PNG；随后递归比较完整相对路径集合，并检查全部候选为60×60 RGBA、主体非空、四角透明、两像素保护区、无候选QRC引用。
- 自动检查后在100%缩放下逐项并排查看现用图标、候选图标和manifest语义；57项逐一记录用途可辨认、关键轮廓清晰、至少一个乐队/舞台特征、无文字/方框/地板/投影、无色边/孔洞/裁切/背景残留，要求57项全通过。
- `comparison_all.png` 只在自动检查和57项视觉清单全部通过后生成；用户整套批准之前不得替换任何运行时文件。
- 实现验证器前先建立预期失败夹具，至少覆盖：相对路径错位、重复/缺失文件、错误尺寸、非透明边界、候选QRC引用、未批准替换和失败批次；先确认夹具失败，再实现最小验证逻辑并重跑到全部通过。

## File Inventory

- 新增候选目录及57个候选PNG。
- 新增清单和对比图；生成板与参考板仅保存在临时目录并可在验证后清理。
- 不修改 `images/runtime/items/*` 与 `resources.qrc`。

## Out of Scope

- 将候选正式替换进游戏。
- 重写道具面板或道具机制。
- 新增动画道具或多帧特效。
