#ifndef ZOMBIE_WEAPON_H
#define ZOMBIE_WEAPON_H

#include <SDL.h>

enum class WeaponType {
    SHOTGUN,
    PISTOL,
    ASSAULT_RIFLE,
    GRENADE_LAUNCHER,
    SMG,                // Submachine gun - high fire rate, moderate damage
    SNIPER,             // Sniper rifle - slow fire, very high damage, long range
    FLAMETHROWER,       // Continuous fire, area effect
    // Melee weapons
    KNIFE,              // Fast melee, low damage
    BAT,                // Medium speed, medium damage
    AXE,                // Slow, high damage
    KATANA              // Fast, high damage
};

struct WeaponStats {
    float fireRate;      // Time between shots in seconds
    float bulletSpeed;
    int damage;
    bool isAutomatic;
    int maxAmmo;         // -1 for infinite ammo
    int ammoPerPickup;   // How much ammo to give when picked up
    bool isMelee;        // True for melee weapons
    float meleeRange;    // Range for melee attacks
    const char* name;
};

class WeaponPickup {
public:
    WeaponPickup(float x, float y, WeaponType type, bool isAmmo = false);

    void render(SDL_Renderer* renderer) const;
    bool checkCollision(float px, float py, float radius) const;

    WeaponType getType() const { return type; }
    bool isCollected() const { return collected; }
    void collect() { collected = true; }
    bool getIsAmmo() const { return isAmmo; }
    void convertToAmmo() { isAmmo = true; }

    float getX() const { return x; }
    float getY() const { return y; }

private:
    float x, y;
    WeaponType type;
    bool collected;
    bool isAmmo;  // True if this is an ammo pickup, false if it's a weapon pickup

    static constexpr float SIZE = 20.0f;
};

// Weapon stats for each type
// Damage balanced for difficulty: Easy (2 HP), Normal (3 HP), Hard (5 HP)
inline WeaponStats getWeaponStats(WeaponType type) {
    switch (type) {
        // Ranged weapons
        case WeaponType::SHOTGUN:
            return {1.2f, 800.0f, 5, false, -1, 0, false, 0.0f, "SHOTGUN"};
        case WeaponType::PISTOL:
            return {0.6f, 400.0f, 2, false, -1, 0, false, 0.0f, "PISTOL"};
        case WeaponType::GRENADE_LAUNCHER:
            return {1.5f, 300.0f, 10, false, 6, 6, false, 0.0f, "GRENADE LAUNCHER"};
        case WeaponType::SNIPER:
            return {2.2f, 1200.0f, 10, false, 8, 8, false, 0.0f, "SNIPER RIFLE"};
        case WeaponType::ASSAULT_RIFLE:
            return {0.08f, 450.0f, 3, true, 30, 30, false, 0.0f, "ASSAULT RIFLE"};
        case WeaponType::SMG:
            return {0.12f, 500.0f, 1, true, 50, 50, false, 0.0f, "SMG"};
        case WeaponType::FLAMETHROWER:
            return {0.04f, 200.0f, 1, true, 100, 50, false, 0.0f, "FLAMETHROWER"};
        // Melee weapons
        case WeaponType::KNIFE:
            return {0.3f, 0.0f, 2, false, -1, 0, true, 50.0f, "KNIFE"};  // Fast, low damage
        case WeaponType::BAT:
            return {0.6f, 0.0f, 3, false, -1, 0, true, 60.0f, "BASEBALL BAT"};  // Medium speed, medium damage
        case WeaponType::AXE:
            return {1.0f, 0.0f, 5, false, -1, 0, true, 55.0f, "AXE"};  // Slow, high damage
        case WeaponType::KATANA:
            return {0.4f, 0.0f, 4, false, -1, 0, true, 65.0f, "KATANA"};  // Fast, high damage
    }
    return {0.8f, 350.0f, 2, false, -1, 0, false, 0.0f, "SHOTGUN"};
}

#endif
