# BanG Dream! Theme Mota Character Art Assets

## Problem

The existing Qt/C++ mota project renders one static 60×60 PNG for the player and each monster. The new game direction needs a recognizable BanG Dream! crossover cast drawn as coherent retro chibi pixel art, with enough directional animation to support a later movement-system upgrade. Producing all assets at once would make style or costume mistakes expensive to correct, so the work needs a small approval-gated pilot before the full batch.

## Goals

- Define and produce a coherent character-only art pack for a MyGO!!!!!, Ave Mujica, and 梦限大みゅーたいぷ themed mota.
- Preserve each character's official hairstyle, color identity, and selected canonical casual or stage outfit.
- Deliver four-direction walking animation sheets at the project's 60×60 tile scale.
- Establish Anon Chihaya's casual and stage sheets as the visual and technical pilot.
- After pilot approval, extend the approved style to ten additional character forms.

## Non-Goals

- Changing the Qt renderer, animation timing, resource loader, map format, combat data, or game logic.
- Replacing `images/player.png` or any `images/monster_XX.png` during the art-only phase.
- Producing tiles, doors, UI, portraits, item icons, battle effects, dialogue art, or audio.
- Designing additional band members beyond the twelve forms listed below.
- Inventing fantasy armor, robes, weapons, or alternate costumes that are not part of the selected official appearances.

## Design Principles

1. **Recognizable before detailed.** Hair silhouette, outfit color blocks, masks, and signature accessories must remain readable at 60×60.
2. **Canonical wardrobe.** Official character and stage visuals are the wardrobe authority; pixel simplification may remove detail but must not redesign the clothing.
3. **One visual grammar.** All sheets share outline weight, head-to-body ratio, lighting direction, palette density, ground anchor, and animation cadence.
4. **Gameplay-readable forms.** Casual forms read as normal or mid-stage entities; stage forms read as elite, upgraded, or final forms through pose, expression, and restrained effects rather than costume alteration.
5. **Pilot before batch.** No non-Anon production begins until both Anon sheets pass visual and technical review.
6. **Clean pixel output.** Hard pixel edges, no anti-aliased fringe, no baked background, no text, and no partially transparent halo pixels outside intentional effects.

## Acceptance Scenarios

```gherkin
Feature: Anon pilot sprite sheets

  Scenario: Casual Anon sheet has the required grid
    Given the casual Anon pilot PNG
    When its dimensions and alpha channel are inspected
    Then it is exactly 240 by 240 pixels in RGBA format
    And it contains four rows and four columns of 60 by 60 frames
    And every frame has a fully transparent two-pixel guard band on all four edges

  Scenario: Stage Anon is visibly the same character in a different canonical outfit
    Given the casual and stage Anon pilot sheets
    When corresponding directions are compared at native size
    Then hairstyle, face palette, body proportions, and ground anchor are consistent
    And the stage sheet uses the selected official stage outfit
    And no fantasy costume elements have been added

  Scenario: Walking cycles are directionally legible
    Given any approved sprite sheet
    When each row is viewed as a four-frame loop
    Then the facing direction is unambiguous
    And the rows depict down, left, right, and up exactly once
    And the row order is down, left, right, then up
    And alternating steps produce visible but restrained limb and hair movement
    And the character remains centered without camera-like jitter

  Scenario: Batch production is blocked until the pilot pair is approved
    Given both Anon pilot sheets have been generated
    And their approval record is not marked approved
    When the gate command is run
    Then it exits nonzero with `BLOCK: pilot approval pending`
    And no non-Anon sprite sheet exists in the delivery directories
    When the user explicitly approves both recorded pilot revisions
    Then the approval record is marked approved
    And the gate command exits zero with `ALLOW: pilot approved`

  Scenario: Changing an approved pilot closes the production gate
    Given both Anon pilot hashes match an approved record
    When either approved pilot sheet changes
    And the gate command is run
    Then it exits nonzero with `BLOCK: pilot hash mismatch`
    And the approval status is reset to pending
    And no non-Anon sheet is written to a delivery directory until the revised pair is explicitly approved

Feature: Full character batch

  Scenario: Full batch contains every approved form
    Given the completed character asset directory
    When the expected filenames are checked
    Then all twelve approved forms are present exactly once
    And each file passes the same grid, alpha, boundary, and animation checks as the pilot

  Scenario: Every form follows its official reference and the approved visual grammar
    Given all twelve forms and the versioned reference manifest
    And the approved Anon pilot pair
    When every form is reviewed against its exact recorded official visual and the pilot pair
    Then every character has a recognizable canonical hairstyle and color identity
    And every costume matches its recorded official visual
    And proportions, outline weight, lighting direction, palette density, foot anchor, and animation cadence match the approved pilot grammar

  Scenario: Viola's two forms communicate transformation
    Given Viola's normal and corrupted sheets
    When corresponding frames are compared
    Then both retain Viola's official base appearance
    And the normal form reads as gentle and composed
    And the corrupted form adds violet vines, phone-screen motifs, and editing-fragment effects
    And those effects remain inside each 60 by 60 frame

  Scenario: Costume progression communicates gameplay strength
    Given a character with casual and stage forms
    When both forms are viewed at game scale
    Then the casual form reads as the lower-strength form
    And the stage form reads as the elite or upgraded form
    And the distinction does not depend on labels or UI text
```

