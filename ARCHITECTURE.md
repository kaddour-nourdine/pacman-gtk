**Author: Nourdine Kaddour**

# Pacman Game Architecture & Game Loop

This document explains the core architecture of the cross-platform Pacman game and how the game loop functions.

## 1. Core Architecture: Engine vs. UI

The project uses a clean architectural pattern that separates the **Game Logic** from the **User Interface (UI)**.

*   **The Engine (`GameModel`)**: Written in shared C++, this acts as a passive calculator. It maintains the state of the game (Pacman's position, ghost AI, maze layout, demo recording) and enforces the rules (collisions, score). It does not know anything about drawing graphics or reading direct keyboard events.
*   **The UI (Platform Specific)**: Each platform (Linux/GTK, Android/Compose, iOS/SwiftUI) implements its own graphical interface. The UI is responsible for capturing input, rendering the visuals, and "driving" the game engine forward.

### Project Structure
```
├── gamemodel.h          Shared C++ header (Entity, Snapshot, GameModel API)
├── gamemodel.cpp        Shared C++ engine (movement, ghosts, recording)
├── main_gtk.cpp         Linux GTK4 frontend (menu, game canvas, key input)
├── tests/
│   └── test_gamemodel.cpp  22 unit tests
├── android/
│   └── app/src/main/
│       ├── cpp/native-lib.cpp   JNI bridge (exposes full API)
│       └── java/com/pacman/     Kotlin Compose UI
```

## 2. The Game Loop & Inversion of Control

Because modern UI frameworks are event-driven, the game relies on a UI-driven timer tied to the monitor's refresh rate.

The standard flow is:
1. **Initialize**: Create an instance of the `GameModel`.
2. **Setup Timer**: Ask the UI framework to call a function every frame (~60 times a second).
3. **Tick**: Every frame:
    *   **Update**: Calculates elapsed time and calls `model.update(deltaTime)`.
    *   **Render**: Redraws the screen with updated coordinates.

## 3. Variable Time Step (`deltaTime`)

All movement is calculated using `speed * deltaTime`. This ensures consistent physical speed regardless of refresh rate (60Hz vs 144Hz).

## 4. Application Modes

The game has three modes managed by the UI layer:

| Mode | Description |
|------|-------------|
| `MENU` | Startup screen with "Play Game" / "Watch Demo" options |
| `PLAYING` | Active game; model records Snapshot every frame |
| `REPLAY` | Playback of recorded Snapshot frames (frame-perfect) |

## 5. Recording & Playback System

The demo mode replaces the old AI-based demo with frame-perfect replay:

*   **Recording**: During PLAYING mode, every frame's full game state is captured in a `Snapshot` struct (entity positions, directions, score, lives, power mode, timers). Stored in `std::vector<Snapshot>`.
*   **Persistence**: On game-over or Escape, `saveRecording()` writes snapshots to `~/.pacman_replay` (raw binary). `loadRecording()` in the constructor restores them on next launch.
*   **Playback**: During REPLAY mode, `update()` reads snapshots sequentially, restoring exact positions. Pellet-eating is re-applied so pellets visually disappear as Pacman passes over them.
*   **Memory**: ~160 bytes/frame. A 30-second game at 60fps ≈ 288KB.

## 6. Ghost AI & Scatter/Chase Cycle

Each ghost uses a unique target algorithm (straight-line Euclidean distance to a target tile):

| Ghost | Personality | Target Algorithm |
|-------|-----------|-----------------|
| Blinky (Red) | Shadow | Pacman's current position |
| Pinky (Pink) | Ambusher | 4 tiles ahead of Pacman's direction |
| Inky (Cyan) | Fickle | Vector reflection from Blinky's position |
| Clyde (Orange) | Coward | Pacman if >8 tiles away, else scatter corner |

**Scatter/Chase Cycle**: Every 20 seconds of chase, ghosts switch to scatter mode for 7 seconds (each heads to its home corner), then back to chase. This gives the player periodic breathing room.

## 7. How it Works in Linux (GTK4)

1.  **Application Start (`main`)** — Creates GTK application.
2.  **Initialization (`activate`)** — Sets up window, drawing area, key bindings, and tick callback (`gtk_widget_add_tick_callback`).
3.  **Tick (`on_tick`)** — Calculates deltaTime, calls `model.update()`, redraws via `gtk_widget_queue_draw()`.
4.  **Rendering (`on_draw`)** — Uses Cairo 2D to draw maze walls, pellets, Pacman (animated arc), ghosts (colored semicircle + eyes), and UI overlay.
5.  **Input (`on_key_pressed`)** — Sets `pacman.next_dir` via `model.setPacmanNextDirection()`.
6.  **Menu** — Menu screen drawn with Cairo; arrow keys navigate, Enter confirms.

## 8. How it Works in Android

Built with Jetpack Compose:
- `LaunchedEffect` drives a 60fps game loop with `delay(16)`
- `Canvas` composable draws maze, entities, and UI (mirrors GTK Cairo logic)
- `detectTapGestures` converts tap position to direction relative to Pacman
- Same `GameModel` C++ engine through JNI (`PacmanGame.kt` wrapper)
