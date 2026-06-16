# Pacman Android Port

This is the Android version of the Pacman game, reusing the core C++ logic from the main project with a Jetpack Compose UI.

## Features
- Full game engine via JNI (`native-lib.cpp` + shared `gamemodel.cpp`)
- Jetpack Compose Canvas rendering (maze, Pacman, ghosts, pellets, UI)
- 60fps game loop via `LaunchedEffect` + `delay(16)`
- Touch input: tap where you want Pacman to go
- Menu screen: **Play Game** / **Watch Demo** (frame-perfect replay of last game)
- Ghost personalities (Blinky, Pinky, Inky, Clyde) with scatter/chase cycle
- Recording persisted to `~/.pacman_replay` (survives app restart)

## How to Build and Run

### Command Line
```bash
cd android
./gradlew assembleDebug
# APK at android/app/build/outputs/apk/debug/pacman-debug.apk
```

### Android Studio
1. Launch Android Studio → **Open** → select the `android/` directory.
2. **Prerequisites:** Android SDK 34, NDK 27+, CMake 3.22.1 (via SDK Manager).
3. Connect a device or start an emulator → click **Run**.

## Project Structure

```
android/app/src/main/
├── cpp/
│   ├── native-lib.cpp       JNI bridge (22 native methods)
│   └── CMakeLists.txt       CMake build (links gamemodel.cpp)
└── java/com/pacman/
    ├── PacmanGame.kt        JNI wrapper class (native method declarations)
    └── MainActivity.kt      Compose UI (menu, game canvas, game loop, touch input)
```

## JNI API (PacmanGame.kt)

| Method | Description |
|--------|-------------|
| `init()` | Create GameModel instance |
| `reset()` | Reset game state |
| `update(dt)` | Advance game logic |
| `setDirection(dir)` | Set Pacman's next direction |
| `getScore()` / `getLives()` | Player stats |
| `isGameOver()` / `isDying()` | State flags |
| `isPowerMode()` / `isScatterMode()` | Power/scatter state |
| `hasWon()` | Win condition check |
| `getTile(x, y)` | Maze tile type |
| `getPacmanPos()` | `[x, y, dir]` |
| `getGhosts()` | `[[x, y, dir, frightened], ...]` |
| `startRecording()` / `stopRecording()` | Demo recording control |
| `startPlayback()` / `clearRecording()` | Demo playback control |
| `hasRecording()` / `isReplaying()` | Demo state queries |

## Controls
- **Tap** any tile to set Pacman's direction toward it.
- **Menu:** Tap an option to select it.
