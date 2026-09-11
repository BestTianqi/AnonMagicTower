# BanG Dream! Mota Character Art Assets Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers-deepseek-v4:subagent-driven-development (recommended) or superpowers-deepseek-v4:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce and verify twelve canonical-costume, retro chibi pixel-art sprite sheets for the approved MyGO!!!!!, Ave Mujica, and 梦限大みゅーたいぷ character roster, beginning with the two-sheet Chihaya Anon pilot.

**Architecture:** Treat the artwork as additive source assets under `images/characters/`, governed by a versioned reference manifest, style guide, and approval record. A non-runtime PowerShell verifier enforces the 4×4 sheet contract, alpha policy, pilot/full rosters, and pilot hash gate; no Qt renderer or runtime resource files change in this plan.

**Tech Stack:** PNG/RGBA pixel art, OpenAI image generation/editing via the `imagegen` skill, PowerShell 7, .NET `System.Drawing`, JSON-compatible YAML metadata, SHA-256, Git.

**Spec:** `docs/superpowers/specs/2026-09-11-bandori-mota-art-assets-design.md`

---

### Task 1: Establish the versioned reference and style contracts

**Files:**
- Create: `docs/art/bandori-character-assets/v1/reference-manifest.yaml`
- Create: `docs/art/bandori-character-assets/v1/style-guide.md`
- Create: `docs/art/bandori-character-assets/v1/pilot-approval.yaml`

**Acceptance Criteria:**

```gherkin
Feature: Versioned art-production contract
  Scenario: Every approved form has an exact official reference
    Given the version 1 reference manifest
    When its roster entries are inspected
    Then all twelve approved IDs are present exactly once
    And every entry identifies an exact official image or stable asset ID
    And every entry records campaign title, retrieval date, inferred details, and style guide version

  Scenario: The pilot gate starts pending
    Given no pilot PNG exists yet
    When the approval record is inspected
    Then its status is pending
    And both SHA-256 fields and reviewed_at are null
    And accepted_exceptions is empty
```

- [ ] **Step 1: Create the metadata directories** — create `docs/art/bandori-character-assets/v1/` and the image directories from the spec's Delivery Layout; do not modify `resources.qrc`.

- [ ] **Step 2: Resolve official references** — use the `agent-reach` web channel and only official BanG Dream! pages. For MyGO!!!!! use `https://anime.bang-dream.com/mygo-avemujica/` and `https://bang-dream.com/artist/mygo/`; for Uika/Doloris use `https://anime.bang-dream.com/avemujica/character/` and `https://bang-dream.com/artist/avemujica/`; for Arale use `https://bang-dream.com/artist/yumemita/`; for Viola use `https://anime.bang-dream.com/yumemita/character/viola/`. Open each official page, resolve the exact image URL or stable image identifier, and record the campaign/title and retrieval date `2026-09-11`.

- [ ] **Step 3: Write the JSON-compatible YAML manifest** — make the file valid JSON as well as YAML so PowerShell can parse it with `ConvertFrom-Json`. Its top-level keys are `manifest_version`, `style_guide_version`, and `forms`; `forms` contains exactly `anon_casual`, `anon_stage`, `soyo_casual`, `soyo_stage`, `tomori_casual`, `tomori_stage`, `uika_casual`, `doloris_stage`, `arale_casual`, `arale_stage`, `viola_normal`, and `viola_corrupted`. Each form contains the fields and types required by the source spec plus its exact `relative_path` from the Delivery Layout. Use `allowed_translucency: []` unless a later approved effect genuinely needs bounded partial alpha.

- [ ] **Step 4: Write style guide version `1.0.0`** — begin `style-guide.md` with exactly `---`, `style_guide_version: "1.0.0"`, `---` on separate lines. This front matter is the authoritative machine-readable style version; Gate compares it with the manifest top-level value and approval record. State: 60×60 frames; 240×240 sheets; rows down/left/right/up; columns left-contact/left-passing/right-contact/right-passing; root anchor `(30,57)`; two-pixel transparent guard on every cell edge; 44–50×52–56 character envelope; one-pixel colored outline; two shade tones plus optional highlight; top-left light; no ground shadow; four equal 150 ms holds in a `1-2-3-4` loop; canonical outfits only.

