#ifndef GAMEMODEL_H
#define GAMEMODEL_H

/**
 * Pacman Game Core Logic
 * Author: Nourdine Kaddour
 */

#include <vector>
#include <string>
#include <cstddef>

enum class TileType {
    EMPTY,
    WALL,
    PELLET,
    POWER_PELLET
};

enum class Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    NONE
};

enum class GhostType {
    BLINKY,
    PINKY,
    INKY,
    CLYDE
};

struct Entity {
    double x, y;
    Direction dir;
    Direction next_dir;
    double speed;
    bool frightened;
    GhostType type;
};

struct Snapshot {
    double px, py;
    Direction pdir, pnext;
    double gx[4], gy[4];
    Direction gdir[4], gnext[4];
    bool frightened[4];
    int score;
    int lives;
    bool powerMode;
    double powerTimer;
    bool dying;
    double dyingTimer;
    bool scatterMode;
    double modeTimer;
    bool gameOver;
};

class GameModel {
public:
    static const int MAP_WIDTH = 21;
    static const int MAP_HEIGHT = 21;

    GameModel();
    void reset();
    void update(double deltaTime);
    void setPacmanNextDirection(Direction dir);

    TileType getTile(int x, int y) const;
    const Entity& getPacman() const { return pacman; }
    const std::vector<Entity>& getGhosts() const { return ghosts; }
    int getScore() const { return score; }
    int getLives() const { return lives; }
    bool isGameOver() const { return gameOver; }
    bool isDying() const { return dying; }
    bool isPowerMode() const { return powerMode; }
    bool isScatterMode() const { return scatterMode; }
    bool hasWon() const;

    void startRecording();
    void stopRecording();
    void startPlayback();
    void clearRecording();
    bool hasRecording() const { return !snapshots.empty(); }
    bool isReplaying() const { return replaying; }

private:
    void resetEntities();
    void moveEntity(Entity& entity, double deltaTime, bool isPacman);
    bool canMove(double x, double y, Direction dir) const;
    void handleCollisions();
    void updateGhosts(double deltaTime);
    void getScatterTarget(GhostType type, double& tx, double& ty) const;
    void recordSnapshot();
    void saveRecording();
    void loadRecording();

    TileType map[MAP_HEIGHT][MAP_WIDTH];
    Entity pacman;
    std::vector<Entity> ghosts;
    int score;
    int lives;
    bool gameOver;
    bool dying;
    double dyingTimer;
    bool powerMode;
    double powerTimer;
    bool scatterMode;
    double modeTimer;

    std::vector<Snapshot> snapshots;
    size_t snapshotIndex;
    bool recording;
    bool replaying;
};

#endif // GAMEMODEL_H
