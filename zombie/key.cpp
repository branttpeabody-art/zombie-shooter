#include "key.h"
#include <cmath>
#include <SDL.h>

Key::Key(float x, float y) : x(x), y(y), collected(false) {}

void Key::render(SDL_Renderer* renderer, bool highlight) const {
    if (collected) return;

    int centerX = static_cast<int>(x);
    int centerY = static_cast<int>(y);

    // Add pulsing glow effect when highlighted (>10 min elapsed)
    if (highlight) {
        Uint32 time = SDL_GetTicks();
        float pulseAmount = (std::sin(time / 200.0f) + 1.0f) / 2.0f;  // Oscillates 0-1
        int glowSize = 10 + static_cast<int>(pulseAmount * 8);

        // Outer glow
        SDL_Rect outerGlow = {centerX - glowSize, centerY - glowSize, glowSize * 2, glowSize * 2};
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, static_cast<int>(80 + pulseAmount * 80));
        SDL_RenderFillRect(renderer, &outerGlow);

        // Middle glow
        SDL_Rect middleGlow = {centerX - glowSize/2, centerY - glowSize/2, glowSize, glowSize};
        SDL_SetRenderDrawColor(renderer, 255, 255, 100, static_cast<int>(120 + pulseAmount * 100));
        SDL_RenderFillRect(renderer, &middleGlow);
    }

    // Draw shadow
    SDL_Rect shadow = {centerX - 8 + 2, centerY - 10 + 2, 16, 20};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
    SDL_RenderFillRect(renderer, &shadow);

    // Key head (circular top part)
    SDL_Rect keyHead = {centerX - 6, centerY - 8, 12, 12};
    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);  // Gold color
    SDL_RenderFillRect(renderer, &keyHead);

    // Key head hole
    SDL_Rect keyHole = {centerX - 3, centerY - 5, 6, 6};
    SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
    SDL_RenderFillRect(renderer, &keyHole);

    // Key shaft (vertical part)
    SDL_Rect keyShaft = {centerX - 2, centerY + 4, 4, 8};
    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
    SDL_RenderFillRect(renderer, &keyShaft);

    // Key teeth (the notches)
    SDL_Rect tooth1 = {centerX + 2, centerY + 6, 3, 2};
    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
    SDL_RenderFillRect(renderer, &tooth1);

    SDL_Rect tooth2 = {centerX + 2, centerY + 10, 3, 2};
    SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
    SDL_RenderFillRect(renderer, &tooth2);

    // Highlight on key head
    SDL_Rect headHighlight = {centerX - 4, centerY - 6, 5, 4};
    SDL_SetRenderDrawColor(renderer, 255, 245, 150, 255);
    SDL_RenderFillRect(renderer, &headHighlight);

    // Border for definition
    SDL_SetRenderDrawColor(renderer, 200, 170, 0, 255);
    SDL_RenderDrawRect(renderer, &keyHead);
    SDL_RenderDrawRect(renderer, &keyShaft);
}

bool Key::checkCollision(float px, float py, float radius) const {
    if (collected) return false;

    float dx = px - x;
    float dy = py - y;
    float dist = std::sqrt(dx*dx + dy*dy);

    return dist < (radius + SIZE/2);
}
