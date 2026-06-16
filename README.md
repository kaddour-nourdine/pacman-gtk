# Pacman

Cross-platform Pacman game with GTK4 (Linux), Android (Compose), and iOS (SwiftUI) ports.

**Author: Nourdine Kaddour**

## Features
- Smooth sub-tile movement with variable time-step physics
- 4 ghost personalities (Blinky, Pinky, Inky, Clyde) with scatter/chase cycle
- Power pellets and frightened mode
- Score and lives tracking
- Startup menu: **Play Game** / **Watch Demo** (replay of last played game)
- Frame-perfect recording and playback persisted to disk (`~/.pacman_replay`)
- Animated Pacman mouth, colored ghost eyes
- Touch input on mobile (tap where you want Pacman to go)
- 22 unit tests covering movement, ghost AI, scatter mode, recording/playback

## How to Build

### Linux (GTK4)
```bash
sudo apt install libgtk-4-dev cmake g++
g++ -std=c++17 -o build/pacman-gtk main_gtk.cpp gamemodel.cpp $(pkg-config --cflags --libs gtk4)
./build/pacman-gtk
```

### Android
```bash
cd android
./gradlew assembleDebug
# APK at android/app/build/outputs/apk/debug/pacman-debug.apk
```

### Tests
```bash
g++ -std=c++17 -I. -o build/test_gamemodel tests/test_gamemodel.cpp gamemodel.cpp
./build/test_gamemodel
```

## Controls

| Platform | Move Pacman | Confirm Menu | Back to Menu |
|----------|-----------|-------------|--------------|
| Linux (GTK) | Arrow keys / WASD | Enter / Space | Escape |
| Android | Tap destination tile | Tap option | - |
| Menu (GTK) | Up/Down to select | Enter to confirm | Escape to quit |

- **R**: Restart after Game Over