## Design

### Asset Roster

| ID | Character | Form | Gameplay reading |
|---|---|---|---|
| `anon_casual` | Chihaya Anon | canonical casual outfit | player, opening form |
| `anon_stage` | Chihaya Anon | canonical MyGO!!!!! stage outfit | player, upgraded form |
| `soyo_casual` | Nagasaki Soyo | canonical casual outfit | mid-game boss |
| `soyo_stage` | Nagasaki Soyo | canonical MyGO!!!!! stage outfit | final boss |
| `tomori_casual` | Takamatsu Tomori | canonical casual outfit | normal monster form |
| `tomori_stage` | Takamatsu Tomori | canonical MyGO!!!!! stage outfit | elite monster form |
| `uika_casual` | Misumi Uika | canonical casual outfit | normal monster form |
| `doloris_stage` | Misumi Uika / Doloris | canonical Ave Mujica masked stage outfit | elite monster form |
| `arale_casual` | Nakamachi Arale | canonical casual outfit | normal monster form |
| `arale_stage` | Nakamachi Arale | canonical 梦限大みゅーたいぷ stage outfit | elite monster form |
| `viola_normal` | Viola | canonical anime outfit | deceptive normal monster form |
| `viola_corrupted` | Viola | same canonical outfit with corruption effects | transformed monster form |

### Reference Selection

- Use only official BanG Dream! character pages, artist pages, anime pages, key visuals, or official live/stage promotional visuals as costume references.
- Store the selection in `docs/art/bandori-character-assets/v1/reference-manifest.yaml` before generating any sheet.
- Every roster ID is a manifest key with these required fields: `character` (string), `form` (string), `canonical_page_url` (URL string), `costume_asset_url_or_id` (string), `campaign_or_title` (string), `retrieved_at` (ISO-8601 date), `inferred_details` (string list), and `style_guide_version` (string).
- Optional `allowed_translucency` is a list whose entries contain `cell: {row, column}`, `rect: {x, y, width, height}`, `min_alpha`, and `max_alpha`. Rows and columns are zero-based integers from 0 through 3. Rectangle coordinates are integer, cell-local, and half-open: `[x, x+width)` and `[y, y+height)`. Rectangles must stay inside `2 <= x`, `2 <= y`, `x+width <= 58`, and `y+height <= 58`. Alpha bounds are integers satisfying `1 <= min_alpha <= max_alpha <= 254`.
- `costume_asset_url_or_id` must point to the exact selected image or record a stable image/asset identifier within the official page. A changing landing-page or carousel URL alone is insufficient.
- Prefer a single official visual campaign per band so members do not accidentally mix unrelated costume generations.
- If an official image does not show a rear or side view, infer hidden garment construction conservatively from the visible silhouette and matching official visuals; do not invent prominent decorations.

