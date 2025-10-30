#include "player.h"
#include "maze.h"
#include "bullet.h"
#include <cmath>

Player::Player(float x, float y) : x(x), y(y), angle(0.0f), pitch(0.0f), moveX(0), moveY(0), keysCollected(0), health(maxHealth), damageCooldown(0.0f),
    walkAnimTime(0.0f), shootAnimTime(0.0f), isShooting(false), currentWeaponSlot(0), meleeWeapon(WeaponType::KNIFE), usingMelee(false), lastShotTime(0.0f) {
    // Start with shotgun in slot 0, nothing in slot 1
    weapons[0] = WeaponType::SHOTGUN;
    weapons[1] = WeaponType::SHOTGUN;  // Empty slot also shotgun initially
    ammo[0] = -1;  // Shotgun has infinite ammo
    ammo[1] = -1;  // Shotgun has infinite ammo
}

void Player::handleInput(const Uint8* keyState) {
    float forwardBack = 0;
    float leftRight = 0;

    // FPS-style controls: W/S for forward/backward, A/D for strafe left/right
    if (keyState[SDL_SCANCODE_W] || keyState[SDL_SCANCODE_UP]) {
        forwardBack = 1;  // Forward
    }
    if (keyState[SDL_SCANCODE_S] || keyState[SDL_SCANCODE_DOWN]) {
        forwardBack = -1;  // Backward
    }
    if (keyState[SDL_SCANCODE_A] || keyState[SDL_SCANCODE_LEFT]) {
        leftRight = -1;  // Strafe left
    }
    if (keyState[SDL_SCANCODE_D] || keyState[SDL_SCANCODE_RIGHT]) {
        leftRight = 1;  // Strafe right
    }

    // Convert to world coordinates based on player angle
    // Forward/backward movement
    moveX = std::cos(angle) * forwardBack;
    moveY = std::sin(angle) * forwardBack;

    // Strafe left/right (perpendicular to forward direction)
    moveX += std::cos(angle + M_PI / 2.0f) * leftRight;
    moveY += std::sin(angle + M_PI / 2.0f) * leftRight;

    // Normalize diagonal movement
    float len = std::sqrt(moveX * moveX + moveY * moveY);
    if (len > 1.0f) {
        moveX /= len;
        moveY /= len;
    }
}

void Player::update(float deltaTime, const Maze& maze) {
    float newX = x + moveX * SPEED * deltaTime;
    float newY = y + moveY * SPEED * deltaTime;

    // Check collision with walls
    bool canMoveX = true;
    bool canMoveY = true;

    // Check horizontal movement
    int testX = static_cast<int>((newX + (moveX > 0 ? radius : -radius)) / Maze::TILE_SIZE);
    int testY = static_cast<int>(y / Maze::TILE_SIZE);
    if (maze.isWall(testX, testY)) canMoveX = false;

    // Check vertical movement
    testX = static_cast<int>(x / Maze::TILE_SIZE);
    testY = static_cast<int>((newY + (moveY > 0 ? radius : -radius)) / Maze::TILE_SIZE);
    if (maze.isWall(testX, testY)) canMoveY = false;

    if (canMoveX) x = newX;
    if (canMoveY) y = newY;

    // Update walk animation when moving
    if (moveX != 0 || moveY != 0) {
        walkAnimTime += deltaTime * 8.0f;  // Animation speed
    } else {
        walkAnimTime = 0.0f;
    }

    // Update shoot animation
    if (shootAnimTime > 0.0f) {
        shootAnimTime -= deltaTime;
        if (shootAnimTime <= 0.0f) {
            shootAnimTime = 0.0f;
            isShooting = false;
        }
    }

    // Update damage cooldown
    if (damageCooldown > 0.0f) {
        damageCooldown -= deltaTime;
        if (damageCooldown < 0.0f) {
            damageCooldown = 0.0f;
        }
    }
}

