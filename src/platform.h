#ifndef PLATFORM_H
#define PLATFORM_H

#include <SDL.h>

class Platform {
public:
    Platform(float x, float y, float width, float height);

    void render(SDL_Renderer* renderer);

    float getX() const { return x; }
    float getY() const { return y; }
    float getWidth() const { return width; }
    float getHeight() const { return height; }

    bool checkCollision(float px, float py, float pwidth, float pheight) const;

private:
    float x, y;
    float width, height;
};

#endif
