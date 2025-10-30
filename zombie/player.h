#ifndef ZOMBIE_PLAYER_H
#define ZOMBIE_PLAYER_H

#include <SDL.h>
#include <vector>
#include <memory>
#include "weapon.h"

class Maze;
class Bullet;

class Player {
public:
    Player(float x, float y);

    void handleInput(const Uint8* keyState);
    void update(float deltaTime, const Maze& maze);
    void render(SDL_Renderer* renderer) const;

    bool shoot(float targetX, float targetY, std::vector<std::unique_ptr<Bullet>>& bullets, float currentTime);

    float getX() const { return x; }
    float getY() const { return y; }
    float getRadius() const { return radius; }
    float getAngle() const { return angle; }
    void setAngle(float a) { angle = a; }
    float getPitch() const { return pitch; }
    void setPitch(float p) { pitch = p; }

    // Weapon system
    void pickupWeapon(WeaponType weapon);
    void pickupAmmo(WeaponType weapon, int amount);
    void switchWeapon();
    void cycleNextWeapon();
    void cyclePrevWeapon();
    WeaponType getCurrentWeapon() const { return usingMelee ? meleeWeapon : weapons[currentWeaponSlot]; }
    bool hasWeapon(WeaponType weapon) const;
    int getCurrentWeaponSlot() const { return currentWeaponSlot; }
    WeaponType getWeaponInSlot(int slot) const { return weapons[slot]; }
    int getCurrentAmmo() const { return ammo[currentWeaponSlot]; }
    int getAmmoInSlot(int slot) const { return ammo[slot]; }
    bool isOutOfAmmo() const;
    bool isUsingMelee() const { return usingMelee; }
    void setUsingMelee(bool melee) { usingMelee = melee; }
    WeaponType getMeleeWeapon() const { return meleeWeapon; }
    void setMeleeWeapon(WeaponType weapon) { meleeWeapon = weapon; }

    void addKey() { keysCollected++; }
    int getKeys() const { return keysCollected; }

    bool takeDamage() {
        if (health > 0 && damageCooldown <= 0.0f) {
            health--;
            damageCooldown = 1.0f;  // 1 second invulnerability after taking damage
            return true;  // Damage was dealt
        }
        return false;  // No damage dealt (cooldown or already dead)
    }
    void heal(int amount = 1) {
        health = std::min(health + amount, maxHealth);
    }
    int getHealth() const { return health; }
    int getMaxHealth() const { return maxHealth; }
    bool isDead() const { return health <= 0; }
    bool isInvulnerable() const { return damageCooldown > 0.0f; }

private:
    float x, y;
    float angle;  // Viewing angle in radians for FPS (horizontal)
    float pitch;  // Vertical look angle in radians (up/down)
    float moveX, moveY;
    int keysCollected;
    int health;
    float damageCooldown;

    // Animation state
    float walkAnimTime;
    float shootAnimTime;
    bool isShooting;

    // Weapon system
    WeaponType weapons[2];  // Two ranged weapon slots
    int ammo[2];            // Ammo for each weapon slot (-1 for infinite)
    int currentWeaponSlot;
    WeaponType meleeWeapon; // Separate melee weapon slot
    bool usingMelee;        // True if currently using melee weapon
    float lastShotTime;

    static constexpr float SPEED = 80.0f;
    static constexpr float radius = 15.0f;
    static constexpr int maxHealth = 5;  // Player can take 5 hits
};

#endif
