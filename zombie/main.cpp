#include "game.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "Zombie Maze Shooter" << std::endl;
    std::cout << "===================" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  WASD or Arrow Keys - Move" << std::endl;
    std::cout << "  Left Mouse Button - Shoot" << std::endl;
    std::cout << "  R - Respawn/Restart (after death or win)" << std::endl;
    std::cout << "  ESC - Quit" << std::endl;
    std::cout << std::endl;
    std::cout << "Objective:" << std::endl;
    std::cout << "  - Collect all 3 keys (yellow squares)" << std::endl;
    std::cout << "  - Shoot the zombies (green squares)" << std::endl;
    std::cout << "  - Reach the exit door (green tile in bottom-right)" << std::endl;
    std::cout << "  - Don't let zombies catch you!" << std::endl;
    std::cout << std::endl;
    std::cout << "Features:" << std::endl;
    std::cout << "  - Randomly generated maze each life!" << std::endl;
    std::cout << "  - Auto-respawn after 2 seconds on death" << std::endl;
    std::cout << "  - Press R to respawn immediately" << std::endl;
    std::cout << std::endl;

    Game::run();

    return 0;
}
