# Recommended Unit Test Cases for Pacman Engine
**Author:** Nourdine Kaddour

This document lists essential test cases to ensure the `GameModel` is bug-free. Currently **22 tests** are implemented in `tests/test_gamemodel.cpp`.

---

## 1. Core Movement & Collision
- **[Wall Blocking]:** If Pacman is at (1,1) and there is a wall at (1,0), `canMove(UP)` must return `false`.
- **[Empty Path]:** If Pacman is at (1,1) and (1,2) is empty, `canMove(DOWN)` must return `true`.
- **[Teleportation Tunnel]:** If Pacman moves past the left map boundary (`x < -0.5`), verify he is teleported to the right side (`x = MAP_WIDTH - 0.5`).
- **[Snap to Center]:** If Pacman is at `(10.2, 17.0)` and tries to turn `DOWN`, verify the engine "snaps" him to `(10.0, 17.0)` before the turn.

## 2. Pellet & Power Pellet Mechanics
- **[Pellet Collection]:** Verify score increases by 10 when Pacman passes over a pellet tile.
- **[Power Mode Trigger]:** Verify `powerMode` becomes `true`, ghosts become `frightened`, and timer starts when power pellet is eaten.
- **[Power Mode Expiration]:** Verify `powerMode` reverts after `powerTimer` elapses.

## 3. Ghost Personalities & Scatter Mode

### Ghost Existence & Types
- Verify exactly 4 ghosts exist after initialization and reset.
- Verify each ghost has the correct `GhostType` (BLINKY, PINKY, INKY, CLYDE).

### Scatter/Chase Cycle
- Verify `isScatterMode()` returns `false` at game start (chase mode).
- Verify scatter mode activates after ~20s of chase (if Pacman survives).
- Verify `reset()` clears scatter mode and restores 4 ghosts.

### Ghost Personalities (Arcade Logic)
| Ghost | Chase Target | Scatter Corner |
|-------|-------------|----------------|
| Blinky | Pacman's current `(x, y)` | `(19, 1)` top-right |
| Pinky | 4 tiles ahead of Pacman's direction | `(1, 1)` top-left |
| Inky | Vector reflection from Blinky → 2 tiles ahead of Pacman, doubled | `(19, 19)` bottom-right |
| Clyde | Pacman if >8 tiles away, else scatter corner | `(1, 19)` bottom-left |

## 4. Recording & Playback (Demo Mode)

- **[Recording Exists]:** After starting recording, running updates, and stopping, `hasRecording()` returns `true`.
- **[Playback Matches]:** After recording, playback restores exact Pacman position, score, and direction.
- **[Replay Game Over]:** A recording that ends in game-over correctly reproduces the game-over state during playback.
- **[No Recording Safety]:** Calling `startPlayback()` with no recording does not crash.
- **[Persistence]:** Recording survives save/load cycle (`~/.pacman_replay` binary file).
- **[Pellet Visibility]:** During playback, pellets disappear as Pacman passes over them (visually matched).

## 5. Game Over & Win Conditions
- **[Game Over State]:** If `lives` reaches 0, verify `isGameOver()` returns `true`.
- **[Win Condition]:** If all pellets are eaten, verify `isGameOver()` returns `true`.

## 6. Timing (DeltaTime)
- **[Speed Consistency]:** Verify Pacman moves exactly `speed * deltaTime` units per frame.
