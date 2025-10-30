#ifndef PLAYER_H
#define PLAYER_H

#include <SDL.h>

class Platform;

class Player {
public:
    Player(float x, float y);

    void handleInput(const Uint8* keyState);
    void update(float deltaTime);
    void render(SDL_Renderer* renderer);

    void checkCollision(const Platform& platform);

    float getX() const { return x; }
    float getY() const { return y; }
    float getWidth() const { return width; }
    float getHeight() const { return height; }

private:
    float x, y;
    float velocityX, velocityY;
    float width, height;
    bool isGrounded;
    int jumpsRemaining;
    bool jumpPressed;

    static constexpr float GRAVITY = 980.0f;
    static constexpr float JUMP_FORCE = -500.0f;
    static constexpr float MOVE_SPEED = 300.0f;
    static constexpr float MAX_FALL_SPEED = 600.0f;
    static constexpr int MAX_JUMPS = 2;
};

#endif
