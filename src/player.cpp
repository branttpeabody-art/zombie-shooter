#include "player.h"
#include "platform.h"
#include <algorithm>

Player::Player(float x, float y)
    : x(x), y(y), velocityX(0), velocityY(0),
      width(30), height(40), isGrounded(false),
      jumpsRemaining(MAX_JUMPS), jumpPressed(false) {}

void Player::handleInput(const Uint8* keyState) {
    velocityX = 0;

    if (keyState[SDL_SCANCODE_LEFT] || keyState[SDL_SCANCODE_A]) {
        velocityX = -MOVE_SPEED;
    }
    if (keyState[SDL_SCANCODE_RIGHT] || keyState[SDL_SCANCODE_D]) {
        velocityX = MOVE_SPEED;
    }

    // Double jump logic
    bool jumpKeyDown = keyState[SDL_SCANCODE_SPACE] || keyState[SDL_SCANCODE_UP] || keyState[SDL_SCANCODE_W];

    if (jumpKeyDown && !jumpPressed && jumpsRemaining > 0) {
        velocityY = JUMP_FORCE;
        jumpsRemaining--;
        jumpPressed = true;
        isGrounded = false;
    } else if (!jumpKeyDown) {
        jumpPressed = false;
    }
}

void Player::update(float deltaTime) {
    // Apply gravity
    velocityY += GRAVITY * deltaTime;

    // Limit fall speed
    if (velocityY > MAX_FALL_SPEED) {
        velocityY = MAX_FALL_SPEED;
    }

    // Update position
    x += velocityX * deltaTime;
    y += velocityY * deltaTime;

    // Keep player on screen horizontally
    if (x < 0) x = 0;
    if (x + width > 800) x = 800 - width;

    // Reset grounded state (will be set by collision detection)
    if (velocityY > 0) {
        isGrounded = false;
    }
}

void Player::render(SDL_Renderer* renderer) {
    SDL_Rect rect;
    rect.x = static_cast<int>(x);
    rect.y = static_cast<int>(y);
    rect.w = static_cast<int>(width);
    rect.h = static_cast<int>(height);

    // Draw player as red rectangle
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &rect);
}

void Player::checkCollision(const Platform& platform) {
    float px = platform.getX();
    float py = platform.getY();
    float pw = platform.getWidth();
    float ph = platform.getHeight();

    // Check if player overlaps with platform
    if (x + width > px && x < px + pw &&
        y + height > py && y < py + ph) {

        // Determine collision side and resolve
        float overlapLeft = (x + width) - px;
        float overlapRight = (px + pw) - x;
        float overlapTop = (y + height) - py;
        float overlapBottom = (py + ph) - y;

        // Find minimum overlap
        float minOverlap = std::min({overlapLeft, overlapRight, overlapTop, overlapBottom});

        if (minOverlap == overlapTop && velocityY > 0) {
            // Collision from top - player is landing on platform
            y = py - height;
            velocityY = 0;
            isGrounded = true;
            jumpsRemaining = MAX_JUMPS;  // Reset jumps when landing
        } else if (minOverlap == overlapBottom && velocityY < 0) {
            // Collision from bottom - player hit head
            y = py + ph;
            velocityY = 0;
        } else if (minOverlap == overlapLeft) {
            // Collision from left
            x = px - width;
            velocityX = 0;
        } else if (minOverlap == overlapRight) {
            // Collision from right
            x = px + pw;
            velocityX = 0;
        }
    }
}
