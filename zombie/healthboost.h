#ifndef ZOMBIE_HEALTHBOOST_H
#define ZOMBIE_HEALTHBOOST_H

#include <SDL.h>

class HealthBoost {
public:
    HealthBoost(float x, float y);

    void render(SDL_Renderer* renderer) const;
    bool checkCollision(float px, float py, float radius) const;

    float getX() const { return x; }
    float getY() const { return y; }
    bool isCollected() const { return collected; }
    void collect() { collected = true; }
    void respawn(float newX, float newY);

private:
    float x, y;
    bool collected;
    static constexpr float SIZE = 15.0f;
};

#endif
