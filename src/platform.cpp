#include "platform.h"

Platform::Platform(float x, float y, float width, float height)
    : x(x), y(y), width(width), height(height) {}

void Platform::render(SDL_Renderer* renderer) {
    SDL_Rect rect;
    rect.x = static_cast<int>(x);
    rect.y = static_cast<int>(y);
    rect.w = static_cast<int>(width);
    rect.h = static_cast<int>(height);

    // Draw platform as green rectangle with a darker border
    SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);
    SDL_RenderFillRect(renderer, &rect);

    // Draw border
    SDL_SetRenderDrawColor(renderer, 20, 100, 20, 255);
    SDL_RenderDrawRect(renderer, &rect);
}

bool Platform::checkCollision(float px, float py, float pwidth, float pheight) const {
    return (px + pwidth > x && px < x + width &&
            py + pheight > y && py < y + height);
}
