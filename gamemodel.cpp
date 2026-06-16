#include "gamemodel.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <string>

static const char* initial_map[GameModel::MAP_HEIGHT] = {
    "#####################",
    "#.........#.........#",
    "#.###.###.#.###.###.#",
    "#O###.###.#.###.###O#",
    "#.###.###.#.###.###.#",
    "#...................#",
    "#.###.#.#####.#.###.#",
    "#.###.#.#####.#.###.#",
    "#.....#...#...#.....#",
    "#####.### # ###.#####",
    "......#       #......",
    "#####.# ##### #.#####",
    "#.....#   #   #.....#",
    "#.###.# ##### #.###.#",
    "#.###.#       #.###.#",
    "#.........#.........#",
    "#.###.###.#.###.###.#",
    "#O..#.....P.....#..O#",
    "###.#.#.#####.#.#.###",
    "#.....#...#...#.....#",
    "#####################"
};

GameModel::GameModel() : recording(false), replaying(false), snapshotIndex(0) {
    std::srand(std::time(nullptr));
    reset();
    loadRecording();
}

void GameModel::reset() {
    score = 0;
    lives = 3;
    gameOver = false;
    dying = false;
    dyingTimer = 0;
    powerMode = false;
    powerTimer = 0;
    scatterMode = false;
    modeTimer = 0.0;

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            char c = initial_map[y][x];
            switch (c) {
                case '#': map[y][x] = TileType::WALL; break;
                case '.': map[y][x] = TileType::PELLET; break;
                case 'O': map[y][x] = TileType::POWER_PELLET; break;
                case 'P': map[y][x] = TileType::EMPTY; break;
                case ' ': map[y][x] = TileType::EMPTY; break;
                default: map[y][x] = TileType::EMPTY; break;
            }
        }
    }

    resetEntities();
}

void GameModel::resetEntities() {
    pacman = { 10.0, 17.0, Direction::NONE, Direction::NONE, 5.5, false, GhostType::BLINKY };
    
    ghosts.clear();
    ghosts.push_back({ 9.0, 10.0, Direction::NONE, Direction::NONE, 4.0, false, GhostType::BLINKY });
    ghosts.push_back({ 10.0, 10.0, Direction::NONE, Direction::NONE, 4.0, false, GhostType::PINKY });
    ghosts.push_back({ 11.0, 10.0, Direction::NONE, Direction::NONE, 4.0, false, GhostType::INKY });
    ghosts.push_back({ 10.0, 9.0, Direction::NONE, Direction::NONE, 4.0, false, GhostType::CLYDE });
}

