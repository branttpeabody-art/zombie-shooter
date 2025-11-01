#ifndef ZOMBIE_ZOMBIE_H
#define ZOMBIE_ZOMBIE_H

#include <SDL.h>
#include <vector>
#include <utility>
#include <memory>

class Maze;

enum class ZombieType {
    NORMAL,     // Standard zombie: balanced speed and health
    FAST,       // Fast zombie: high speed, low health
    TANK,       // Tank zombie: slow speed, high health
    RUNNER      // Runner zombie: very fast, medium health
};

class Zombie {
public:
    Zombie(float x, float y, int maxHealth = 3, ZombieType type = ZombieType::NORMAL);

    void update(float deltaTime, float playerX, float playerY, const Maze& maze, const std::vector<std::unique_ptr<Zombie>>* allZombies = nullptr);
    void render(SDL_Renderer* renderer) const;

    bool checkCollision(float px, float py, float radius) const;

    float getX() const { return x; }
    float getY() const { return y; }
    float getRadius() const { return radius; }

    void setPosition(float newX, float newY) { x = newX; y = newY; }

    bool isDead() const { return dead; }
    void takeDamage(int damage = 1) {
        health -= damage;
        if (health <= 0 && !dead) {
            dead = true;
            deathAnimTime = 0.0f;
        }
    }
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    ZombieType getType() const { return type; }
    float getFacingAngle() const { return facingAngle; }

private:
    float x, y;
    float facingAngle;  // Angle zombie is facing (in radians)
    bool dead;
    int health;
    int maxHealth;  // Now instance variable instead of static constant
    ZombieType type;
    float speedMultiplier;  // Speed modifier based on type

    // Pathfinding
    std::vector<std::pair<int, int>> path;
    int pathIndex;
    float pathUpdateTimer;

    // Animation state
    float walkAnimTime;
    float deathAnimTime;

    // Wandering state
    bool isChasing;
    float wanderTimer;

    void findPath(int startX, int startY, int goalX, int goalY, const Maze& maze);
    void findRandomWanderTarget(const Maze& maze);
    bool hasLineOfSight(float targetX, float targetY, const Maze& maze) const;

    static constexpr float BASE_SPEED = 85.0f;  // VERY FAST when chasing - SCARY!
    static constexpr float CLOSE_SPEED = 55.0f;   // Fast even when close - relentless!
    static constexpr float SPEED_TRANSITION_DIST = 200.0f;  // Distance for speed transition
    static constexpr float THROUGH_WALL_DETECTION = 150.0f;  // Can see through walls 5 tiles away (5 * 30 pixels)
    static constexpr float MAX_SIGHT_RANGE = 3000.0f;  // Max detection with clear line of sight
    static constexpr float WANDER_RADIUS = 150.0f;  // How far zombies wander when idle
    static constexpr float WANDER_INTERVAL = 3.0f;  // Pick new wander target every 3 seconds
    static constexpr int MAX_HEALTH = 5;  // Base max health (stronger zombies!)
    static constexpr float radius = 12.0f;
    static constexpr float PATH_UPDATE_INTERVAL = 0.5f;  // Update path every 0.5 seconds
    static constexpr float DEATH_ANIM_DURATION = 0.5f;  // Death animation duration
    static constexpr float SEPARATION_DISTANCE = 60.0f;  // How far zombies try to stay apart
    static constexpr float SEPARATION_STRENGTH = 80.0f;  // Strength of separation force
};

#endif
