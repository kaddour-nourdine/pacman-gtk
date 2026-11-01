# TODO — Bug fixes for pacman

Ordered by severity-if-triggered ÷ likelihood-of-being-triggered, so the cheap-and-visible
items come first. Every entry lists where it lives, why it happens, and how to confirm it's fixed.

Status key: `[ ]` open · `[~]` in progress · `[x]` done

---

## P0 — States you cannot recover from

### [ ] 1. Unclamped `deltaTime` lets entities leave the maze
**Where:** `main_gtk.cpp:207-209` (source), `gamemodel.cpp:228` (amplifier)

`on_tick` feeds the raw frame delta into the model. GTK4 frame clocks stop while a window is
occluded or minimized, so `last_time` goes stale and the first frame back carries the entire
occlusion duration into `moveDist = speed * deltaTime`. At `dt = 2.0` that is 11 tiles of movement
in one frame, and `canMove` only inspects the tile adjacent to the *current* rounded position —
so every wall in between is skipped.

Reproduced: pacman went from `(10.88, 17.0)` to `(-0.5, 17.0)` in a single `update(2.0)`, then to
`(20.5, -632.0)` after repeated stalled frames.

**Fix** — clamp in `on_tick`, which is the only place a real delta enters the system:

```cpp
double deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - last_time).count() / 1000000.0;
deltaTime = std::min(deltaTime, 0.05);   // never simulate more than 3 frames at once
last_time = now;
```

Needs `#include <algorithm>`. Do not clamp inside `GameModel::update` — tests feed arbitrary
deltas on purpose and should keep doing so.

**Verify:** start a game, minimize for ~5s, restore. Pacman must be on a walkable tile and must
not have jumped. Also covered by test 16 below.

---

### [ ] 2. `canMove` returns `true` for every direction when horizontally out of bounds
**Where:** `gamemodel.cpp:268`

```cpp
if (gx < 0 || gx >= MAP_WIDTH) return true;   // ignores `dir` and `gy` entirely
```

The tunnel wrap at `gamemodel.cpp:240-241` parks entities at exactly `x = -0.5` or `x = 20.5`, where
`std::round` yields `-1` / `21` — out of range. At that instant `UP` and `DOWN` are also reported
as legal even though rows 9 and 11 at the tunnel mouth are walls, which is what lets an entity
escape vertically. The `dist < 0.3` turn guard happens to prevent a normal player from turning at
that exact instant, so this is only reachable via bug 1 or corrupt data — but it is a landmine.

**Fix** — the wrap is horizontal-only, so say so, and refuse vertical moves from outside the map:

```cpp
if (!std::isfinite(x) || !std::isfinite(y)) return false;
if (gy < 0 || gy >= MAP_HEIGHT) return false;
if (gx < 0 || gx >= MAP_WIDTH)
    return dir == Direction::LEFT || dir == Direction::RIGHT;   // tunnel only
return map[gy][gx] != TileType::WALL;
```

The `isfinite` guard matters: NaN comparisons are always false, so NaN coordinates from a corrupt
recording fall straight through to `map[gy][gx]` and read out of bounds.

**Verify:** unit test asserting `canMove` semantics for every direction at `x = -0.5`, `x = 20.5`,
`y = -0.5`, and NaN — needs `canMove` promoted to a public/testable seam, or test via `update`.

---

### [ ] 3. Unvalidated `count` from the replay file aborts the process
**Where:** `gamemodel.cpp:490-502`

```cpp
size_t count;
std::fread(&count, sizeof(count), 1, f);
snapshots.resize(count);      // no bound, no magic, no version
```

An 8-byte file containing a large integer makes `resize` throw `std::length_error`; nothing in the
program catches it, so the process terminates. Reproduced: `terminate called after throwing an
instance of 'std::length_error'` → abort, core dumped.

Compounding it: the file has no magic or version header, so a recording written by a build with a
different `Snapshot` layout (or by anything else at that path) is parsed as raw structs and feeds
garbage coordinates into the model — which is what turns into the OOB read in bug 2.

**Fix**

- Write a magic + version header (`0x5041434D414E3031ULL`, version 1) on save; refuse to load a
  file without it. Old recordings can be discarded — they are not worth a migration path.
- Clamp on load: `if (count == 0 || count > kMaxSnapshots) { fclose(f); snapshots.clear(); return; }`
  with `kMaxSnapshots` sized to something reachable (2h at 120 Hz ≈ 864k).
