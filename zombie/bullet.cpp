#include "bullet.h"
#include "maze.h"
#include <cmath>

Bullet::Bullet(float x, float y, float dirX, float dirY, int damage, float speed, bool isExplosive, float explosionRadius)
    : x(x), y(y), dirX(dirX), dirY(dirY), active(true), damage(damage), speed(speed), explosive(isExplosive), explosionRadius(explosionRadius) {
    // Normalize direction
    float len = std::sqrt(dirX * dirX + dirY * dirY);
    if (len > 0) {
        this->dirX /= len;
        this->dirY /= len;
    }
}

void Bullet::update(float deltaTime, const Maze& maze) {
    if (!active) return;

    x += dirX * speed * deltaTime;
    y += dirY * speed * deltaTime;

    // Check collision with walls
    int tileX = static_cast<int>(x / Maze::TILE_SIZE);
    int tileY = static_cast<int>(y / Maze::TILE_SIZE);

    if (maze.isWall(tileX, tileY)) {
        active = false;
    }
}

void Bullet::render(SDL_Renderer* renderer) const {
    if (!active) return;

    float bulletRadius = explosive ? radius * 1.5f : radius;  // Grenades are bigger
    SDL_Rect rect = {
        static_cast<int>(x - bulletRadius),
        static_cast<int>(y - bulletRadius),
        static_cast<int>(bulletRadius * 2),
        static_cast<int>(bulletRadius * 2)
    };

    if (explosive) {
        // Orange/red for grenades
        SDL_SetRenderDrawColor(renderer, 255, 150, 0, 255);
    } else {
        // White for regular bullets
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    }
    SDL_RenderFillRect(renderer, &rect);
}