- [ ] **Step 5: Create the pending approval record** — write JSON-compatible YAML with exactly:

  ```json
  {
    "status": "pending",
    "casual_sha256": null,
    "stage_sha256": null,
    "style_guide_version": "1.0.0",
    "reviewed_at": null,
    "accepted_exceptions": []
  }
  ```

- [ ] **Step 6: Structure self-check** — run `powershell -NoProfile -Command "$m=Get-Content -Raw docs/art/bandori-character-assets/v1/reference-manifest.yaml | ConvertFrom-Json; if($m.forms.psobject.Properties.Count -ne 12){exit 1}; if($m.style_guide_version -ne '1.0.0'){exit 1}"`; expect exit `0`. Confirm all asset references use `https://` official domains and no manifest field contains `TBD`, `TODO`, or an empty string.

- [ ] **Step 7: Commit** — run `git add docs/art/bandori-character-assets/v1 images/characters && git commit -m "feat(art): define character asset contracts"`.

### Task 2: Build the sprite verifier test-first

**Files:**
- Create: `tools/test_character_sprite_verifier.ps1`
- Create: `tools/verify_character_sprites.ps1`
- Create: `tools/normalize_character_sprite.ps1`
- Create: `tools/testdata/character-sprites/`

**Acceptance Criteria:**

```gherkin
Feature: Character sprite verification
  Scenario: Valid pilot sheets pass Pilot mode
    Given exactly two valid Anon sheets and a twelve-form manifest
    When the verifier runs in Pilot mode
    Then it exits zero with PASS: 2 sheets, 32 frames

  Scenario: Invalid image invariants fail precisely
    Given a fixture violating one declared invariant
    When Single mode checks that exact manifest form and relative path
    Then the verifier exits nonzero
    And its output names the file, cell, and failed invariant

  Scenario: Pilot approval hashes control Gate mode
    Given an approved record whose hashes match both pilot sheets
    When Gate mode runs
    Then it exits zero with ALLOW: pilot approved

  Scenario: Generated art is normalized without artistic alteration
    Given a decodable transparent source image
    When the normalizer writes a tracked sprite sheet
    Then nearest-neighbor scaling produces exactly 240 by 240 pixels
    And alpha below 128 becomes 0 while alpha from 128 becomes 255

  Scenario: Interrupted gate invalidation is recoverable
    Given a pilot hash mismatch interrupts invalidation after its marker is written
    When Gate mode is run repeatedly
    Then all non-Anon PNGs end in one quarantine transaction
    And approval is atomically reset to pending
```

- [ ] **Step 1: Scaffold the RED fixture harness** — create `tools/test_character_sprite_verifier.ps1`. It uses `System.Drawing.Bitmap` to generate isolated temporary fixtures and invokes scripts in child PowerShell processes. First assert `RED: verifier missing`, then add the smallest argument-parser implementation and prove malformed arguments/metadata exit `2`. Every later case asserts an exact exit code and stable message substring.

- [ ] **Step 2: Add Single-mode rules one RED/GREEN cycle at a time** — define `[ValidateSet('Single','Pilot','Full','Gate')] [string]$Mode`, mandatory `Root` and `Manifest`, optional `File`, `FormId`, `Approval`, and `StyleGuide`. For `Single`, require both `File` and `FormId`; success prints `PASS: <form_id>, 16 frames`. Add and fail one fixture before implementing each rule: corrupt/truncated PNG with `DECODE`, wrong dimensions, non-`Format32bppArgb`, nontransparent two-pixel guard, undeclared alpha `128`, allowed translucency outside the 56×56 interior, and allowed-alpha values immediately below `min_alpha`, at both inclusive bounds, and immediately above `max_alpha`. Image violations exit `1`; invalid mode arguments or metadata exit `2`.

- [ ] **Step 3: Add exact-path and roster cycles** — require each file's normalized repo-relative path to equal its manifest `relative_path`, not merely its basename. Add a misplaced-valid-file RED fixture before implementing this comparison. Then separately add RED/GREEN cases for unexpected Pilot files, missing Full files, an exact valid Pilot roster, and an exact valid Full roster. Recursively enumerate PNGs while excluding `.quarantine`; Pilot expects the two Anon paths and Full expects all twelve manifest paths. The valid runs print exactly `PASS: 2 sheets, 32 frames` and `PASS: 12 sheets, 192 frames`.

