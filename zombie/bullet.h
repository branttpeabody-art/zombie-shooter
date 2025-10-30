#ifndef ZOMBIE_BULLET_H
#define ZOMBIE_BULLET_H

#include <SDL.h>

class Maze;

class Bullet {
public:
    Bullet(float x, float y, float dirX, float dirY, int damage, float speed = 400.0f, bool isExplosive = false, float explosionRadius = 0.0f);

    void update(float deltaTime, const Maze& maze);
    void render(SDL_Renderer* renderer) const;

    bool isActive() const { return active; }
    void deactivate() { active = false; }

    float getX() const { return x; }
    float getY() const { return y; }
    float getRadius() const { return radius; }
    int getDamage() const { return damage; }
    bool isExplosive() const { return explosive; }
    float getExplosionRadius() const { return explosionRadius; }

private:
    float x, y;
    float dirX, dirY;
    bool active;
    int damage;
    float speed;
    bool explosive;           // True for grenades
    float explosionRadius;    // Radius of explosion damage

    static constexpr float radius = 4.0f;
};

#endif
