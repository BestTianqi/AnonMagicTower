# Boss Battle Pre-Fight CG Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers-deepseek-v4:subagent-driven-development (recommended) or superpowers-deepseek-v4:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Present a persistent, mandatory full-window story CG immediately before each selected Boss battle without changing combat rules.

**Architecture:** `Game/BossEncounter.h` defines the single value-type Boss identity shared by gameplay and presentation. `Game` exposes read-only readiness for prerequisite-gated fights; `UI/BossBattlePresentation.h` maps that identity to CG descriptors. A small callback-injected `UI/BossEncounterFlow.h` makes the exact presentation → mark → optional first strike → fight → post-fight order independently testable, while `MainWindow` supplies the real collaborators. Existing story keys provide save and undo persistence.

**Tech Stack:** C++17, Qt 6 Widgets, Qt resources, MinGW CMake, assertion-based tests, PNG assets.

**Spec:** `docs/superpowers/specs/2026-09-19-boss-battle-cg-design.md`

---

### Task 1: Add exact Boss registry and Game eligibility contract

**Files:**
- Create: `Game/BossEncounter.h`
- Create: `UI/BossBattlePresentation.h`
- Modify: `Game/Game.h`, `Game/Game.cpp`
- Modify: `tests/test_classic_items.cpp`, `tests/test_battle_feedback.cpp`

**Acceptance Criteria:**

```gherkin
Feature: Boss classification and readiness
  Scenario: Selected Bosses have exact descriptors
    Given the nine approved floor and monster-name pairs
    When the registry is queried
    Then each pair returns its configured story key and CG path
    And Game and UI resolve the same shared Boss identity for all nine pairs
    And floors 42 and 48 return no descriptor

  Scenario: Prerequisite-gated Boss is blocked
    Given floor 49 is sealed or floor 10 ambush is not triggered
    When Game classifies the Boss encounter
    Then it reports blocked rather than ready
```

- [ ] **Step 1: Write registry RED tests** — assert all nine exact pairs, floor 32 key reuse, independent floor 40 key, and 42/48 exclusions.
- [ ] **Step 2: Run RED** — build/run `mota_battle_feedback_tests`; expect failure because the registry header is absent.
- [ ] **Step 3: Implement registry** — add a value-only exact lookup containing story key, CG, speaker, dialogue, portrait and accent.
- [ ] **Step 4: Run registry GREEN immediately** — build/run `mota_battle_feedback_tests`; require all nine shared identities/descriptors and the 42/48 exclusions to pass before starting eligibility work.
- [ ] **Step 5: Write eligibility RED tests** — cover blocked/ready floor 10 and sealed/ready floor 49 using `Game::bossEncounterStateAt`, plus exhaustive agreement with the same nine identities.
- [ ] **Step 6: Run eligibility RED** — build/run `mota_classic_tests`; expect missing enum/method failure.
- [ ] **Step 7: Implement eligibility GREEN** — add `BossEncounterState { NotBoss, Blocked, Ready }`, shared-identity matching and the existing floor 10/49 gates.
- [ ] **Step 8: Run eligibility GREEN** — both focused tests exit 0.
- [ ] **Step 9: Commit** — stage Task 1 hunks; commit `feat(boss-cg): classify boss encounters`.

### Task 2: Add mandatory story completion and unified combat coordination

**Files:**
- Create: `UI/BossEncounterFlow.h`
- Modify: `UI/StoryPresentation.h`
- Modify: `UI/MainWindow.h`, `UI/MainWindow.cpp`
- Modify: `tests/test_battle_feedback.cpp`, `tests/test_classic_items.cpp`
- Modify: `CMakeLists.txt`

**Acceptance Criteria:**

```gherkin
Feature: Mandatory ordered Boss encounter
  Scenario: Escape cannot skip the CG
    Given a mandatory Boss story has not reached its final page
    When Escape, reject or close is requested
    Then presentation remains incomplete
    And combat does not begin

  Scenario: Floor 32 keeps first strike ordering
    Given the scripted knight reached Anon
    When the coordinator runs
    Then CG completion precedes the first strike
    And fightAt is called exactly once afterward

  Scenario: Floor 32 first strike is lethal
    Given the scripted knight reached Anon
    When its first strike reduces HP to zero
    Then game-over handling runs immediately
    And fightAt is never called

  Scenario: Ready gated Boss is reached by either input route
    Given a ready floor 10 or floor 49 Boss
    When keyboard movement or click-to-move reaches it
    Then the CG coordinator runs before any stalemate preview can return
    And the unchanged prerequisite result follows the CG
```