- Reject any snapshot whose pacman/ghost coordinates are not finite and within
  `[-1, MAP_WIDTH]` × `[-1, MAP_HEIGHT]`.
- Wrap the `resize` in `try/catch (const std::bad_alloc&)` and clear on failure.

**Verify:** write `~/.pacman_replay` containing 8 bytes of `0xFF`; the game must start normally.
Test this, plus a valid-header/lying-count case.

---

## P1 — Wrong results in normal play

### [ ] 4. Clearing the maze reports a loss
**Where:** `gamemodel.cpp:179` (cause), `main_gtk.cpp:175-181` (symptom), `gamemodel.h:80` (dead code)

`update()` sets `gameOver = true` when the board is clear, and the UI unconditionally prints
`GAME OVER (Press R to restart)`. `hasWon()` exists and is called from nowhere.

**Fix** — add a `bool won` member, set it where the win is detected, leave `gameOver` for real
losses, and branch the UI text on it. This also gives `hasWon()` a purpose, or it can be deleted
in favour of the flag.

```cpp
if (win) { won = true; gameOver = true; }
```

```cpp
if (model.hasWon())        text = "YOU WIN!  (Press R to play again)";
else if (appMode == AppMode::DEMO) text = "REPLAY OVER";
else                        text = "GAME OVER (Press R to restart)";
```

Reset `won` in `reset()`. Add it to `Snapshot` if the win animation should replay in the demo.

**Verify:** play through to a clear; the message must say win. Cheapest way to force it: expose a
test seam that clears the board, or add a debug key.

---

### [ ] 5. Kill radius is 0.71 tiles, so Pacman dies with a visible gap
**Where:** `gamemodel.cpp:457`

```cpp
if (dx*dx + dy*dy < 0.5)      // true out to 0.707 tiles
```

Sprites are 0.43 tiles in radius, so contact happens at ~0.87 tiles. Death therefore fires while
there is a visible gap between the two characters, and the same radius is also used to eat ghosts
(`powerMode && ghost.frightened`) — so the game is lenient when it helps you and harsh when it
hurts you.

**Fix** — collide on shared tile, as the original does:

```cpp
int px = (int)std::round(pacman.x), py = (int)std::round(pacman.y);
int gx = (int)std::round(ghost.x), gy = (int)std::round(ghost.y);
if (px == gx && py == gy) { ... }
```

If a little forgiveness is wanted, use a small explicit radius (~0.25) rather than 0.71, and
document why.

**Verify:** a script that walks Pacman into a stationary ghost and asserts death occurs at the tile
boundary, not before.

---

### [ ] 6. Demo mode skips collision handling entirely
**Where:** `gamemodel.cpp:103` (early return) vs `gamemodel.cpp:167` (`handleCollisions()`)

Playback applies a snapshot and returns, so ghosts walk through Pacman without consequence during
"Watch Demo". Recorded deaths still replay, because `dying`/`lives` come from the snapshot — but
live collisions never fire. The README's "frame-perfect recording and playback" overstates this.

**Decide which you want:**

- *(a) Intentional* — a demo that cannot be interrupted is reasonable. Then make it explicit with a
  `if (replaying) return;` guard at the top of `handleCollisions()` plus a comment, and correct the
  README claim.
- *(b) Faithful* — call `handleCollisions()` on the playback path. Positions during playback are
  identical to the recording, so it reproduces the recorded collisions exactly; the risk is
  double-counting, since a snapshot that already carries `dying = true` and decremented `lives`
  would decrement again. Gate the mutation on `replaying` if you take this path.

Option (a) is the smaller change. Do not leave this implicit either way.

---

## P2 — Test suite gives false confidence

### [ ] 7. Tests read and overwrite the real `~/.pacman_replay`
**Where:** `gamemodel.cpp:477-502`, `tests/test_gamemodel.cpp`

`saveRecording()` targets `getenv("HOME")`, and the constructor calls `loadRecording()`. Running the
suite therefore destroys the player's actual last game, and results depend on whatever file happens
to be on the machine — tests are not hermetic and not repeatable.

**Fix** — resolve the path through an override that tests can set:

```cpp
static std::string replayPath() {
    const char* override = getenv("PACMAN_REPLAY");
    return override ? std::string(override) : std::string(getenv("HOME")) + "/.pacman_replay";
}
```

Have the test `main` call `setenv("PACMAN_REPLAY", "<tmpdir>/replay", 1)` before constructing any
`GameModel`.

### [ ] 8. One test asserts nothing
**Where:** `tests/test_gamemodel.cpp:60-76`

