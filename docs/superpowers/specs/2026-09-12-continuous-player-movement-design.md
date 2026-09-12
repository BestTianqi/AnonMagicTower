# Continuous Player Movement

## Problem

`MainWindow::keyPressEvent` currently changes the player's integer tile coordinate immediately, while `MapWidget` paints the player at one tile rectangle. This produces a visible jump between cells and never uses the prepared four-direction walking sheet.

## Goals

- Render the player at interpolated pixel coordinates so movement appears continuous.
- Use the existing 4×4 Anon walking sheet for directional walking frames.
- Preserve original grid-based collision, combat, pickup, door, NPC, shop, and stair behavior.
- Keep static `images/player.png` as a safe fallback.

## Non-Goals

- Rewriting map collision or combat rules.
- Adding diagonal movement, acceleration, or free-form sub-tile interactions.
- Changing monster animation or editor behavior.

## Design Principles

1. Gameplay remains grid-authoritative; interpolation is presentation-only.
2. Input is deterministic: one queued destination at a time, with key-repeat handled by the widget timer.
3. Animation must be crisp at the 60×60 tile scale and never stretch the sprite sheet.
4. Existing save data continues to store integer tile coordinates only.

## Acceptance Scenarios

```gherkin
Feature: Continuous player movement

  Scenario: Player visibly travels between two cells
    Given the player is on a walkable tile and a direction key is held
    When the next tile is walkable
    Then the player sprite moves through intermediate pixel positions
    And arrives at the destination tile without changing the saved integer coordinate format

  Scenario: Collision remains grid authoritative
    Given the adjacent tile is a wall, locked door, or monster
    When the direction key is pressed
    Then no interpolation starts until the existing move result allows it
    And combat, door, and pickup behavior remains unchanged

  Scenario: Walking frames follow direction rows
    Given the Anon 4×4 sheet is loaded
    When the player walks down, left, right, or up
    Then the renderer selects the matching row
    And cycles its four frames while moving
    And uses a stable idle frame when movement stops

  Scenario: Static fallback remains usable
    Given the walking sheet cannot be loaded
    When the map is painted
    Then the existing static player PNG is drawn at the interpolated position
```

## Design

### MainWindow input controller

Add a small movement state containing direction, current visual position, destination tile, and a `QElapsedTimer`/`QTimer` tick. A key press requests the next grid move through `Game::tryMovePlayer`; the resulting side effects happen once, at request time as today, while the visual position animates to the accepted tile. Holding a key uses a timer to request another move after the current animation completes.

### MapWidget player renderer

Load `:/images/characters/pilot/anon_casual.png` as a 4×4 sheet, split into 60×60 frames, and expose the active direction/frame to the painter. Draw the frame centered in the interpolated tile rectangle. If the sheet is unavailable, draw the existing static `m_playerPix`.

### Timing

Use 180 pixels per second with a 16 ms repaint timer. A one-tile transition therefore lasts about 333 ms. Walking animation advances every 80 ms and loops four frames; idle uses frame 1 of the current direction.

### Persistence

Save/load formats remain unchanged: only integer `Player::x` and `Player::y` are persisted. In-progress interpolation is reset to the loaded tile position.

## Implementation Phases

1. Add a failing movement-state/render test seam and sheet-loading API.
2. Implement frame-sheet loading and interpolated painting with fallback.
3. Replace direct key-repeat behavior with queued movement and timer ticks.
4. Run build, classic tests, and a focused movement smoke test.

## Testing Strategy

- Unit-level checks for frame row/column selection and interpolation clamping.
- Existing `mota_classic_tests` for unchanged game rules.
- Manual smoke: hold each arrow key, verify smooth travel, wall blocking, pickup, combat, and stair transitions.

## File Inventory

- `UI/MapWidget.h/.cpp` — sprite-sheet state and interpolated player paint.
- `UI/MainWindow.h/.cpp` — movement timer, key state, and queue.
- `images/characters/pilot/anon_casual.png` — existing 4×4 walking sheet.

## Out of Scope

- Replacing monster sprites or adding camera scrolling.
