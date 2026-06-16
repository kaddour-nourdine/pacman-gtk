# Proposed Improvements for Pacman
**Author:** Nourdine Kaddour

This document outlines the roadmap for future development of the cross-platform Pacman engine.

---

## 1. Gameplay & AI Enhancements (Core Logic)

### ✅ True Ghost Personalities (Implemented)
- **Blinky (Red):** *The Shadow.* Targets Pacman's current position. Scatter corner: top-right.
- **Pinky (Pink):** *The Ambusher.* Targets 4 tiles ahead of Pacman's direction. Scatter corner: top-left.
- **Inky (Cyan):** *The Fickle.* Uses vector reflection from Blinky to flank. Scatter corner: bottom-right.
- **Clyde (Orange):** *The Coward.* Hunts Pacman until within 8 tiles, then retreats to scatter corner (bottom-left).

### ✅ Scatter/Chase Cycle (Implemented)
Every 20s chase → 7s scatter → repeat. Ghosts head to their corners during scatter, giving the player a break.

### ✅ Demo Mode (Recording/Playback — Implemented)
Frame-perfect replay of the last played game (not AI-driven). Persisted to `~/.pacman_replay`.

### ❌ Future Mechanics
- **Level Progression:** Reset map on win, keep score/lives, increase ghost speed 5% per level.
- **Fruit Spawning:** Spawn bonus fruit after 70 and 170 pellets eaten.

---

## 2. Android Mobile Experience
### ✅ Touch Controls (Implemented)
Tap destination tile — direction calculated relative to Pacman's current position.

### ❌ Future Enhancements
- **Swipe Controls:** Use `Modifier.pointerInput` for swipe detection (removes need for tap-based direction).
- **Haptic Feedback:** Vibrate on pellet eat and life loss.
- **High Score Persistence:** Use `SharedPreferences` for permanent high score.

---

## 3. Visuals & Polish
- **Sprite Animation:** Replace geometric shapes with PNG sprites.
    - Animate Pacman's mouth more smoothly.
    - Ghost eyes already implemented.
- **Sound Effects:** Integrate `miniaudio` for waka-waka, siren, and death jingle.

---

## 4. Technical Architecture
- **External Map Files:** Move map strings out of `gamemodel.cpp` into a `.txt` file for runtime maze loading.
- **Difficulty Settings:** Add Easy/Normal/Hard toggle to adjust `initialGhostSpeed`.
- **Unit Testing:** 22 tests implemented covering movement, ghost AI, scatter mode, and recording/playback.