void Player::render(SDL_Renderer* renderer) const {
    int centerX = static_cast<int>(x);
    int centerY = static_cast<int>(y);

    // Walking animation - bob up and down
    int walkBob = 0;
    if (walkAnimTime > 0.0f) {
        walkBob = static_cast<int>(std::sin(walkAnimTime) * 2.0f);
    }
    centerY += walkBob;

    // Shooting animation - gun recoil
    int gunRecoil = 0;
    if (isShooting && shootAnimTime > 0.0f) {
        gunRecoil = static_cast<int>(shootAnimTime * 20.0f);  // Recoil distance
    }

    // Draw shadow (elongated oval under character)
    SDL_Rect shadow = {
        centerX - 12,
        centerY + 10,
        24,
        8
    };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 120);
    SDL_RenderFillRect(renderer, &shadow);

    // === LEGS ===
    // Left leg
    SDL_Rect leftLeg = {centerX - 8, centerY + 2, 5, 10};
    SDL_SetRenderDrawColor(renderer, 40, 60, 100, 255);
    SDL_RenderFillRect(renderer, &leftLeg);
    SDL_SetRenderDrawColor(renderer, 30, 50, 90, 255);
    SDL_RenderDrawRect(renderer, &leftLeg);

    // Right leg
    SDL_Rect rightLeg = {centerX + 3, centerY + 2, 5, 10};
    SDL_SetRenderDrawColor(renderer, 40, 60, 100, 255);
    SDL_RenderFillRect(renderer, &rightLeg);
    SDL_SetRenderDrawColor(renderer, 30, 50, 90, 255);
    SDL_RenderDrawRect(renderer, &rightLeg);

    // === BODY ===
    SDL_Rect body = {centerX - 7, centerY - 8, 14, 12};
    SDL_SetRenderDrawColor(renderer, 0, 100, 200, 255);
    SDL_RenderFillRect(renderer, &body);

    // Body highlight
    SDL_Rect bodyHighlight = {centerX - 5, centerY - 7, 10, 6};
    SDL_SetRenderDrawColor(renderer, 50, 150, 255, 255);
    SDL_RenderFillRect(renderer, &bodyHighlight);

    // Body border
    SDL_SetRenderDrawColor(renderer, 0, 70, 160, 255);
    SDL_RenderDrawRect(renderer, &body);

    // === ARMS ===
    // Left arm
    SDL_Rect leftArm = {centerX - 11, centerY - 5, 4, 10};
    SDL_SetRenderDrawColor(renderer, 220, 180, 150, 255);
    SDL_RenderFillRect(renderer, &leftArm);
    SDL_SetRenderDrawColor(renderer, 180, 140, 110, 255);
    SDL_RenderDrawRect(renderer, &leftArm);

    // Right arm
    SDL_Rect rightArm = {centerX + 7, centerY - 5, 4, 10};
    SDL_SetRenderDrawColor(renderer, 220, 180, 150, 255);
    SDL_RenderFillRect(renderer, &rightArm);
    SDL_SetRenderDrawColor(renderer, 180, 140, 110, 255);
    SDL_RenderDrawRect(renderer, &rightArm);

    // === GUN (held in right hand) ===
    // Gun barrel (with recoil animation)
    SDL_Rect gunBarrel = {centerX + 11 - gunRecoil, centerY - 2, 8, 3};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &gunBarrel);

    // Gun handle/grip
    SDL_Rect gunHandle = {centerX + 9 - gunRecoil/2, centerY - 1, 3, 5};
    SDL_SetRenderDrawColor(renderer, 60, 50, 40, 255);
    SDL_RenderFillRect(renderer, &gunHandle);

    // Gun barrel highlight (metallic shine)
    SDL_Rect gunHighlight = {centerX + 12 - gunRecoil, centerY - 1, 4, 1};
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderFillRect(renderer, &gunHighlight);

    // Muzzle flash during shooting
    if (isShooting && shootAnimTime > 0.15f) {
        SDL_Rect muzzleFlash = {centerX + 19 - gunRecoil, centerY - 3, 4, 7};
        SDL_SetRenderDrawColor(renderer, 255, 255, 100, 200);
        SDL_RenderFillRect(renderer, &muzzleFlash);
    }

    // === HEAD ===
    SDL_Rect head = {centerX - 6, centerY - 16, 12, 12};
    // Skin tone
    SDL_SetRenderDrawColor(renderer, 255, 220, 180, 255);
    SDL_RenderFillRect(renderer, &head);

    // Face highlight
    SDL_Rect faceHighlight = {centerX - 4, centerY - 14, 8, 6};
    SDL_SetRenderDrawColor(renderer, 255, 235, 200, 255);
    SDL_RenderFillRect(renderer, &faceHighlight);

    // Eyes
    SDL_Rect leftEye = {centerX - 4, centerY - 12, 2, 2};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &leftEye);

    SDL_Rect rightEye = {centerX + 2, centerY - 12, 2, 2};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &rightEye);

    // Head border
    SDL_SetRenderDrawColor(renderer, 200, 160, 130, 255);
    SDL_RenderDrawRect(renderer, &head);
}