### Versioned Visual Guide

- Store the shared visual contract in `docs/art/bandori-character-assets/v1/style-guide.md`.
- The guide must contain: guide version; approved pilot revision; head-to-body ratio; character envelope; outline rules; light direction; palette-density rule; alpha policy; row and column order; foot anchor; motion cadence; and any accepted exceptions.
- Every produced sheet records the guide version it follows in the reference manifest.

### Pixel Style

- Canvas per frame: 60×60 transparent pixels.
- Character envelope: approximately 44–50 pixels wide and 52–56 pixels tall, bottom-centered, with at least 2 transparent pixels above the highest opaque pixel.
- Proportion: large chibi head, compact torso, short limbs; keep the same proportion template for all forms.
- Outline: predominantly one-pixel dark colored outline; avoid pure black except for the darkest costume or mask accents.
- Shading: two main tones per material plus one highlight where necessary; no gradients or painterly blur.
- Color: preserve canonical hair, eye, skin, and costume color relationships; reduce palette only where required for clarity.
- Shadow: no detached ground shadow in the production sheet. The game can add one later if desired.

### Sprite-Sheet Contract

- File type: transparent RGBA PNG.
- Sheet size: 240×240.
- Grid: four columns by four rows; every cell is exactly 60×60.
- Row order: down, left, right, up.
- Column order: left-foot contact, left passing pose, right-foot contact, right passing pose.
- Anchor: local coordinate `(30, 57)` is a fixed ground-plane/root anchor independent of the geometric midpoint of the visible feet. Lifted-foot and passing poses may place visible feet above or asymmetrically around this root without moving the root itself.
- Guard band: local coordinates `x=0..1`, `x=58..59`, `y=0..1`, and `y=58..59` in every cell must remain fully transparent. Character and effect pixels are restricted to the 56×56 interior region. This makes accidental cross-cell overflow mechanically detectable.
- Loops must not translate the character globally; apparent movement comes from limb, garment, hair, and accessory motion.
- Effects belonging to Viola's corrupted form or boss/elite emphasis must remain frame-local and must not obscure the facing direction.

### Form Differentiation

- Casual and stage variants use their respective official costumes with identical character proportions.
- Stage and boss strength may be expressed through firmer stance, sharper expression, brighter eye highlights, mask visibility, or a compact one-pixel/two-pixel aura.
- Soyo's stage form must remain recognizably Soyo in MyGO!!!!! attire; boss framing must not turn her into a generic armored villain.
- Viola's corrupted form keeps the same clothes and body identity as the normal form. Violet vines, phone-screen rectangles, splice marks, and glitch-like fragments are overlays, not a replacement costume.

### Pilot Review Gate

The first delivery contains only `anon_casual.png` and `anon_stage.png`. Review occurs in this order:

1. Native-size silhouette and character recognition.
2. Official-costume fidelity.
3. Directional clarity for all four rows.
4. Four-frame motion consistency and anchor stability.
5. Transparency, grid boundaries, and pixel-edge cleanliness.

Record the gate in `docs/art/bandori-character-assets/v1/pilot-approval.yaml` with these required fields: `status` (`pending` or `approved`), `casual_sha256`, `stage_sha256`, `style_guide_version`, `reviewed_at`, and `accepted_exceptions`.