`test_scatter_mode_initial_state` prints `INFO:` and passes unconditionally. Make it deterministic
instead — drive `modeTimer` directly, or test the mode transition with ghosts parked far away, and
assert `isScatterMode()` flips after 20s of chase and back after 7s of scatter.

### [ ] 9. Harness aborts on first failure
**Where:** `tests/test_gamemodel.cpp:5-11, 168-186`

`ASSERT_MSG` calls `exit(1)`, so one failure hides every later result, and the `try/catch` in `main`
can never fire because nothing throws. Track a failure count, keep going, return non-zero at the
end, and either drop the `try` or use it for real (`std::bad_alloc` from bug 3 is a live case).

### [ ] 10. No behavioural tests
Every existing test re-checks internals the code already guarantees. Add assertions on behaviour:

- Pacman is never inside a `WALL` tile after any update, over a long random walk
- Same, at `dt` = 0.008 / 0.016 / 0.033 / 0.2 — this is what catches bugs 1 and 5
- `update(2.0)` does not move anything off-map (catches bug 1)
- Corrupt replay file → no crash (catches bug 3)
- Board cleared → win state (catches bug 4)
- Playback finishes with identical score and lives to the recording

---

## P3 — Resource leaks and polish

### [ ] 11. Recording grows without bound
One 192-byte snapshot per frame, no cap (`gamemodel.cpp:288`), and the whole file is loaded into RAM
at startup. A 30-minute game at 60 Hz ≈ 21 MB. Options: decimate to a fixed rate (e.g. 30 Hz), cap
total snapshots, or drop the on-disk feature. Whichever, stop recording rather than growing.

### [ ] 12. `clearRecording()` does not delete the file
`gamemodel.cpp:305` clears the vector only, so a "cleared" recording reappears on next launch.
Delete the file too, or rename the method to match what it does.

### [ ] 13. Two ghosts can be eaten in one frame
`gamemodel.cpp:454-468` — the eat branch does not `break`, so every ghost inside the radius is
scored in the same frame. Add `break`, or document the combo as intentional.

### [ ] 14. `srand(time(nullptr))` in the constructor
`gamemodel.cpp:33` — every `GameModel` built in the same second gets an identical seed, which makes
ghost behaviour and the test suite order-dependent. Inject a seed, or use a per-instance RNG.

### [ ] 15. Frame-rate dependence in the turn thresholds
`moveEntity` turns within `dist < 0.3` (`gamemodel.cpp:206`), `updateGhosts` within `dist < 0.2`
(`gamemodel.cpp:348`). Two different windows for the same operation, both absolute, so ghosts and
Pacman resolve corners at different points and the game plays differently at 60 vs 144 Hz — which
contradicts the README's "variable time-step physics" claim. Unify to one threshold, and express it
relative to `moveDist` where possible.

### [ ] 16. Closing the window mid-game discards the recording
`stopRecording()` is only called from the Escape handler (`main_gtk.cpp:282`). There is no
`delete-event` handler, so quitting from the window manager loses the game in progress. Add one and
call `stopRecording()`.

### [ ] 17. Window resize crops the maze
`on_draw` ignores `width`/`height` (`main_gtk.cpp:191`) and everything is drawn at a fixed
`TILE_SIZE`. Either scale to the available area or set a non-resizable window.

### [ ] 18. Repo hygiene
No `.gitignore`, which is how `build/pacman-gtk`, `android/gradle-8.10.2` and `android/.gradle` got
committed and later had to be removed in cleanup commits. Add:

```
build/
android/build/
android/app/build/
android/.gradle/
android/gradle/
*.o
```

### [ ] 19. `-Wreorder` warning
`gamemodel.cpp:32` initialises `snapshotIndex` before `recording`/`replaying`, which are declared
first. Reorder the init list to match declaration order (`snapshotIndex`, `recording`, `replaying`).

### [ ] 20. README overstates things
- "22 unit tests" — there are 11 test functions (26 `ASSERT_MSG` calls).
- "Frame-perfect recording and playback" — see bug 6.
- Build commands: the `g++` one-liners work, but `cmake -B build && cmake --build build` is the
  documented path in `CMakeLists.txt` and should be the one shown.

---

## Suggested order of work

1. Bugs 1 + 2 together (same root cause, small diff, removes the only unrecoverable state)
2. Bug 3 (crash on startup; pairs with a new test)
3. Bug 4 (visible in normal play)
4. Bugs 7 + 10 (make the suite safe to run, then let it catch the rest)
5. Everything else