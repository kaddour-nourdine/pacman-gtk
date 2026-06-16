# iOS Porting Guide for Pacman
**Author:** Nourdine Kaddour

This document explains how to port the existing Pacman C++ core to iOS using **SwiftUI** and **Objective-C++**.

---

## 1. Prerequisites
- **A Mac** with macOS installed.
- **Xcode** (Latest version from the App Store).
- The shared project files: `gamemodel.cpp` and `gamemodel.h`.

---

## 2. Project Setup in Xcode
1.  **Create New Project:** Open Xcode and select **Create a new Xcode project**.
2.  **Platform:** Select **iOS** and then **App**.
3.  **Options:**
    - **Interface:** SwiftUI.
    - **Language:** Swift.
4.  **Add C++ Files:** Drag and drop `gamemodel.cpp` and `gamemodel.h` from your Linux folder into the Xcode project navigator.
    - *Check "Copy items if needed" and ensure the "Target" is your app.*

---

## 3. Creating the Objective-C++ Bridge

### A. Create the Header (`GameBridge.h`)
```objc
#import <Foundation/Foundation.h>

@interface GameBridge : NSObject
- (void)initGame;
- (void)reset;
- (void)update:(double)deltaTime;
- (void)setDirection:(int)dir;
- (int)getScore;
- (int)getLives;
- (BOOL)isGameOver;
- (BOOL)isDying;
- (BOOL)isPowerMode;
- (BOOL)isScatterMode;
- (BOOL)hasWon;
- (int)getTileX:(int)x Y:(int)y;
- (NSArray<NSNumber *> *)getPacmanPos;
- (NSArray<NSArray<NSNumber *> *> *)getGhosts;
- (void)startRecording;
- (void)stopRecording;
- (void)startPlayback;
- (void)clearRecording;
- (BOOL)hasRecording;
- (BOOL)isReplaying;
@end
```

### B. Create the Implementation (`GameBridge.mm`)
```objc
#import "GameBridge.h"
#include "gamemodel.h"

@implementation GameBridge {
    GameModel* model;
}

- (void)initGame {
    if (model) delete model;
    model = new GameModel();
}

- (void)reset { if (model) model->reset(); }
- (void)update:(double)deltaTime { if (model) model->update(deltaTime); }
- (void)setDirection:(int)dir { if (model) model->setPacmanNextDirection(static_cast<Direction>(dir)); }
- (int)getScore { return model ? model->getScore() : 0; }
- (int)getLives { return model ? model->getLives() : 0; }
- (BOOL)isGameOver { return model ? model->isGameOver() : YES; }
- (BOOL)isDying { return model ? model->isDying() : NO; }
- (BOOL)isPowerMode { return model ? model->isPowerMode() : NO; }
- (BOOL)isScatterMode { return model ? model->isScatterMode() : NO; }
- (BOOL)hasWon { return model ? model->hasWon() : NO; }

- (int)getTileX:(int)x Y:(int)y {
    return model ? static_cast<int>(model->getTile(x, y)) : 0;
}

- (NSArray<NSNumber *> *)getPacmanPos {
    if (!model) return @[@0, @0, @4];
    const Entity& p = model->getPacman();
    return @[@(p.x), @(p.y), @(static_cast<int>(p.dir))];
}

- (NSArray<NSArray<NSNumber *> *> *)getGhosts {
    if (!model) return @[];
    const auto& ghosts = model->getGhosts();
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:ghosts.size()];
    for (const auto& g : ghosts) {
        [result addObject:@[@(g.x), @(g.y), @(static_cast<int>(g.dir)), @(g.frightened ? 1 : 0)]];
    }
    return result;
}

- (void)startRecording { if (model) model->startRecording(); }
- (void)stopRecording { if (model) model->stopRecording(); }
- (void)startPlayback { if (model) model->startPlayback(); }
- (void)clearRecording { if (model) model->clearRecording(); }
- (BOOL)hasRecording { return model ? model->hasRecording() : NO; }
- (BOOL)isReplaying { return model ? model->isReplaying() : NO; }
@end
```

### C. The Bridging Header
Xcode will ask if you want to create a **Bridging Header**. Click **Yes**. In that file (`ProjectName-Bridging-Header.h`), add:
```objc
#import "GameBridge.h"
```

---

## 4. Building the SwiftUI View

### Menu View (`ContentView.swift`)
```swift
import SwiftUI

struct MenuView: View {
    let bridge: GameBridge
    @Binding var appMode: String
    @State private var menuSel = 0

    var body: some View {
        VStack {
            Text("PACMAN").font(.system(size: 52, weight: .bold)).foregroundColor(.yellow)
            Text("by Nourdine Kaddour").font(.caption).foregroundColor(.gray)

            Button("Play Game") {
                bridge.reset()
                bridge.startRecording()
                appMode = "PLAYING"
            }.font(.title2).padding()

            Button("Watch Demo") {
                if bridge.hasRecording() {
                    bridge.startPlayback()
                    appMode = "REPLAY"
                }
            }.font(.title2).padding()

            if !bridge.hasRecording() {
                Text("(no recording - play a game first)").font(.caption2).foregroundColor(.gray)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color.black)
    }
}
```

### Game View (`GameView.swift`)
```swift
import SwiftUI

struct GameView: View {
    let bridge: GameBridge
    let appMode: String
    @State private var score = 0
    @State private var tick = 0
    @State private var gameOverTimer = 0.0
    @State private var prevGameOver = false

    var body: some View {
        Canvas { context, size in
            let _ = tick
            let ts = min(size.width / 23, size.height / 26)
            // Draw maze, pellets, Pacman, ghosts (same logic as Android/GTK)
            // ...
        }
        .background(Color.black)
        .onAppear {
            Timer.scheduledTimer(withTimeInterval: 0.016, repeats: true) { timer in
                if appMode == "REPLAY" && bridge.isGameOver() {
                    if !prevGameOver { gameOverTimer = 4.0; prevGameOver = true }
                    gameOverTimer -= 0.016
                    if gameOverTimer <= 0 { bridge.reset(); prevGameOver = false; appMode = "MENU" }
                } else {
                    bridge.update(0.016)
                    prevGameOver = false
                }
                score = Int(bridge.getScore())
                tick += 1
            }
        }
    }
}
```

---

## 5. Running the App
1.  Select an **iPhone Simulator** from the top toolbar in Xcode.
2.  Click the **Play** button (or press `Cmd + R`).
3.  The app will compile the C++ core and launch the Pacman game on your virtual iPhone.
