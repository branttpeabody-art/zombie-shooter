#ifndef ZOMBIE_GAME_H
#define ZOMBIE_GAME_H

#include <SDL.h>

enum class GameState {
    MENU,
    MAZE_TYPE_SELECT,
    DIFFICULTY_SELECT,
    CODE_ENTRY,
    CONTROLS,
    PLAYING,
    PAUSED,
    GAME_WON,
    GAME_LOST
};

enum class Difficulty {
    EASY,
    NORMAL,
    HARD,
    TESTING
};

class Game {
public:
    static void run();

    static constexpr int SCREEN_WIDTH = 960;   // 32 tiles * 30 pixels
    static constexpr int SCREEN_HEIGHT = 720;  // 24 tiles * 30 pixels
    static constexpr int KEYS_NEEDED = 5;  // Increased from 3
};

#endif