- `approved` is valid only when both hashes identify the exact sheets reviewed by the user.
- Before a pilot exists, pending `casual_sha256` and `stage_sha256` may be `null`. Once both pilots exist, these fields contain their current lowercase hexadecimal SHA-256 values.
- In `pending`, `reviewed_at` is `null` and `accepted_exceptions` is `[]`. Any pilot hash change stores the new current hashes and restores those pending values, even if an earlier revision was approved.
- Any correction changes the relevant hash, resets status to `pending`, reruns all five visual and mechanical review stages for both sheets, and requires renewed explicit approval.
- A style rejection leaves the record pending, stops all non-Anon production, updates the reference/style guides, and starts a new two-sheet pilot revision. Ending the project also leaves it pending and simply stops production; no extra persisted lifecycle state is introduced.
- The remaining ten sheets start only while the approval record is `approved` and both current pilot hashes match the record.
- Run the named gate interface before every non-Anon output and again before final handoff:

  ```powershell
  powershell -NoProfile -File tools/verify_character_sprites.ps1 -Mode Gate -Root images/characters -Manifest docs/art/bandori-character-assets/v1/reference-manifest.yaml -Approval docs/art/bandori-character-assets/v1/pilot-approval.yaml -StyleGuide docs/art/bandori-character-assets/v1/style-guide.md
  ```

- Gate success exits `0` and prints `ALLOW: pilot approved`. A hash or style-guide mismatch resets approval to the pending values above, exits `3`, and prints `BLOCK:` followed by the failed condition.
- If approval becomes invalid after batch work exists, move all non-Anon PNGs to `images/characters/.quarantine/<timestamp>/` before completing the state transition. Quarantined files are recoverable but are excluded from delivery and all Pilot/Full verification.

### Delivery Layout

```text
images/
  characters/
    pilot/
      anon_casual.png
      anon_stage.png
    mygo/
      soyo_casual.png
      soyo_stage.png
      tomori_casual.png
      tomori_stage.png
    ave_mujica/
      uika_casual.png
      doloris_stage.png
    mugendai_mewtype/
      arale_casual.png
      arale_stage.png
      viola_normal.png
      viola_corrupted.png
docs/
  art/
    bandori-character-assets/
      v1/
        reference-manifest.yaml
        style-guide.md
        pilot-approval.yaml
tools/
  verify_character_sprites.ps1
```

These files are additive art sources. They are not inserted into `resources.qrc` and do not replace existing runtime files during this scope.

## Implementation Phases

### Phase 1: Reference Manifest

- Create the versioned reference manifest and style guide at the paths defined above.
- Select and record exact official visual asset IDs or image URLs for all twelve forms, including campaign/title and retrieval date.
- Confirm that the chosen casual and stage visuals are internally consistent within each band.
- Prepare the shared palette, proportion, outline, and anchor guide.

### Phase 2: Anon Pilot Pair

- Produce `anon_casual.png` and `anon_stage.png`.
- Inspect both sheets at native size and enlarged nearest-neighbor scale.
- Correct frame boundaries, anchor drift, transparency, costume errors, and directional ambiguity.
- Present both sheets to the user for explicit approval.
- After every correction, rerun the complete check suite, refresh both hashes, reset the gate to pending, and request approval again.

### Phase 3: Remaining Ten Forms

- Run the Gate mode immediately before writing each non-Anon output; a nonzero result stops the phase and quarantines any existing non-Anon delivery files.
- Produce the MyGO!!!!! monster and Soyo boss sheets using the approved Anon visual grammar.
- Produce Uika and Doloris while preserving identity across mask and costume changes.
- Produce Arale's casual and stage sheets.
- Produce Viola's normal sheet, then derive the corrupted form without losing identity or frame alignment.

### Phase 4: Batch Verification and Handoff

- Run mechanical image checks across all twelve files.
- Re-run Gate mode immediately before the final Full verification and handoff.
- Build native-size and enlarged contact sheets for visual comparison without treating them as runtime assets.
- Deliver the verified PNGs and document any intentionally inferred costume details.

## Testing Strategy

### Mechanical Checks

- Implement the non-runtime verifier `tools/verify_character_sprites.ps1` with `-Mode Pilot|Full|Gate`.
- Pilot verification expects exactly `anon_casual` and `anon_stage` and runs as:

  ```powershell
  powershell -NoProfile -File tools/verify_character_sprites.ps1 -Mode Pilot -Root images/characters -Manifest docs/art/bandori-character-assets/v1/reference-manifest.yaml
  ```

