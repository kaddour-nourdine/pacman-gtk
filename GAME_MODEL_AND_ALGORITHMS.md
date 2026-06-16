# GameModel: Key Methods and Core Algorithms

The `GameModel` acts as a pure, mathematical state machine for the Pacman engine.

## Part 1: Key Methods

### Core Game Loop
- **`update(double deltaTime)`**: Called every frame. Advances game state, moves entities, checks collisions, processes pellets, runs ghost AI, captures recording snapshots, or replays recorded snapshots.

### Player Input
- **`setPacmanNextDirection(Direction dir)`**: Stores the player's intended direction as `next_dir`. The engine executes it when safe (at tile center, snap-to-grid, or instant opposite reversal).

### Game State Queries
- **`getTile(int x, int y)`**: Returns `TileType` (WALL/EMPTY/PELLET/POWER_PELLET) at a grid coordinate.
- **`getPacman()` / `getGhosts()`**: Returns `Entity` structs with sub-tile `x`, `y`, `dir`, `frightened`.
- **`getScore()` / `getLives()`**: Player stats.
- **`isGameOver()` / `isDying()`**: State flags for UI death animation and game-over screen.
- **`isPowerMode()`**: Whether ghosts are frightened (for UI visual effects).
- **`isScatterMode()`**: Whether ghosts are in scatter or chase phase.
- **`hasWon()`**: Whether all pellets and power pellets have been eaten.

### Recording & Playback
- **`startRecording()`**: Clears previous recording, sets `recording = true`.
- **`stopRecording()`**: Stops recording and persists snapshots to `~/.pacman_replay`.
- **`startPlayback()`**: Resets game state, begins sequential replay of recorded snapshots.
- **`clearRecording()`**: Clears in-memory snapshots.
- **`hasRecording()`**: Whether snapshots exist (for menu "Watch Demo" availability).
- **`isReplaying()`**: Whether playback is active (for UI to show REPLAY label).

### Lifecycle
- **`reset()`**: Reinitializes map, entities, scores, timers to starting state. Does **not** clear recording.

---

## Part 2: Core Algorithms

### 1. Movement & Collision
- **Variable Time Step**: `speed * deltaTime` ensures frame-rate-independent movement.
- **Center-Snapping**: When Pacman is within 0.3 tiles of grid center and requests a turn, the engine snaps to the integer grid center before changing direction. This prevents corner snagging.
- **Opposite Reversal**: If the player presses the opposite direction, Pacman reverses instantly without waiting for grid center.
- **Boundary Teleportation**: `x < -0.5` wraps to `MAP_WIDTH - 0.5` (tunnel).
- **Dead-End Handling**: When movement is blocked by a wall and the entity is within `moveDist` of the grid center, it snaps to center and stops.

### 2. Ghost AI — Scatter/Chase Cycle
- **Chase Mode (20s)**: Each ghost targets Pacman using its unique personality algorithm.
- **Scatter Mode (7s)**: Each ghost targets its assigned home corner.

| Ghost | Corner | Chase Target |
|-------|--------|-------------|
| Blinky | (19, 1) | Pacman's `(x, y)` |
| Pinky | (1, 1) | 4 tiles ahead of Pacman's direction |
| Inky | (19, 19) | Vector: 2 tiles ahead of Pacman, doubled from Blinky's position |
| Clyde | (1, 19) | Pacman if >8 tiles away, else scatter corner |

Ghosts never reverse direction; at each intersection they choose the valid direction closest to their target (Euclidean distance). Frightened ghosts use random directions.

### 3. Recording Algorithm
Every frame in PLAYING mode, a `Snapshot` is captured after all game logic runs:
```
struct Snapshot {
    px, py, pdir, pnext;          // Pacman state
    gx[4], gy[4], gdir[4], gnext[4], frightened[4];  // Ghost states
    score, lives, powerMode, powerTimer;
    dying, dyingTimer, scatterMode, modeTimer, gameOver;
};
```

During REPLAY, the snapshot is applied directly:
1. Read `Snapshot` at index `snapshotIndex`
2. Set all entity positions/directions/timers from snapshot
3. Clear pellets at Pacman's current grid position for visual matching
4. Increment `snapshotIndex`

This guarantees frame-perfect replay regardless of RNG differences.

### 4. Power Pellet Mechanics
When eaten: `powerMode = true`, all ghosts become `frightened = true`, `powerTimer = 10.0s`. Timer decrements each frame; when it reaches 0, normal ghost AI resumes.
