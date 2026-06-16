package com.pacman

class PacmanGame {
    companion object {
        init { System.loadLibrary("pacman") }
    }

    external fun init()
    external fun reset()
    external fun update(deltaTime: Double)
    external fun setDirection(dir: Int)
    external fun getScore(): Int
    external fun getLives(): Int
    external fun isGameOver(): Boolean
    external fun isDying(): Boolean
    external fun getTile(x: Int, y: Int): Int
    external fun getPacmanPos(): DoubleArray
    external fun getGhosts(): Array<DoubleArray>
    external fun startRecording()
    external fun stopRecording()
    external fun startPlayback()
    external fun clearRecording()
    external fun hasRecording(): Boolean
    external fun isReplaying(): Boolean
    external fun isPowerMode(): Boolean
    external fun isScatterMode(): Boolean
    external fun hasWon(): Boolean
}

object TileType {
    const val EMPTY = 0
    const val WALL = 1
    const val PELLET = 2
    const val POWER_PELLET = 3
}

object Direction {
    const val UP = 0
    const val DOWN = 1
    const val LEFT = 2
    const val RIGHT = 3
    const val NONE = 4
}
