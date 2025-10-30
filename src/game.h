#ifndef GAME_H
#define GAME_H

#include <SDL.h>
#include <vector>
#include <memory>

class Player;
class Platform;

class Game {
public:
    static void run();

private:
    static constexpr int SCREEN_WIDTH = 800;
    static constexpr int SCREEN_HEIGHT = 600;
};

#endif
