#include "game.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "Simple Platformer Game" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Arrow Keys or A/D - Move left/right" << std::endl;
    std::cout << "  Space or W - Jump (press again in air for double jump!)" << std::endl;
    std::cout << "  ESC - Quit" << std::endl;

    Game::run();

    return 0;
}