- [ ] **Step 4: Add Gate cycles and atomic invalidation** — RED/GREEN test approved, pending, pilot-hash mismatch, manifest/approval/style-guide version mismatch, an injected interruption after marker creation, recovery, and a repeated Gate invocation. Gate parses the exact `style_guide_version` YAML front matter and compares it with manifest and approval. A match prints `ALLOW: pilot approved` and exits `0`; pending or mismatch prints the exact `BLOCK:` reason and exits `3`. On mismatch, Gate writes an invalidation marker, moves every non-Anon PNG to a single `.quarantine/<timestamp>/` transaction, atomically replaces approval via a same-directory temporary file with status `pending`, current hashes, null `reviewed_at`, and empty exceptions, then removes the marker. At startup, an existing marker resumes the same transaction; retries must be idempotent and must not create duplicate quarantine copies.

- [ ] **Step 5: Add the deterministic normalizer test-first** — add RED cases for wrong source size and partial alpha, then create `tools/normalize_character_sprite.ps1` with mandatory `InputPath` and `OutputPath` and default `AlphaThreshold 128`. Decode with `System.Drawing`, reject fully opaque/no-transparency sources instead of guessing a background, resize to exactly 240×240 using nearest-neighbor sampling, map alpha `<128` to `0` and `>=128` to `255`, save as `Format32bppArgb`, and write through a same-directory temporary file before atomic replacement. Assert pixel mapping at the 127/128 boundary and idempotent output bytes. This script may only normalize dimensions/alpha; all visual corrections remain imagegen edits.

- [ ] **Step 6: Run the complete GREEN suite** — run `powershell -NoProfile -File tools/test_character_sprite_verifier.ps1`; expect `PASS: verifier fixtures` and exit `0`. Run it twice to prove fixtures, normalization, Gate recovery, and Gate resets are idempotent. Also execute a valid `Single`, `Pilot`, `Full`, and approved `Gate` fixture command explicitly.

- [ ] **Step 7: Commit** — run `git add tools/verify_character_sprites.ps1 tools/normalize_character_sprite.ps1 tools/test_character_sprite_verifier.ps1 tools/testdata/character-sprites && git commit -m "test(art): verify character sprite contracts"`.

### Task 3: Produce the Chihaya Anon casual pilot sheet

**Files:**
- Create: `images/characters/pilot/anon_casual.png`
- Modify: `docs/art/bandori-character-assets/v1/reference-manifest.yaml`
- Modify: `docs/art/bandori-character-assets/v1/pilot-approval.yaml`

**Acceptance Criteria:**

```gherkin
Feature: Casual Anon pilot
  Scenario: Anon is recognizable and mechanically valid
    Given the exact official casual reference recorded for anon_casual
    When the finished sheet is viewed at native size and verified in Single mode
    Then Anon's canonical hair, colors, and casual outfit are recognizable
    And every one of the sixteen frames obeys the shared grid, anchor, guard, and alpha rules
```

- [ ] **Step 1: Prepare references** — download the exact recorded official `anon_casual` images to a temporary directory outside the repository, inspect each with the image viewing tool, and do not commit downloaded copyrighted references.

- [ ] **Step 2: Generate the first sheet with the `imagegen` skill** — use the official local references and this production prompt: “Create one transparent retro JRPG chibi pixel-art sprite sheet of Chihaya Anon in the referenced canonical casual outfit. Exact 4×4 grid, no grid lines, no text, 16 isolated 60×60 cells on a transparent canvas. Rows: down, left, right, up. Columns: left-foot contact, left passing, right-foot contact, right passing. Fixed ground root at local (30,57), two fully transparent pixels at every cell edge, hard one-pixel colored outline, binary alpha, top-left lighting, consistent body size and identity.” Save raw output outside the tracked asset directory, then run `powershell -NoProfile -File tools/normalize_character_sprite.ps1 -InputPath <raw-output> -OutputPath images/characters/pilot/anon_casual.png`.

