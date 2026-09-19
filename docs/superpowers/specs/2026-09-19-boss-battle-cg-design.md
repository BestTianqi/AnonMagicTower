# Boss Battle Pre-Fight CG

## Problem

Boss encounters currently enter combat through several different code paths. Normal keyboard movement and click-to-move fight in separate handlers, while the floor 32 scripted knight fight calls `Game::fightAt` directly. Existing story CGs cover some floor-entry and trap sequences, but they do not consistently pause immediately before every required Boss battle. The archmage battle and floor 50 finale particularly lack a dedicated pre-fight CG.

## Goals

- Show a full-window story CG immediately before each required Boss battle.
- Cover the Boss encounters associated with floors 10, 15, 20, 24/25, 32, 35, 40, 49, and 50.
- Keep floor 48 excluded and retain floor 42's existing capture CG without treating it as a Boss battle.
- Block player input and defer `Game::fightAt` until the player advances through the modal story page.
- Play each Boss pre-fight CG once per saved timeline, with undo and older saves restoring the earlier playback state.
- Use the existing visual-novel presentation: CG fills the window and dialogue remains in the lower dialogue box.

## Non-Goals

- Changing Boss statistics, rewards, first-strike rules, movement scripts, door conditions, or ending behavior.
- Treating every elite or flower-door guard as a Boss.
- Adding a Boss CG to the floor 48 Fujito Miyako SP encounter.
- Replacing the existing floor 42 capture sequence.
- Adding text or logos directly into generated CG images.

## Design Principles

1. Boss identity is determined by floor and exact monster identity, never by a broad substring alone.
2. Every path into a Boss fight must pass through one shared encounter coordinator.
3. The story dialog is synchronous and modal: combat begins only after it returns.
4. Playback state uses `Game::storyShown`/`markStoryShown`, because those keys already participate in normal saves and undo snapshots.
5. Presentation changes must not alter grid-authoritative movement or combat calculations.
6. Each generated image has one clear focal confrontation, no embedded text, no watermark, and a 16:9 composition safe behind the lower dialogue box.

## Acceptance Scenarios

```gherkin
Feature: Boss pre-fight CG presentation

  Scenario Outline: Required Boss displays its CG before combat
    Given the player is about to fight the required Boss on floor <floor>
    And that Boss CG has not been shown in the current saved timeline
    When the encounter is initiated
    Then a modal full-window CG is displayed
    And the Boss dialogue remains visible in the lower dialogue box
    And the focal confrontation remains visible above the dialogue box
    And combat has not modified either combatant while the CG is visible
    When the player clicks through the final story page
    Then the original battle begins exactly once

    Examples:
      | floor | Boss |
      | 10 | 八幡海铃·骷髅队长 |
      | 15 | 宫永ののかSP·巨型章鱼 |
      | 20 | 凑友希那·吸血鬼 |
      | 25 | 户山香澄·大法师 |
      | 32 | 幼年长崎素世·骑士队长 |
      | 35 | 薇欧拉SP·魔龙 |
      | 40 | 幼年长崎素世·骑士队长 |
      | 49 | 长崎素世·幻影 |
      | 50 | 长崎素世·本体 |

  Scenario: Archmage story terminology remains compatible
    Given the project refers to the archmage sequence as the floor 24 story
    And the classic map places 户山香澄·大法师 on runtime floor 25
    When the player reaches the actual archmage monster
    Then the dedicated archmage CG is shown immediately before that floor 25 fight
    And the existing floor 24 story and passage behavior remain unchanged

  Scenario: Keyboard and click encounters behave identically
    Given a required Boss can be reached through keyboard movement or click-to-move
    When either input method initiates the encounter
    Then both paths select the same CG, dialogue, and once-only key
    And both paths fight only after the CG closes

  Scenario: Scripted floor 32 first strike remains ordered
    Given the floor 32 knight has completed the scripted walk toward Anon
    When the scripted battle is ready to begin
    Then the Boss CG is shown before the knight's first-strike damage is applied
    And closing the CG resumes the existing first-strike and battle sequence

  Scenario: Mandatory Boss CG cannot be dismissed early
    Given an unshown Boss CG is open
    When the player presses Escape or attempts to close the dialog before its final page
    Then the dialog remains open
    And the story key remains uncommitted
    And combat does not begin

  Scenario: Sealed Boss does not consume its CG
    Given the floor 49 phantom is still sealed by one or more magic guards
    When the player collides with the phantom
    Then the existing sealed warning is shown without the Boss CG
    And the Boss CG story key remains absent
    When the guards are cleared and the real phantom encounter begins
    Then the floor 49 Boss CG is shown before combat

  Scenario: Playback state follows undo and saves
    Given a Boss CG has been shown and its story key has been saved
    When the player collides with the surviving Boss again
    Then the CG is not shown again
    When the player undoes to a snapshot captured before the CG or loads an older save
    Then the story key is absent and the CG is shown again on the next encounter

  Scenario: Excluded encounters do not receive Boss CGs
    Given the player fights floor 48 Fujito Miyako SP or participates in the floor 42 capture event
    When those events occur
    Then no new Boss pre-fight CG is inserted
    And the existing floor 42 story CG remains unchanged

  Scenario: Missing image has a safe presentation fallback
    Given a configured Boss CG cannot be loaded
    When the encounter begins
    Then the visual-novel dialog falls back to the current scene and character portraits
    And the player can still continue into the battle
```