bool Player::shoot(float targetX, float targetY, std::vector<std::unique_ptr<Bullet>>& bullets, float currentTime) {
    WeaponStats stats = getWeaponStats(weapons[currentWeaponSlot]);

    // Check fire rate
    if (currentTime - lastShotTime < stats.fireRate) {
        return false;  // Too soon to shoot again
    }

    // Check ammo (pistol has infinite ammo with -1)
    if (ammo[currentWeaponSlot] == 0) {
        return false;  // Out of ammo
    }

    float dirX = targetX - x;
    float dirY = targetY - y;

    // Check if current weapon is grenade launcher for explosive bullets
    bool isExplosive = (weapons[currentWeaponSlot] == WeaponType::GRENADE_LAUNCHER);
    float explosionRadius = isExplosive ? 150.0f : 0.0f;  // 150 pixel explosion radius

    bullets.push_back(std::make_unique<Bullet>(x, y, dirX, dirY, stats.damage, stats.bulletSpeed, isExplosive, explosionRadius));

    // Consume ammo (only if not infinite)
    if (ammo[currentWeaponSlot] > 0) {
        ammo[currentWeaponSlot]--;
    }

    // Trigger shooting animation
    isShooting = true;
    shootAnimTime = 0.2f;  // Animation duration
    lastShotTime = currentTime;
    return true;  // Shot was fired
}

// Weapon management functions
void Player::pickupWeapon(WeaponType weapon) {
    // Replace current weapon with picked up weapon
    weapons[currentWeaponSlot] = weapon;

    // Initialize ammo for the weapon
    WeaponStats stats = getWeaponStats(weapon);
    ammo[currentWeaponSlot] = stats.maxAmmo;
}

void Player::pickupAmmo(WeaponType weaponType, int amount) {
    // Add ammo to the matching weapon slot
    for (int i = 0; i < 2; i++) {
        if (weapons[i] == weaponType && ammo[i] >= 0) {  // Don't add ammo to infinite weapons
            WeaponStats stats = getWeaponStats(weaponType);
            ammo[i] = std::min(ammo[i] + amount, stats.maxAmmo * 2);  // Max 2x capacity
            return;
        }
    }
}

bool Player::isOutOfAmmo() const {
    return ammo[currentWeaponSlot] == 0;
}

void Player::switchWeapon() {
    currentWeaponSlot = 1 - currentWeaponSlot;  // Toggle between 0 and 1
}

void Player::cycleNextWeapon() {
    currentWeaponSlot = (currentWeaponSlot + 1) % 2;
}

void Player::cyclePrevWeapon() {
    currentWeaponSlot = (currentWeaponSlot + 1) % 2;  // Same as cycleNext for 2 slots
}

bool Player::hasWeapon(WeaponType weapon) const {
    return weapons[0] == weapon || weapons[1] == weapon;
}