- Pilot success exits `0` and prints `PASS: 2 sheets, 32 frames`.
- Full verification expects exactly the twelve Asset Roster IDs and runs as:

  ```powershell
  powershell -NoProfile -File tools/verify_character_sprites.ps1 -Mode Full -Root images/characters -Manifest docs/art/bandori-character-assets/v1/reference-manifest.yaml
  ```

- Full success exits `0` and prints `PASS: 12 sheets, 192 frames`. Any violation exits nonzero and prints the file, row, column, and failed invariant. Pilot mode fails if a non-Anon delivery PNG exists; Full mode fails if any roster sheet is absent or if an unexpected delivery PNG exists. Both modes ignore `.quarantine/`.
- The verifier checks that every output is a decodable RGBA PNG measuring exactly 240×240, divides into sixteen 60×60 cells, matches the expected filename roster, and leaves the two-pixel guard band on every cell edge fully transparent.
- Alpha policy is binary by default: every pixel alpha must be `0` or `255`. A sheet may use alpha `1..254` only inside manifest-declared `allowed_translucency` rectangles with explicit `min_alpha` and `max_alpha`; any semi-transparent pixel outside those regions fails.
- The verifier confirms that every declared translucency rectangle remains inside the 56×56 cell interior and never intersects a guard band.

### Verifier Test-First Protocol

1. Add minimal generated fixtures under `tools/testdata/character-sprites/`: one valid pilot pair plus separate invalid cases for dimensions, encoding/alpha mode, missing/unexpected roster files, nontransparent guard bands, undeclared partial alpha, out-of-bounds translucency declarations, hash mismatch, and style-guide mismatch.
2. Before implementing each verifier rule, run its fixture and confirm the test harness fails for the intended missing rule rather than for an unrelated setup error.
3. Implement the minimum verifier behavior for that rule.
4. Re-run the fixture suite and require all expected-pass and expected-failure cases to report the specified exit code and message.
5. Finally run the exact Pilot, Gate, and Full commands against production assets.

### Visual Checks

- View every frame at 1× and nearest-neighbor 4× scale.
- Compare all forms for stable head size, body scale, foot anchor, outline weight, and light direction.
- Preview every row using the fixed loop `1-2-3-4` at 150 ms per frame with equal holds; reject visible global jitter, direction changes, or cadence inconsistent with the approved pilot.
- Compare each costume against its recorded official reference.
- Check that elite/Boss forms remain readable without relying on non-canonical costume changes.

### In-Game Compatibility Check

- This art-only phase does not modify the renderer.
- Before runtime integration, crop representative 60×60 frames from both Anon sheets and preview them over the existing floor tile to confirm scale and contrast.
- Animation integration and its automated tests belong to a later code specification.

## File Inventory

### New specification and decision files

- `docs/superpowers/specs/2026-09-11-bandori-mota-art-assets-design.md`
- `docs/superpowers/brainstorms/2026-09-11-bandori-mota-art-assets-brainstorm.md`

### Planned production metadata and verification files

- `docs/art/bandori-character-assets/v1/reference-manifest.yaml`
- `docs/art/bandori-character-assets/v1/style-guide.md`
- `docs/art/bandori-character-assets/v1/pilot-approval.yaml`
- `tools/verify_character_sprites.ps1`
- `tools/testdata/character-sprites/` verifier fixtures

### Planned art files

- The twelve PNG files listed under Delivery Layout.

### Existing files intentionally unchanged

- `resources.qrc`
- `images/player.png`
- `images/monster_01.png` through `images/monster_18.png`
- `UI/MapWidget.cpp`
- `UI/MainWindow.cpp`

## Out of Scope

- Mapping the new forms onto the eighteen current monster IDs.
- Adding more MyGO!!!!!, Ave Mujica, or 梦限大みゅーたいぷ members.
- Combat portraits, cut-ins, transformation cinematics, and boss attack animation.
- Publishing or commercially licensing the fan project.
- Any representation that changes the selected official outfits into original fantasy costumes.