- [ ] **Step 3: Correct through image editing only** — inspect the output at native size and 4× nearest-neighbor display. Use `imagegen` edits to correct wrong outfit details, grid drift, inconsistent face/hair, direction ambiguity, or non-pixel edges. Do not use Python or a drawing script for artistic edits.

- [ ] **Step 4: Record current pilot state** — calculate the lowercase SHA-256 of `anon_casual.png`, store it in `casual_sha256`, leave `stage_sha256: null`, and keep approval `pending`, `reviewed_at: null`, `accepted_exceptions: []`.

- [ ] **Step 5: Single-sheet mechanical check** — run `powershell -NoProfile -File tools/verify_character_sprites.ps1 -Mode Single -Root images/characters -Manifest docs/art/bandori-character-assets/v1/reference-manifest.yaml -File images/characters/pilot/anon_casual.png -FormId anon_casual`; expect exactly `PASS: anon_casual, 16 frames`. This is not yet the two-sheet Pilot gate.

- [ ] **Step 6: Commit** — run `git add images/characters/pilot/anon_casual.png docs/art/bandori-character-assets/v1/reference-manifest.yaml docs/art/bandori-character-assets/v1/pilot-approval.yaml && git commit -m "feat(art): add casual Anon pilot"`.

### Task 4: Produce the Chihaya Anon stage pilot sheet

**Files:**
- Create: `images/characters/pilot/anon_stage.png`
- Modify: `docs/art/bandori-character-assets/v1/reference-manifest.yaml`
- Modify: `docs/art/bandori-character-assets/v1/pilot-approval.yaml`

**Acceptance Criteria:**

```gherkin
Feature: Stage Anon pilot
  Scenario: Both Anon forms share one visual grammar
    Given the approved official casual and MyGO!!!!! stage references
    When corresponding frames from both sheets are compared
    Then character proportions, hairstyle, face palette, root anchor, outline, lighting, and cadence match
    And anon_stage uses the canonical stage outfit without fantasy additions
```

- [ ] **Step 1: Prepare stage references** — download and inspect the exact official `anon_stage` image recorded in the manifest; use the same official MyGO!!!!! visual campaign selected for Soyo and Tomori stage forms.

- [ ] **Step 2: Generate with the `imagegen` skill** — reference both the official Anon stage visual and the finished `anon_casual.png`. Reuse Task 3's exact grid/animation prompt, replacing only the outfit instruction with the canonical MyGO!!!!! stage outfit and requiring identical proportions, face palette, and anchor. Save raw output outside the tracked asset directory, then normalize it to `images/characters/pilot/anon_stage.png` with `tools/normalize_character_sprite.ps1`.

- [ ] **Step 3: Correct through image editing only** — compare all sixteen corresponding cells against the casual pilot; use `imagegen` edits until the identity, scale, directions, pixel treatment, and clothing details are consistent.

- [ ] **Step 4: Refresh pending approval metadata** — store both current lowercase SHA-256 values, keep status `pending`, set `reviewed_at: null`, and clear `accepted_exceptions`.

- [ ] **Step 5: Run Single and Pilot verification** — run Single mode with `-File images/characters/pilot/anon_stage.png -FormId anon_stage`; expect `PASS: anon_stage, 16 frames`. Then run `powershell -NoProfile -File tools/verify_character_sprites.ps1 -Mode Pilot -Root images/characters -Manifest docs/art/bandori-character-assets/v1/reference-manifest.yaml`; expect exactly `PASS: 2 sheets, 32 frames`.

- [ ] **Step 6: Commit** — run `git add images/characters/pilot/anon_stage.png docs/art/bandori-character-assets/v1/reference-manifest.yaml docs/art/bandori-character-assets/v1/pilot-approval.yaml && git commit -m "feat(art): add stage Anon pilot"`.

### Task 5: Review and authorize the pilot pair

**Files:**
- Modify: `docs/art/bandori-character-assets/v1/style-guide.md`
- Modify: `docs/art/bandori-character-assets/v1/pilot-approval.yaml`

**Acceptance Criteria:**

