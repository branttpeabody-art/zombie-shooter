#ifndef ZOMBIE_KEY_H
#define ZOMBIE_KEY_H

#include <SDL.h>

class Key {
public:
    Key(float x, float y);

    void render(SDL_Renderer* renderer, bool highlight = false) const;
    bool checkCollision(float px, float py, float radius) const;

    float getX() const { return x; }
    float getY() const { return y; }
    bool isCollected() const { return collected; }
    void collect() { collected = true; }

private:
    float x, y;
    bool collected;
    static constexpr float SIZE = 15.0f;
};

#endif