void GameModel::update(double deltaTime) {
    if (replaying) {
        if (snapshotIndex >= snapshots.size()) {
            gameOver = true;
            replaying = false;
            return;
        }
        const Snapshot& s = snapshots[snapshotIndex++];
        pacman.x = s.px; pacman.y = s.py;
        pacman.dir = s.pdir; pacman.next_dir = s.pnext;
        for (int i = 0; i < 4; ++i) {
            ghosts[i].x = s.gx[i]; ghosts[i].y = s.gy[i];
            ghosts[i].dir = s.gdir[i]; ghosts[i].next_dir = s.gnext[i];
            ghosts[i].frightened = s.frightened[i];
        }
        score = s.score; lives = s.lives;
        powerMode = s.powerMode; powerTimer = s.powerTimer;
        dying = s.dying; dyingTimer = s.dyingTimer;
        scatterMode = s.scatterMode; modeTimer = s.modeTimer;
        gameOver = s.gameOver;
        int px = (int)std::round(pacman.x);
        int py = (int)std::round(pacman.y);
        if (px >= 0 && px < MAP_WIDTH && py >= 0 && py < MAP_HEIGHT) {
            if (map[py][px] == TileType::PELLET) map[py][px] = TileType::EMPTY;
            else if (map[py][px] == TileType::POWER_PELLET) map[py][px] = TileType::EMPTY;
        }
        return;
    }

    if (gameOver) {
        if (recording) {
            recording = false;
            saveRecording();
        }
        return;
    }

    if (dying) {
        dyingTimer -= deltaTime;
        if (dyingTimer <= 0) {
            dying = false;
            if (lives <= 0) {
                gameOver = true;
            } else {
                resetEntities();
            }
        }
        if (recording) recordSnapshot();
        return;
    }

    moveEntity(pacman, deltaTime, true);

    int px = (int)std::round(pacman.x);
    int py = (int)std::round(pacman.y);
    if (px >= 0 && px < MAP_WIDTH && py >= 0 && py < MAP_HEIGHT) {
        if (map[py][px] == TileType::PELLET) {
            map[py][px] = TileType::EMPTY;
            score += 10;
        } else if (map[py][px] == TileType::POWER_PELLET) {
            map[py][px] = TileType::EMPTY;
            score += 50;
            powerMode = true;
            powerTimer = 10.0;
            for(auto& ghost : ghosts) ghost.frightened = true;
        }
    }

    modeTimer += deltaTime;
    if (scatterMode) {
        if (modeTimer >= 7.0) {
            scatterMode = false;
            modeTimer = 0.0;
        }
    } else {
        if (modeTimer >= 20.0) {
            scatterMode = true;
            modeTimer = 0.0;
        }
    }

    updateGhosts(deltaTime);

    if (powerMode) {
        powerTimer -= deltaTime;
        if (powerTimer <= 0) {
            powerMode = false;
            for(auto& ghost : ghosts) ghost.frightened = false;
        }
    }

    handleCollisions();

    bool win = true;
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            if (map[y][x] == TileType::PELLET || map[y][x] == TileType::POWER_PELLET) {
                win = false;
                break;
            }
        }
        if (!win) break;
    }
    if (win) gameOver = true;

    if (recording) recordSnapshot();
}

void GameModel::setPacmanNextDirection(Direction dir) {
    pacman.next_dir = dir;
}

void GameModel::moveEntity(Entity& entity, double deltaTime, bool isPacman) {
    if (isPacman && entity.next_dir != Direction::NONE && entity.dir != Direction::NONE) {
        bool opposite = false;
        if (entity.dir == Direction::UP && entity.next_dir == Direction::DOWN) opposite = true;
        else if (entity.dir == Direction::DOWN && entity.next_dir == Direction::UP) opposite = true;
        else if (entity.dir == Direction::LEFT && entity.next_dir == Direction::RIGHT) opposite = true;
        else if (entity.dir == Direction::RIGHT && entity.next_dir == Direction::LEFT) opposite = true;
        
        if (opposite) {
            entity.dir = entity.next_dir;
            entity.next_dir = Direction::NONE;
        }
    }

    double gridX = std::round(entity.x);
    double gridY = std::round(entity.y);
    double dist = std::sqrt(std::pow(entity.x - gridX, 2) + std::pow(entity.y - gridY, 2));

    if (dist < 0.3 && entity.next_dir != Direction::NONE) {
        if (entity.next_dir != entity.dir) {
            if (canMove(gridX, gridY, entity.next_dir)) {
                entity.x = gridX;
                entity.y = gridY;
                entity.dir = entity.next_dir;
                entity.next_dir = Direction::NONE;
            }
        } else {
            entity.next_dir = Direction::NONE;
        }
    }

    if (entity.dir == Direction::NONE) {
        if (entity.next_dir != Direction::NONE && canMove(gridX, gridY, entity.next_dir)) {
            entity.dir = entity.next_dir;
            entity.next_dir = Direction::NONE;
        } else {
            return;
        }
    }

    double moveDist = entity.speed * deltaTime;
    double nx = entity.x;
    double ny = entity.y;

    switch (entity.dir) {
        case Direction::UP: ny -= moveDist; break;
        case Direction::DOWN: ny += moveDist; break;
        case Direction::LEFT: nx -= moveDist; break;
        case Direction::RIGHT: nx += moveDist; break;
        default: break;
    }

    if (nx < -0.5) nx = MAP_WIDTH - 0.5;
    else if (nx > MAP_WIDTH - 0.5) nx = -0.5;

    if (!canMove(entity.x, entity.y, entity.dir)) {
        if (dist < moveDist) {
            entity.x = gridX;
            entity.y = gridY;
            entity.dir = Direction::NONE;
            return;
        }
    }

    entity.x = nx;
    entity.y = ny;
}

