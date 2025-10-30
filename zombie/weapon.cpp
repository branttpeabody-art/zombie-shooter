#include "weapon.h"
#include <cmath>

WeaponPickup::WeaponPickup(float x, float y, WeaponType type, bool isAmmo)
    : x(x), y(y), type(type), collected(false), isAmmo(isAmmo) {}

void WeaponPickup::render(SDL_Renderer* renderer) const {
    if (collected) return;

    int centerX = static_cast<int>(x);
    int centerY = static_cast<int>(y);

    // Shadow
    SDL_Rect shadow = {centerX - 12 + 2, centerY - 8 + 2, 24, 16};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100);
    SDL_RenderFillRect(renderer, &shadow);

    // Draw weapon based on type
    switch (type) {
        case WeaponType::SHOTGUN: {
            // Pump-action shotgun (M870 style)
            // Stock
            SDL_Rect stock = {centerX - 12, centerY + 2, 8, 6};
            SDL_SetRenderDrawColor(renderer, 70, 50, 30, 255);
            SDL_RenderFillRect(renderer, &stock);

            // Receiver/body
            SDL_Rect body = {centerX - 6, centerY - 1, 12, 8};
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &body);

            // Long barrel
            SDL_Rect barrel = {centerX + 6, centerY, 10, 4};
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            SDL_RenderFillRect(renderer, &barrel);

            // Pump/foregrip
            SDL_Rect pump = {centerX + 2, centerY + 5, 6, 4};
            SDL_SetRenderDrawColor(renderer, 70, 50, 30, 255);
            SDL_RenderFillRect(renderer, &pump);

            // Barrel highlight
            SDL_Rect barrelHighlight = {centerX + 7, centerY + 1, 6, 2};
            SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
            SDL_RenderFillRect(renderer, &barrelHighlight);
            break;
        }
        case WeaponType::PISTOL: {
            // Pistol handle
            SDL_Rect handle = {centerX - 4, centerY, 4, 8};
            SDL_SetRenderDrawColor(renderer, 60, 40, 20, 255);
            SDL_RenderFillRect(renderer, &handle);

            // Pistol barrel
            SDL_Rect barrel = {centerX, centerY - 3, 8, 5};
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
            SDL_RenderFillRect(renderer, &barrel);

            // Highlight
            SDL_Rect barrelHighlight = {centerX + 1, centerY - 2, 4, 2};
            SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);
            SDL_RenderFillRect(renderer, &barrelHighlight);
            break;
        }
        case WeaponType::ASSAULT_RIFLE: {
            // Stock
            SDL_Rect stock = {centerX - 10, centerY + 1, 6, 4};
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &stock);

            // Body
            SDL_Rect body = {centerX - 6, centerY - 2, 10, 6};
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            SDL_RenderFillRect(renderer, &body);

            // Barrel
            SDL_Rect barrel = {centerX + 4, centerY - 1, 8, 3};
            SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
            SDL_RenderFillRect(renderer, &barrel);

            // Magazine
            SDL_Rect mag = {centerX - 2, centerY + 4, 4, 4};
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_RenderFillRect(renderer, &mag);

            // Highlight
            SDL_Rect highlight = {centerX - 4, centerY - 1, 6, 2};
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &highlight);
            break;
        }
        case WeaponType::GRENADE_LAUNCHER: {
            // Body
            SDL_Rect body = {centerX - 8, centerY - 2, 12, 6};
            SDL_SetRenderDrawColor(renderer, 70, 70, 50, 255);
            SDL_RenderFillRect(renderer, &body);

            // Barrel (large tube)
            SDL_Rect barrel = {centerX + 4, centerY - 4, 8, 10};
            SDL_SetRenderDrawColor(renderer, 80, 80, 60, 255);
            SDL_RenderFillRect(renderer, &barrel);

            // Handle
            SDL_Rect handle = {centerX - 4, centerY + 4, 4, 6};
            SDL_SetRenderDrawColor(renderer, 60, 40, 20, 255);
            SDL_RenderFillRect(renderer, &handle);

            // Barrel end (darker)
            SDL_Rect barrelEnd = {centerX + 10, centerY - 3, 2, 8};
            SDL_SetRenderDrawColor(renderer, 40, 40, 30, 255);
            SDL_RenderFillRect(renderer, &barrelEnd);

            // Highlight
            SDL_Rect highlight = {centerX - 6, centerY - 1, 8, 2};
            SDL_SetRenderDrawColor(renderer, 100, 100, 80, 255);
            SDL_RenderFillRect(renderer, &highlight);
            break;
        }
        case WeaponType::SMG: {
            // Compact submachine gun
            // Stock (small)
            SDL_Rect stock = {centerX - 8, centerY + 2, 4, 3};
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_RenderFillRect(renderer, &stock);

            // Body (compact)
            SDL_Rect body = {centerX - 5, centerY - 1, 8, 5};
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &body);

            // Short barrel
            SDL_Rect barrel = {centerX + 3, centerY, 6, 3};
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            SDL_RenderFillRect(renderer, &barrel);

            // Large magazine (extended)
            SDL_Rect mag = {centerX - 2, centerY + 4, 4, 6};
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
            SDL_RenderFillRect(renderer, &mag);

            // Highlight
            SDL_Rect highlight = {centerX - 3, centerY, 5, 1};
            SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
            SDL_RenderFillRect(renderer, &highlight);
            break;
        }
        case WeaponType::SNIPER: {
            // Long sniper rifle
            // Stock
            SDL_Rect stock = {centerX - 12, centerY + 1, 8, 5};
            SDL_SetRenderDrawColor(renderer, 60, 45, 30, 255);
            SDL_RenderFillRect(renderer, &stock);

            // Body/receiver
            SDL_Rect body = {centerX - 6, centerY - 1, 10, 6};
            SDL_SetRenderDrawColor(renderer, 55, 55, 55, 255);
            SDL_RenderFillRect(renderer, &body);

            // Very long barrel
            SDL_Rect barrel = {centerX + 4, centerY, 14, 3};
            SDL_SetRenderDrawColor(renderer, 65, 65, 65, 255);
            SDL_RenderFillRect(renderer, &barrel);

            // Scope
            SDL_Rect scope = {centerX - 2, centerY - 4, 6, 3};
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_RenderFillRect(renderer, &scope);

            // Barrel highlight
            SDL_Rect barrelHighlight = {centerX + 5, centerY + 1, 10, 1};
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &barrelHighlight);
            break;
        }
        case WeaponType::FLAMETHROWER: {
            // Bulky flamethrower
            // Fuel tank
            SDL_Rect tank = {centerX - 10, centerY - 3, 8, 10};
            SDL_SetRenderDrawColor(renderer, 150, 50, 50, 255);  // Red tank
            SDL_RenderFillRect(renderer, &tank);

            // Tank highlight
            SDL_Rect tankHighlight = {centerX - 9, centerY - 2, 3, 4};
            SDL_SetRenderDrawColor(renderer, 180, 70, 70, 255);
            SDL_RenderFillRect(renderer, &tankHighlight);

            // Nozzle body
            SDL_Rect nozzleBody = {centerX - 4, centerY, 8, 4};
            SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
            SDL_RenderFillRect(renderer, &nozzleBody);

            // Nozzle tip
            SDL_Rect nozzleTip = {centerX + 4, centerY + 1, 6, 2};
            SDL_SetRenderDrawColor(renderer, 90, 60, 30, 255);  // Bronze/brass colored
            SDL_RenderFillRect(renderer, &nozzleTip);

            // Grip
            SDL_Rect grip = {centerX - 2, centerY + 4, 3, 5};
            SDL_SetRenderDrawColor(renderer, 50, 40, 30, 255);
            SDL_RenderFillRect(renderer, &grip);
            break;
        }
        case WeaponType::KNIFE: {
            // Simple knife
            SDL_Rect blade = {centerX + 2, centerY - 2, 10, 3};
            SDL_SetRenderDrawColor(renderer, 180, 180, 190, 255);
            SDL_RenderFillRect(renderer, &blade);
            SDL_Rect handle = {centerX - 4, centerY - 1, 6, 5};
            SDL_SetRenderDrawColor(renderer, 60, 40, 20, 255);
            SDL_RenderFillRect(renderer, &handle);
            break;
        }
        case WeaponType::BAT: {
            // Baseball bat
            SDL_Rect barrel = {centerX - 8, centerY - 2, 14, 6};
            SDL_SetRenderDrawColor(renderer, 70, 50, 30, 255);
            SDL_RenderFillRect(renderer, &barrel);
            SDL_Rect handle = {centerX + 6, centerY, 6, 3};
            SDL_SetRenderDrawColor(renderer, 50, 35, 20, 255);
            SDL_RenderFillRect(renderer, &handle);
            break;
        }
        case WeaponType::AXE: {
            // Axe with blade
            SDL_Rect handle = {centerX - 8, centerY, 12, 4};
            SDL_SetRenderDrawColor(renderer, 60, 45, 30, 255);
            SDL_RenderFillRect(renderer, &handle);
            SDL_Rect blade = {centerX + 4, centerY - 4, 6, 10};
            SDL_SetRenderDrawColor(renderer, 150, 150, 160, 255);
            SDL_RenderFillRect(renderer, &blade);
            break;
        }
        case WeaponType::KATANA: {
            // Long curved blade
            SDL_Rect blade = {centerX - 10, centerY - 1, 16, 3};
            SDL_SetRenderDrawColor(renderer, 190, 190, 200, 255);
            SDL_RenderFillRect(renderer, &blade);
            SDL_Rect handle = {centerX + 6, centerY - 2, 5, 5};
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
            SDL_RenderFillRect(renderer, &handle);
            SDL_Rect guard = {centerX + 5, centerY - 3, 2, 7};
            SDL_SetRenderDrawColor(renderer, 100, 100, 0, 255);
            SDL_RenderFillRect(renderer, &guard);
            break;
        }
    }

    // Glow effect - different color for ammo
    SDL_Rect glowBox = {centerX - 14, centerY - 10, 28, 20};
    if (isAmmo) {
        SDL_SetRenderDrawColor(renderer, 255, 200, 50, 80);  // Orange glow for ammo
    } else {
        SDL_SetRenderDrawColor(renderer, 100, 200, 255, 80);  // Blue glow for weapons
    }
    SDL_RenderDrawRect(renderer, &glowBox);

    // Border
    SDL_Rect border = {centerX - 13, centerY - 9, 26, 18};
    if (isAmmo) {
        SDL_SetRenderDrawColor(renderer, 255, 220, 100, 255);  // Orange border for ammo
    } else {
        SDL_SetRenderDrawColor(renderer, 150, 200, 255, 255);  // Blue border for weapons
    }
    SDL_RenderDrawRect(renderer, &border);

    // Draw "A" indicator for ammo pickups
    if (isAmmo) {
        SDL_Rect aLeg1 = {centerX - 18, centerY + 8, 2, 6};
        SDL_Rect aLeg2 = {centerX + 16, centerY + 8, 2, 6};
        SDL_Rect aTop = {centerX - 16, centerY + 8, 14, 2};
        SDL_Rect aCross = {centerX - 16, centerY + 11, 14, 2};
        SDL_SetRenderDrawColor(renderer, 255, 220, 50, 255);
        SDL_RenderFillRect(renderer, &aLeg1);
        SDL_RenderFillRect(renderer, &aLeg2);
        SDL_RenderFillRect(renderer, &aTop);
        SDL_RenderFillRect(renderer, &aCross);
    }
}

bool WeaponPickup::checkCollision(float px, float py, float radius) const {
    if (collected) return false;

    float dx = px - x;
    float dy = py - y;
    float dist = std::sqrt(dx*dx + dy*dy);

    return dist < (radius + SIZE);
}