```gherkin
Feature: Pilot production gate
  Scenario: Verified delegated approval opens the gate
    Given both pilot sheets pass mechanical and visual review
    And the user delegated subsequent approval in the planning instruction
    When the approval record is finalized with current hashes
    Then Gate mode exits zero with ALLOW: pilot approved

  Scenario: A changed approved pilot closes and renews the gate
    Given approved pilot hashes no longer match the Anon PNGs
    When Gate mode runs
    Then it prints BLOCK: pilot hash mismatch
    And approval is reset to pending
    And every non-Anon PNG is quarantined
    And production resumes only after renewed delegated objective review records the new hashes
```

- [ ] **Step 1: Mechanical review** — rerun the verifier fixture suite, then Pilot mode. Both must exit `0` with their exact PASS messages.

- [ ] **Step 2: Visual review** — inspect both sheets at 1× and 4×; animate each row as equal 150 ms frames in order `1-2-3-4`. Check recognition, official costume fidelity, four distinct directions, stable root, stable scale, binary alpha, hard pixel edges, and absence of fantasy additions.

- [ ] **Step 3: Apply corrections and restart checks** — if either sheet changes, use `imagegen` for the edit, refresh both hashes, reset approval to pending, then repeat Steps 1 and 2 in full. Do not approve a stale hash.

- [ ] **Step 4: Record delegated approval** — because the user explicitly stated “后续不需要我批准,” set status to `approved`, store both current hashes, set `reviewed_at` to the current ISO-8601 timestamp, set `accepted_exceptions` to an empty list unless the objective review documented one, and record the approved pilot revision in `style-guide.md`.

- [ ] **Step 5: Run Gate mode** — run the exact Gate command from the source spec; expect `ALLOW: pilot approved` and exit `0`.

- [ ] **Step 6: Commit** — run `git add images/characters/pilot docs/art/bandori-character-assets/v1/style-guide.md docs/art/bandori-character-assets/v1/pilot-approval.yaml && git commit -m "chore(art): approve Anon pilot style"`.

### Task 6: Produce the MyGO!!!!! monster and boss sheets

**Files:**
- Create: `images/characters/mygo/soyo_casual.png`
- Create: `images/characters/mygo/soyo_stage.png`
- Create: `images/characters/mygo/tomori_casual.png`
- Create: `images/characters/mygo/tomori_stage.png`
- Modify: `docs/art/bandori-character-assets/v1/reference-manifest.yaml`

**Acceptance Criteria:**

```gherkin
Feature: MyGO!!!!! character forms
  Scenario: Soyo and Tomori follow the approved Anon grammar
    Given the approved Anon pilot and exact official MyGO!!!!! references
    When the four MyGO!!!!! sheets are compared at native size
    Then Soyo and Tomori are recognizable in both canonical outfits
    And casual forms read as lower strength while stage forms read as elite or boss forms
    And all sheets match the approved proportions, outline, lighting, anchor, and cadence
```

- [ ] **Step 1: Revalidate the gate** — run Gate mode immediately before each of the four output writes. On any BLOCK result, stop; Gate performs the recoverable atomic quarantine/reset transaction specified in Task 2.

- [ ] **Step 2: Produce `soyo_casual.png`** — use `imagegen` with the exact official Soyo casual reference and approved Anon pilots. Reuse the full 4×4 prompt; express mid-boss presence only through composed posture, direct gaze, and restrained expression, never invented costume pieces.

- [ ] **Step 3: Produce `soyo_stage.png`** — use the selected canonical MyGO!!!!! stage reference. Preserve Soyo identity and outfit; communicate final-boss strength through firmer stance, sharper expression, and at most a compact binary-alpha aura that remains within every guard band.

- [ ] **Step 4: Produce `tomori_casual.png`** — use the official Tomori casual reference and the approved style guide; keep her characteristic hair silhouette and reserved posture readable at 60×60.

- [ ] **Step 5: Produce `tomori_stage.png`** — use the matching MyGO!!!!! campaign stage reference; distinguish the elite form through performance stance and expression while keeping the canonical outfit unchanged.