bool GameModel::canMove(double x, double y, Direction dir) const {
    int gx = (int)std::round(x);
    int gy = (int)std::round(y);

    switch (dir) {
        case Direction::UP: gy--; break;
        case Direction::DOWN: gy++; break;
        case Direction::LEFT: gx--; break;
        case Direction::RIGHT: gx++; break;
        default: return true;
    }

    if (gx < 0 || gx >= MAP_WIDTH) return true;
    if (gy < 0 || gy >= MAP_HEIGHT) return false;

    return map[gy][gx] != TileType::WALL;
}

void GameModel::recordSnapshot() {
    Snapshot s;
    s.px = pacman.x; s.py = pacman.y;
    s.pdir = pacman.dir; s.pnext = pacman.next_dir;
    for (int i = 0; i < 4; ++i) {
        s.gx[i] = ghosts[i].x; s.gy[i] = ghosts[i].y;
        s.gdir[i] = ghosts[i].dir; s.gnext[i] = ghosts[i].next_dir;
        s.frightened[i] = ghosts[i].frightened;
    }
    s.score = score; s.lives = lives;
    s.powerMode = powerMode; s.powerTimer = powerTimer;
    s.dying = dying; s.dyingTimer = dyingTimer;
    s.scatterMode = scatterMode; s.modeTimer = modeTimer;
    s.gameOver = gameOver;
    snapshots.push_back(s);
}

void GameModel::startRecording() {
    snapshots.clear();
    recording = true;
    replaying = false;
    snapshotIndex = 0;
}

void GameModel::stopRecording() {
    if (recording) {
        recording = false;
        saveRecording();
    }
}

void GameModel::clearRecording() {
    snapshots.clear();
    recording = false;
}

void GameModel::startPlayback() {
    if (snapshots.empty()) return;
    reset();
    replaying = true;
    recording = false;
    snapshotIndex = 0;
}

bool GameModel::hasWon() const {
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x)
            if (map[y][x] == TileType::PELLET || map[y][x] == TileType::POWER_PELLET)
                return false;
    return true;
}

void GameModel::getScatterTarget(GhostType type, double& tx, double& ty) const {
    switch (type) {
        case GhostType::BLINKY: tx = 19; ty = 1;  break;
        case GhostType::PINKY:  tx = 1;  ty = 1;  break;
        case GhostType::INKY:   tx = 19; ty = 19; break;
        case GhostType::CLYDE:  tx = 1;  ty = 19; break;
    }
}