## Design

### Boss encounter registry

Add a small UI-side registry keyed by `(floor, exact monster name)`. Each entry contains:

- persistent story key, such as `boss_prebattle_f20_yukina_vampire`;
- CG resource path;
- speaker, dialogue, portrait, and accent color.

Required entries:

| Runtime floor | Exact monster | Story key | CG |
|---|---|---|---|
| 10 | 八幡海铃·骷髅队长 | `boss_prebattle_f10_umiri` | existing `floor10_ambush.png` |
| 15 | 宫永ののかSP·巨型章鱼 | `boss_prebattle_f15_octopus` | `boss15_octopus_prebattle.png` |
| 20 | 凑友希那·吸血鬼 | `boss_prebattle_f20_yukina` | `boss20_yukina_vampire_prebattle.png` |
| 25 | 户山香澄·大法师 | `boss_prebattle_f25_kasumi` | `boss25_kasumi_archmage_prebattle.png` |
| 32 | 幼年长崎素世·骑士队长 | existing `floor32_knight_dialogue` | existing `floor32_child_soyo_charge.png` |
| 35 | 薇欧拉SP·魔龙 | `boss_prebattle_f35_viola_dragon` | `boss35_viola_dragon_prebattle.png` |
| 40 | 幼年长崎素世·骑士队长 | `boss_prebattle_f40_knight` | `boss40_knight_prebattle.png` |
| 49 | 长崎素世·幻影 | `boss_prebattle_f49_phantom` | `boss49_soyo_phantom_prebattle.png` |
| 50 | 长崎素世·本体 | `boss_prebattle_f50_soyo` | `boss50_soyo_final_prebattle.png` |

The floor is part of the registry identity so the identical knight name on floors 32 and 40 receives different CGs. Floor 32 deliberately reuses its existing once-only story key instead of adding parallel state for the same immediately-pre-strike scene.

### Game-layer Boss classification and eligibility

Expose a small read-only Game contract for a monster position that distinguishes:

- non-Boss encounter;
- configured Boss whose prerequisite gate is still blocked;
- configured Boss that is ready to enter combat.

The prerequisite decision shares the same conditions currently enforced by `Game::fightAt`: floor 10 requires its ambush to be triggered, and floor 49 requires the phantom seal to be reduced. Attack/defence strength is not a prerequisite; a ready Boss still shows its CG even if the player's build later produces a normal stalemate warning.

### Shared encounter coordinator

Add one `MainWindow` encounter coordinator that owns the complete sequence: retrieve monster, query Game-layer classification/eligibility, optionally present the Boss CG, apply a scripted pre-fight action when required, call `Game::fightAt` exactly once, then deliver common feedback and post-fight handling.

Keyboard movement, click-to-move, and the floor 32 scripted knight sequence delegate to this coordinator. Callers do not invoke `Game::fightAt` themselves. A small encounter mode selects the existing floor 32 first-strike branch without creating a second general combat implementation.

For a blocked Boss, the coordinator skips and does not mark the CG, then lets the existing gated `Game::fightAt` call produce its established warning. For a ready Boss, it selects the exact registry entry.

### Mandatory pre-fight presentation

Extend the visual-novel dialog with an explicit mandatory-completion result or policy. In mandatory mode, Escape, window close, and dialog rejection cannot complete the dialog. The function returns success only after the player advances the final page.

The coordinator marks the story key only after successful final-page advancement and immediately before continuing into special first-strike logic or `Game::fightAt`. If presentation cannot complete, combat does not begin and the key remains absent. The movement action has already captured its undo snapshot, so undo restores the key to its prior absent state. Existing save/load persistence of `m_storyOnceKeys` requires no new file-format block.

### Encounter integration

Route all three battle paths through the shared encounter coordinator:

1. keyboard movement `Move_Encounter`;
2. click-to-move `handleTeleportResult(... Move_Encounter)`;
3. scripted floor 32 knight sequence, with mandatory CG completion before `resolveFloor32KnightStory()` applies first-strike damage.

No caller retains a separate `Game::fightAt` call. The modal function completes synchronously; the coordinator then applies the ordered first-strike branch if present and invokes the fight once.

### CG presentation and input lock

Reuse `showVisualNovelDialogue`, which already creates a frameless modal dialog sized to the game window. When a valid explicit CG is present, it fills the backdrop and hides the side portraits; the lower dialogue box remains visible without covering the focal confrontation. Mandatory mode ignores Escape and close/reject attempts. Modal execution blocks keyboard movement, click-to-move, inventory, undo, and menu actions until the last page is advanced.

### Art direction

Generate seven new landscape CGs at a consistent 16:9 ratio. Use anime cel-shaded game-story illustration, recognizable costumes and silhouettes consistent with existing project portraits, dramatic tower lighting, and lower-quarter composition safe for the dialogue box. Do not include text, UI, logos, or watermarks.

The existing floor 10 and floor 32 CGs remain the configured images for those fights; no destructive overwrite is required.

### Resource registration

Place final images under `images/runtime/cg/` and register every new file once in `resources.qrc`. Code uses only `:/images/runtime/cg/...` paths. Missing-file behavior remains the existing visual-novel fallback rather than blocking combat.

## Implementation Phases

1. Add focused tests for the exact Boss registry, exclusions, floor 32 key reuse, eligibility gates, mandatory completion, coordinator ordering, and save/load persistence. Run each focused test and observe the intended RED failure before production changes.
2. Implement the Game-layer classification/eligibility contract, shared encounter coordinator, registry, and mandatory presentation result with the minimum code needed for GREEN.
3. Route keyboard, click-to-move, and floor 32 scripted combat through the coordinator and rerun focused tests to GREEN.
4. Generate, inspect, copy, and register seven new 16:9 CG assets.
5. Build the game and test keyboard, click, scripted first strike, undo, save/load, and missing-image fallback.

## Testing Strategy

- For each code behavior, write a focused failing test first, run it, and confirm it fails for the intended missing behavior. Add only the minimum implementation, rerun the focused test to GREEN, then run the relevant regression suite.
- Add a pure Boss descriptor/helper test seam so the nine inclusions and floor 48/42 exclusions can be verified without opening modal dialogs.
- Assert floor 32 and floor 40 produce different story keys despite sharing the same monster name.
- Assert floor 32 reuses `floor32_knight_dialogue` rather than creating a second once-only key.
- Assert sealed floor 49 and untriggered floor 10 return blocked eligibility and do not consume their CG keys; ready states do.
- Assert mandatory dialog rejection/Escape cannot report completion or begin combat.
- Assert coordinator event order is CG completion, optional floor 32 first strike, one `fightAt`, then post-fight handling.
- Save and reload a game after marking a Boss story key and assert it remains shown; load a snapshot from before marking and assert it is absent.
- Retain existing classic combat tests to ensure Boss values, rewards, first strikes, and win conditions do not change.
- Verify every new resource exists, decodes, is registered exactly once, and has a 16:9 landscape aspect ratio.
- Manual smoke both keyboard and click-to-move against one normal Boss, the floor 32 scripted Boss, and the floor 50 final Boss.

## File Inventory

- `UI/MainWindow.h/.cpp` — Boss registry, pre-fight presentation, and three encounter integrations.
- `Game/Game.h/.cpp` — read-only Boss classification and prerequisite eligibility contract; no new persistence format.
- `tests/test_battle_feedback.cpp` or a new focused Boss-CG test — registry and ordering checks.
- `tests/test_classic_items.cpp` — save/load playback-key regression where appropriate.
- `resources.qrc` — seven new CG registrations.
- `images/runtime/cg/boss15_octopus_prebattle.png`
- `images/runtime/cg/boss20_yukina_vampire_prebattle.png`
- `images/runtime/cg/boss25_kasumi_archmage_prebattle.png`
- `images/runtime/cg/boss35_viola_dragon_prebattle.png`
- `images/runtime/cg/boss40_knight_prebattle.png`
- `images/runtime/cg/boss49_soyo_phantom_prebattle.png`
- `images/runtime/cg/boss50_soyo_final_prebattle.png`

## Out of Scope

- CG animation, video playback, voice acting, or music changes.
- New Bosses or changes to floors 42 and 48.
- Repainting unrelated existing CGs or character portrait sets.
- General battle-window redesign.
