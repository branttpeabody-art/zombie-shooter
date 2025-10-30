#include "healthboost.h"
#include <cmath>

HealthBoost::HealthBoost(float x, float y) : x(x), y(y), collected(false) {}

void HealthBoost::render(SDL_Renderer* renderer) const {
    if (collected) return;

    int centerX = static_cast<int>(x);
    int centerY = static_cast<int>(y);

    // Draw shadow
    SDL_Rect shadow = {centerX - 10 + 2, centerY - 10 + 2, 20, 20};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
    SDL_RenderFillRect(renderer, &shadow);

    // Draw heart/health symbol (cross shape)
    // Horizontal bar
    SDL_Rect horizBar = {centerX - 8, centerY - 2, 16, 4};
    SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);  // Red
    SDL_RenderFillRect(renderer, &horizBar);

    // Vertical bar
    SDL_Rect vertBar = {centerX - 2, centerY - 8, 4, 16};
    SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);  // Red
    SDL_RenderFillRect(renderer, &vertBar);

    // Add white highlights
    SDL_Rect highlightH = {centerX - 6, centerY - 1, 6, 2};
    SDL_SetRenderDrawColor(renderer, 255, 150, 150, 255);
    SDL_RenderFillRect(renderer, &highlightH);

    SDL_Rect highlightV = {centerX - 1, centerY - 6, 2, 6};
    SDL_SetRenderDrawColor(renderer, 255, 150, 150, 255);
    SDL_RenderFillRect(renderer, &highlightV);

    // Border for definition
    SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &horizBar);
    SDL_RenderDrawRect(renderer, &vertBar);

    // Outer glow effect (pulsing) - simulate with multiple rectangles
    SDL_Rect glow = {centerX - 10, centerY - 10, 20, 20};
    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 50);
    SDL_RenderDrawRect(renderer, &glow);
}

bool HealthBoost::checkCollision(float px, float py, float radius) const {
    if (collected) return false;

    float dx = px - x;
    float dy = py - y;
    float dist = std::sqrt(dx*dx + dy*dy);

    return dist < (radius + SIZE/2);
}

void HealthBoost::respawn(float newX, float newY) {
    x = newX;
    y = newY;
    collected = false;
}