void GameModel::updateGhosts(double deltaTime) {
    Direction dirs[] = { Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT };

    Entity* blinky = nullptr;
    for (auto& g : ghosts) {
        if (g.type == GhostType::BLINKY) { blinky = &g; break; }
    }

    for (auto& ghost : ghosts) {
        double gridX = std::round(ghost.x);
        double gridY = std::round(ghost.y);
        double dist = std::sqrt(std::pow(ghost.x - gridX, 2) + std::pow(ghost.y - gridY, 2));

        if (dist < 0.2) {
            std::vector<Direction> validDirs;
            for (auto d : dirs) {
                if (ghost.dir != Direction::NONE) {
                    if ((ghost.dir == Direction::UP && d == Direction::DOWN) ||
                        (ghost.dir == Direction::DOWN && d == Direction::UP) ||
                        (ghost.dir == Direction::LEFT && d == Direction::RIGHT) ||
                        (ghost.dir == Direction::RIGHT && d == Direction::LEFT)) continue;
                }
                if (canMove(gridX, gridY, d)) validDirs.push_back(d);
            }

            if (validDirs.empty()) {
                if (ghost.dir == Direction::UP) ghost.dir = Direction::DOWN;
                else if (ghost.dir == Direction::DOWN) ghost.dir = Direction::UP;
                else if (ghost.dir == Direction::LEFT) ghost.dir = Direction::RIGHT;
                else if (ghost.dir == Direction::RIGHT) ghost.dir = Direction::LEFT;
                moveEntity(ghost, deltaTime, false);
                continue;
            }

            Direction bestDir = validDirs[0];

            if (ghost.frightened) {
                bestDir = validDirs[std::rand() % validDirs.size()];
            } else {
                double targetX, targetY;

                if (scatterMode) {
                    getScatterTarget(ghost.type, targetX, targetY);
                } else {
                    switch (ghost.type) {
                        case GhostType::BLINKY:
                            targetX = pacman.x;
                            targetY = pacman.y;
                            break;

                        case GhostType::PINKY: {
                            targetX = pacman.x;
                            targetY = pacman.y;
                            switch (pacman.dir) {
                                case Direction::UP:    targetY -= 4; targetX -= 4; break;
                                case Direction::DOWN:  targetY += 4; break;
                                case Direction::LEFT:  targetX -= 4; break;
                                case Direction::RIGHT: targetX += 4; break;
                                case Direction::NONE: break;
                            }
                            break;
                        }

                        case GhostType::INKY: {
                            double aheadX = pacman.x;
                            double aheadY = pacman.y;
                            switch (pacman.dir) {
                                case Direction::UP:    aheadY -= 2; aheadX -= 2; break;
                                case Direction::DOWN:  aheadY += 2; break;
                                case Direction::LEFT:  aheadX -= 2; break;
                                case Direction::RIGHT: aheadX += 2; break;
                                case Direction::NONE: break;
                            }
                            if (blinky) {
                                targetX = aheadX + (aheadX - blinky->x);
                                targetY = aheadY + (aheadY - blinky->y);
                            } else {
                                targetX = aheadX;
                                targetY = aheadY;
                            }
                            break;
                        }

                        case GhostType::CLYDE: {
                            double dx = pacman.x - ghost.x;
                            double dy = pacman.y - ghost.y;
                            double distToPacman = std::sqrt(dx*dx + dy*dy);
                            if (distToPacman > 8.0) {
                                targetX = pacman.x;
                                targetY = pacman.y;
                            } else {
                                getScatterTarget(ghost.type, targetX, targetY);
                            }
                            break;
                        }
                    }
                }

                double minDist = 1e9;
                for (auto d : validDirs) {
                    double nx = gridX, ny = gridY;
                    if (d == Direction::UP) ny--;
                    else if (d == Direction::DOWN) ny++;
                    else if (d == Direction::LEFT) nx--;
                    else if (d == Direction::RIGHT) nx++;
                    double d2 = std::pow(nx - targetX, 2) + std::pow(ny - targetY, 2);
                    if (d2 < minDist) { minDist = d2; bestDir = d; }
                }
            }

            ghost.dir = bestDir;
        }
        moveEntity(ghost, deltaTime, false);
    }
}

void GameModel::handleCollisions() {
    if (dying) return;

    for (auto& ghost : ghosts) {
        double dx = pacman.x - ghost.x;
        double dy = pacman.y - ghost.y;
        if (dx*dx + dy*dy < 0.5) {
            if (powerMode && ghost.frightened) {
                ghost.x = 10.0; ghost.y = 10.0;
                ghost.frightened = false;
                score += 200;
            } else if (!ghost.frightened) {
                lives--;
                dying = true;
                dyingTimer = 1.5;
                break;
            }
        }
    }
}

TileType GameModel::getTile(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return TileType::WALL;
    return map[y][x];
}

void GameModel::saveRecording() {
    if (snapshots.empty()) return;
    const char* home = getenv("HOME");
    if (!home) return;
    std::string path = std::string(home) + "/.pacman_replay";
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return;
    size_t count = snapshots.size();
    std::fwrite(&count, sizeof(count), 1, f);
    std::fwrite(snapshots.data(), sizeof(Snapshot), count, f);
    std::fclose(f);
}

void GameModel::loadRecording() {
    const char* home = getenv("HOME");
    if (!home) return;
    std::string path = std::string(home) + "/.pacman_replay";
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return;
    size_t count;
    if (std::fread(&count, sizeof(count), 1, f) != 1) { std::fclose(f); return; }
    snapshots.resize(count);
    if (std::fread(snapshots.data(), sizeof(Snapshot), count, f) != count) {
        snapshots.clear();
    }
    std::fclose(f);
}