- [ ] **Step 1: Write mandatory-policy RED tests** — ordinary stories remain rejectable, mandatory stories reject early completion, final advance succeeds once.
- [ ] **Step 2: Run RED** — `mota_battle_feedback_tests` fails for the missing policy.
- [ ] **Step 3: Implement story policy** — make `showVisualNovelDialogue` return completion; mandatory `ClickableStoryDialog` ignores Escape/reject/close until final advance.
- [ ] **Step 4: Run mandatory-policy GREEN immediately** — rebuild/run `mota_battle_feedback_tests`; require the policy cycle to pass before writing coordinator tests.
- [ ] **Step 5: Write coordinator RED tests** — add `BossEncounterFlow` tests with injected presentation, mark, first-strike, fight and post-fight callbacks. Require complete ordered traces for normal and floor 32 flows, lethal first strike → game over with zero fights, missing-image fallback → fight once, blocked → no mark/fight, and already-shown → no presentation but fight once. Add the header/tests to the focused CMake target.
- [ ] **Step 6: Run coordinator RED** — focused target must fail before the flow contract exists.
- [ ] **Step 7: Implement coordinator GREEN** — implement the pure flow contract, then make `MainWindow` supply real mandatory CG, story mark, optional first strike, one `fightAt`, feedback, retreat, game-over and game-win collaborators.
- [ ] **Step 8: Write routing RED tests** — prove keyboard and click-to-move both enter the coordinator before ready floor 10/49 preview returns; prove floor 32 uses the same coordinator and its reused key.
- [ ] **Step 9: Run routing RED** — focused/classic tests fail before callers change.
- [ ] **Step 10: Route every path and GREEN** — replace direct Boss fight calls in keyboard encounter, click encounter and scripted floor 32 sequence; coordinator runs before the keyboard stalemate early return. Preserve non-Boss preview, damage, rewards and animation.
- [ ] **Step 11: Persistence regressions** — save a snapshot before marking, complete the CG, then restore/undo and assert it replays; load a synthetic older save without the key and assert replay; assert ordinary reload after the key skips it. Confirm floor 42 trap behavior remains unchanged.
- [ ] **Step 12: Run GREEN/regressions** — classic, battle-feedback and motion tests all exit 0.
- [ ] **Step 13: Commit** — stage Task 2 hunks; commit `feat(boss-cg): coordinate mandatory pre-fight stories`.

### Task 3: Generate and register seven dedicated Boss CGs

**Files:**
- Create: `images/runtime/cg/boss15_octopus_prebattle.png`
- Create: `images/runtime/cg/boss20_yukina_vampire_prebattle.png`
- Create: `images/runtime/cg/boss25_kasumi_archmage_prebattle.png`
- Create: `images/runtime/cg/boss35_viola_dragon_prebattle.png`
- Create: `images/runtime/cg/boss40_knight_prebattle.png`
- Create: `images/runtime/cg/boss49_soyo_phantom_prebattle.png`
- Create: `images/runtime/cg/boss50_soyo_final_prebattle.png`
- Modify: `resources.qrc`

**Acceptance Criteria:**

```gherkin
Feature: Dedicated Boss CG resources
  Scenario: Every CG is loadable and registered
    Given the seven required filenames
    When each image and the resource manifest are inspected
    Then every file is a landscape 16:9 PNG
    And every path occurs exactly once in resources.qrc
    And no image contains text, UI, logo or watermark
```

- [ ] **Step 1: Generate seven scenes** — one built-in image-generation call per scene; consistent anime cel-shaded game-story style, correct character identity/costume, tower confrontation, no text, lower-quarter dialogue-safe composition.
- [ ] **Step 2: Inspect outputs** — validate identity, costume, composition and absence of unwanted text; regenerate only failed scenes with one targeted correction.
- [ ] **Step 3: Copy final PNGs** — store under the exact seven workspace paths without overwriting unrelated CGs.
- [ ] **Step 4: Register resources** — add each path exactly once in the CG section of `resources.qrc`.
- [ ] **Step 5: Validate** — decode files, require nonzero dimensions and aspect ratio within one percent of 16:9, and count exact-one QRC registration.
- [ ] **Step 6: Commit** — stage seven PNGs and QRC; commit `feat(art): add boss battle CGs`.

### Task 4: Full verification and executable

**Files:**
- Verify: all Task 1–3 files
- Build: `build/mota.exe`, or `build/mota_updated.exe` if the old executable is locked

**Acceptance Criteria:**

```gherkin
Feature: Boss CG release verification
  Scenario: Updated game passes automated and startup checks
    Given code and resources are integrated
    When the project is built and tests run
    Then compilation succeeds
    And all three test executables exit successfully
    And the updated game survives a three-second startup smoke test

  Scenario: Every approved story-to-combat sequence is preserved
    Given the nine selected Boss identities, an old save, and an undo snapshot
    When encounters are exercised through keyboard, click-to-move and scripted floor 32 routes
    Then each sequence is CG then mark then optional first strike then battle then result
    And undo or old-save restoration replays the CG
    And floor 42 remains unchanged and floor 48 has no Boss CG
```

- [ ] **Step 1: Structural checks** — run `git diff --check`, exact-one QRC checks and placeholder scan.
- [ ] **Step 2: Full build** — run `cmake --build build -j 2`; if old `mota.exe` is locked, link current objects as `mota_updated.exe` without killing the user's process.
- [ ] **Step 3: All tests** — run classic, motion and battle-feedback executables; require exit 0.
- [ ] **Step 4: Startup smoke** — launch the new executable hidden, ensure it stays alive three seconds, then stop only the smoke process.
- [ ] **Step 5: Coverage audit** — registry contains floors 10,15,20,25,32,35,40,49,50 and excludes 42/48.
- [ ] **Step 6: Ordered-flow audit** — automated event traces cover all nine identities, floor 32 lethal/nonlethal first strike, keyboard/click parity, pre-mark snapshot restoration, old-save replay and invalid-CG fallback.
- [ ] **Step 7: Manual encounter smoke** — in administrator/debug mode exercise one keyboard Boss, one click-to-move Boss, floor 32 arrival/first strike/retreat, and floor 50 final Boss; confirm input stays locked through the final CG page and the battle begins afterward.
- [ ] **Step 8: Terminology/behavior audit** — retain the user-facing “24层大法师剧情” wording while documenting that the runtime map encounter is floor 25; verify floor 42 capture CG remains unchanged.
