#include "game.h"
#include "player.h"
#include "platform.h"
#include <iostream>

void Game::run() {
    // Init
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }

    auto window = SDL_CreateWindow("Simple Platformer",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   SCREEN_WIDTH,
                                   SCREEN_HEIGHT,
                                   SDL_WINDOW_SHOWN);

    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    // Create player
    auto player = std::make_unique<Player>(100.0f, 100.0f);

    // Create platforms
    std::vector<std::unique_ptr<Platform>> platforms;
    platforms.push_back(std::make_unique<Platform>(0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, 50));
    platforms.push_back(std::make_unique<Platform>(200, 450, 200, 30));
    platforms.push_back(std::make_unique<Platform>(500, 350, 200, 30));
    platforms.push_back(std::make_unique<Platform>(100, 250, 150, 30));
    platforms.push_back(std::make_unique<Platform>(450, 200, 180, 30));

    // Game loop
    bool running = true;
    Uint32 lastTime = SDL_GetTicks();

    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Cap delta time to prevent huge jumps
        if (deltaTime > 0.05f) {
            deltaTime = 0.05f;
        }

        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }

        // Update
        const Uint8* keyState = SDL_GetKeyboardState(nullptr);
        player->handleInput(keyState);
        player->update(deltaTime);

        // Check collisions with all platforms
        for (const auto& platform : platforms) {
            player->checkCollision(*platform);
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
        SDL_RenderClear(renderer);

        for (const auto& platform : platforms) {
            platform->render(renderer);
        }

        player->render(renderer);

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
