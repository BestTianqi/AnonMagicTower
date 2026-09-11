---
style_guide_version: "1.0.0"
---

# BanG Dream! Mota Character Sprite Style Guide

## Version and pilot state

- Guide version: `1.0.0`.
- Approved pilot revision: none; the Anon pilot gate is pending.
- Pilot authority: `anon_casual.png` and `anon_stage.png` become the visual baseline only after their recorded hashes are approved.
- Accepted exceptions: none.

## Canonical identity and costume

- Use only the exact official references recorded in `reference-manifest.yaml`.
- Preserve each character's canonical hair silhouette, eye/skin relationship, costume color blocks, mask, and signature accessories.
- Simplification may remove details that cannot read at 60×60; it must not introduce fantasy armor, weapons, robes, or replacement clothing.
- Infer unseen side or rear construction conservatively from visible seams, silhouettes, and the matching official band campaign.
- Casual or normal forms read as lower strength. Stage, masked, boss, or corrupted forms read as upgraded through stance, gaze, expression, mask visibility, brighter eye highlights, or compact bounded effects—not invented costume elements.

## Geometry and proportions

- File: transparent RGBA PNG, exactly 240×240 pixels.
- Grid: 4 columns × 4 rows; every frame is exactly 60×60 pixels.
- Character envelope: approximately 44–50 pixels wide and 52–56 pixels tall, bottom-centered.
- Keep at least two transparent pixels above the highest opaque pixel.
- Proportion template: large chibi head, compact torso, and short limbs; use one stable head-to-body ratio across all forms, approximately 1:1 from head height to the combined torso-and-leg height.
- Fixed root anchor: local cell coordinate `(30,57)`. This ground-plane anchor does not move even when visible feet lift or become asymmetric.
- No global body translation or camera-like jitter across a row.

## Grid order and motion

- Row order, top to bottom: down, left, right, up.
- Column order, left to right: left-foot contact, left passing, right-foot contact, right passing.
- Playback: four equal 150 ms holds in a `1-2-3-4` loop.
- Motion is restrained and comes from limbs, clothing, hair, masks, and accessories; all four directions must remain unambiguous.

## Guard band and alpha

- In every 60×60 cell, `x=0..1`, `x=58..59`, `y=0..1`, and `y=58..59` are fully transparent.
- Character and effect pixels remain inside the 56×56 interior.
- Default alpha is binary: every pixel alpha is `0` or `255`.
- Partial alpha is forbidden unless a form declares a bounded cell-local half-open rectangle and inclusive alpha range in `allowed_translucency`.
- No anti-aliased fringe, baked background, grid lines, text, or detached ground shadow.

## Pixel treatment and palette

- Outline: predominantly a one-pixel dark colored outline; reserve pure black for the darkest canonical costume or mask accents.
- Shading: two main tones per material plus one optional highlight where necessary.
- Lighting: top-left consistently across every frame and form.
- Palette density: use the fewest colors that preserve canonical hair, eyes, skin, costume relationships, and readable material separation; avoid gradients and painterly blur.
- Edges must remain hard pixels at native size and nearest-neighbor enlargement.

## Form-specific constraints

- Soyo's stage boss form stays recognizably Soyo in the canonical MyGO!!!!! outfit; strength comes from posture and expression.
- Uika and Doloris share one body, hair, and face identity; Doloris adds only the canonical mask and Ave Mujica stage costume.
- Viola corrupted keeps Viola's normal clothing, body, hair, and face. Violet vines, phone-screen rectangles, splice marks, and editing fragments are overlays kept inside each frame and may not obscure facing direction.

## Review order

1. Native-size silhouette and character recognition.
2. Official-costume fidelity.
3. Directional clarity for all four rows.
4. Four-frame cadence, root stability, scale stability, and restrained alternating motion.
5. Transparency, guard bands, binary alpha, and hard pixel edges.

Any pilot correction changes its hash, resets approval to pending, and requires the complete review order to run again before batch production.