- [ ] **Step 6: Normalize and validate each sheet** — save each imagegen result outside the tracked directory, normalize it to its declared path with `tools/normalize_character_sprite.ps1`, then run `powershell -NoProfile -File tools/verify_character_sprites.ps1 -Mode Single -Root images/characters -Manifest docs/art/bandori-character-assets/v1/reference-manifest.yaml -File <relative_path> -FormId <form_id>`. Expect `PASS: <form_id>, 16 frames`; compare all sixteen frames to the approved pilot at 1× and 4×. Update only factual `inferred_details` or bounded `allowed_translucency` in the manifest.

- [ ] **Step 7: Commit** — run `git add images/characters/mygo docs/art/bandori-character-assets/v1/reference-manifest.yaml && git commit -m "feat(art): add MyGO character forms"`.

### Task 7: Produce the Uika and Doloris sheets

**Files:**
- Create: `images/characters/ave_mujica/uika_casual.png`
- Create: `images/characters/ave_mujica/doloris_stage.png`
- Modify: `docs/art/bandori-character-assets/v1/reference-manifest.yaml`

**Acceptance Criteria:**

```gherkin
Feature: Uika and Doloris identity continuity
  Scenario: The masked stage form remains recognizable as Uika
    Given exact official Uika and Doloris references
    When corresponding casual and stage frames are compared
    Then hair, face palette, proportions, and movement identify one character
    And Doloris uses the canonical Ave Mujica mask and stage costume
```

- [ ] **Step 1: Revalidate the gate** — run Gate mode before each output; stop on any failure and let Gate complete its atomic quarantine/reset transaction.

- [ ] **Step 2: Produce `uika_casual.png`** — use the official casual reference plus both approved Anon sheets; apply the exact grid, guard, root, palette, outline, and cadence contract.

- [ ] **Step 3: Produce `doloris_stage.png`** — reference the finished Uika sheet and exact Doloris official visual. Preserve identity across mask and costume change; do not enlarge the mask or hair beyond the shared envelope.

- [ ] **Step 4: Normalize, verify, and compare** — save raw outputs outside the tracked directory, normalize each with `tools/normalize_character_sprite.ps1`, then run Single mode with the exact `-File` and `-FormId` from the manifest; expect `PASS: <form_id>, 16 frames`. Animate all four rows at 150 ms equal holds and compare identity continuity cell by cell.

- [ ] **Step 5: Commit** — run `git add images/characters/ave_mujica docs/art/bandori-character-assets/v1/reference-manifest.yaml && git commit -m "feat(art): add Uika and Doloris forms"`.

### Task 8: Produce the Arale and Viola sheets

**Files:**
- Create: `images/characters/mugendai_mewtype/arale_casual.png`
- Create: `images/characters/mugendai_mewtype/arale_stage.png`
- Create: `images/characters/mugendai_mewtype/viola_normal.png`
- Create: `images/characters/mugendai_mewtype/viola_corrupted.png`
- Modify: `docs/art/bandori-character-assets/v1/reference-manifest.yaml`

**Acceptance Criteria:**

```gherkin
Feature: 梦限大 character forms
  Scenario: Arale retains canonical casual and stage identities
    Given exact official Arale references
    When both Arale sheets are compared
    Then her hair, colors, proportions, and official outfits are recognizable and consistent

  Scenario: Viola transforms without changing her base identity
    Given Viola's official normal appearance
    When normal and corrupted frames are compared
    Then both retain the same body, clothing, hair, and face identity
    And the corrupted form adds bounded violet vines, phone-screen motifs, and editing fragments
```

- [ ] **Step 1: Revalidate the gate** — run Gate mode immediately before every output. Stop if the gate closes; Gate completes the recoverable atomic quarantine/reset transaction.

- [ ] **Step 2: Produce both Arale sheets** — use exact official casual and 梦限大 stage references. Generate `arale_casual.png` first, then reference it while producing `arale_stage.png` so proportions and identity remain fixed.

- [ ] **Step 3: Produce `viola_normal.png`** — use the official Viola anime reference. Preserve the gentle smile, composed stance, canonical clothes, and violet flower identity without exposing the corrupted effects.

- [ ] **Step 4: Produce `viola_corrupted.png` by image editing** — edit/reference the finished normal sheet rather than regenerating an unrelated character. Add violet vines, rectangular phone-screen light, splice marks, and editing fragments; keep clothing and body identity unchanged. Prefer binary alpha; if partial alpha is essential, record cell-local half-open rectangles and exact alpha bounds in `allowed_translucency`.

- [ ] **Step 5: Normalize, verify, and compare** — save raw outputs outside the tracked directory, normalize each with `tools/normalize_character_sprite.ps1`, then run Single mode with its exact manifest `-File` and `-FormId`; expect `PASS: <form_id>, 16 frames`. Animate all rows at 150 ms equal holds, verify effect bounds and directional clarity, and compare both character pairs against the approved pilot grammar.

- [ ] **Step 6: Commit** — run `git add images/characters/mugendai_mewtype docs/art/bandori-character-assets/v1/reference-manifest.yaml && git commit -m "feat(art): add Mugendai and Viola forms"`.

### Task 9: Run full-batch verification and prepare delivery

**Files:**
- Modify only if factual corrections are required: `docs/art/bandori-character-assets/v1/reference-manifest.yaml`
- Modify only if approved objective exceptions are required: `docs/art/bandori-character-assets/v1/pilot-approval.yaml`

**Acceptance Criteria:**

```gherkin
Feature: Complete character-art delivery
  Scenario: All twelve sheets pass final verification
    Given the approved pilot gate and completed delivery directories
    When Gate and Full modes run
    Then Gate prints ALLOW: pilot approved
    And Full prints PASS: 12 sheets, 192 frames
    And the Git diff contains no runtime code or resource-list changes

  Scenario: Every walk cycle reads correctly without interface labels
    Given native-size and nearest-neighbor 4x previews of every sheet
    When each row is animated in order 1-2-3-4 at equal 150 ms holds
    Then down, left, right, and up are unambiguous
    And alternating foot motion is restrained
    And no body or root exhibits global jitter

  Scenario: Costume pairs communicate strength through performance
    Given each casual-stage pair and the normal-corrupted Viola pair
    When the forms are compared without names, labels, or UI
    Then each casual or normal form reads as lower strength
    And each stage or corrupted form reads as its upgraded enemy form
    And the distinction uses canonical pose, expression, and bounded effects rather than invented costume pieces
```

- [ ] **Step 1: Verify the verifier** — run `powershell -NoProfile -File tools/test_character_sprite_verifier.ps1` twice; expect `PASS: verifier fixtures` both times.

- [ ] **Step 2: Verify the approval gate** — run the exact Gate command; expect `ALLOW: pilot approved` and exit `0`. If blocked, quarantine all non-Anon PNGs and return to Task 5 instead of weakening the gate.

- [ ] **Step 3: Verify the full roster** — run `powershell -NoProfile -File tools/verify_character_sprites.ps1 -Mode Full -Root images/characters -Manifest docs/art/bandori-character-assets/v1/reference-manifest.yaml`; expect exactly `PASS: 12 sheets, 192 frames`.

- [ ] **Step 4: Perform final visual review** — build temporary native-size and nearest-neighbor 4× contact sheets outside the repository, inspect all corresponding directions and forms, and delete only the temporary previews after review. Animate every row in `1-2-3-4` order at equal 150 ms holds and confirm unambiguous down/left/right/up directions, restrained alternating foot motion, no global body/root jitter, exact reference fidelity, stable proportions, outline, lighting, and palette density. Compare every casual/stage pair and Viola normal/corrupted pair without labels or UI and confirm the upgraded form reads stronger through canonical pose, expression, and bounded effects.

- [ ] **Step 5: Prove scope containment** — run `git diff --name-only $(git merge-base HEAD main)..HEAD`; confirm changes are limited to `images/characters/`, `docs/art/bandori-character-assets/`, `tools/`, and the approved spec/plan documents. Confirm `resources.qrc`, `images/player.png`, all `images/monster_*.png`, `UI/MapWidget.cpp`, and `UI/MainWindow.cpp` are unchanged.

- [ ] **Step 6: Commit factual final corrections if needed** — if Steps 1–5 required metadata-only corrections, run `git add docs/art/bandori-character-assets/v1 && git commit -m "docs(art): finalize character asset delivery"`; otherwise make no empty commit.
