#include "zombie.h"
#include "maze.h"
#include <cmath>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <random>

Zombie::Zombie(float x, float y, int maxHealth, ZombieType type) : x(x), y(y), facingAngle(0.0f), dead(false), health(maxHealth), maxHealth(maxHealth),
    type(type), pathIndex(0), pathUpdateTimer(0.0f),
    walkAnimTime(0.0f), deathAnimTime(0.0f), isChasing(false), wanderTimer(0.0f) {

    // Set speed multiplier and adjust health based on zombie type
    switch (type) {
        case ZombieType::FAST:
            speedMultiplier = 1.5f;  // 50% faster
            // Fast zombies are glass cannons - less health
            this->maxHealth = maxHealth - 2;  // 2 less health
            if (this->maxHealth < 1) this->maxHealth = 1;
            this->health = this->maxHealth;
            break;
        case ZombieType::TANK:
            speedMultiplier = 0.6f;  // 40% slower
            // Tank zombies are VERY tanky - much more health!
            this->maxHealth = maxHealth + 5;  // 5 MORE health (total 10!)
            this->health = this->maxHealth;
            break;
        case ZombieType::RUNNER:
            speedMultiplier = 2.0f;  // 100% faster!
            // Runners are fast but fragile
            this->maxHealth = maxHealth - 1;  // 1 less health
            if (this->maxHealth < 1) this->maxHealth = 1;
            this->health = this->maxHealth;
            break;
        case ZombieType::NORMAL:
        default:
            speedMultiplier = 1.0f;  // Normal speed
            // Normal zombies keep default health
            break;
    }
}

void Zombie::findPath(int startX, int startY, int goalX, int goalY, const Maze& maze) {
    path.clear();

    // Bounds check
    if (startX < 0 || startX >= Maze::WIDTH || startY < 0 || startY >= Maze::HEIGHT ||
        goalX < 0 || goalX >= Maze::WIDTH || goalY < 0 || goalY >= Maze::HEIGHT) {
        return;
    }

    // A* pathfinding
    struct Node {
        int x, y;
        float g, h;
        std::pair<int, int> parent;

        float f() const { return g + h; }

        bool operator>(const Node& other) const { return f() > other.f(); }
    };

    auto heuristic = [](int x1, int y1, int x2, int y2) {
        return std::abs(x1 - x2) + std::abs(y1 - y2);
    };

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
    std::unordered_map<int, Node> closedSet;

    Node start = {startX, startY, 0.0f, static_cast<float>(heuristic(startX, startY, goalX, goalY)), {-1, -1}};
    openSet.push(start);

    const int dx[] = {0, 0, 1, -1, 1, -1, 1, -1};  // 8 directions
    const int dy[] = {1, -1, 0, 0, 1, -1, -1, 1};
    const float cost[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.414f, 1.414f, 1.414f, 1.414f};  // Diagonal cost is sqrt(2)

    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();

        int key = current.y * Maze::WIDTH + current.x;

        if (closedSet.find(key) != closedSet.end()) {
            continue;
        }

        closedSet[key] = current;

        // Found goal
        if (current.x == goalX && current.y == goalY) {
            // Reconstruct path
            std::vector<std::pair<int, int>> reconstructed;
            Node node = current;
            while (node.parent.first != -1) {
                reconstructed.push_back({node.x, node.y});
                int parentKey = node.parent.second * Maze::WIDTH + node.parent.first;
                node = closedSet[parentKey];
            }
            std::reverse(reconstructed.begin(), reconstructed.end());
            path = reconstructed;
            pathIndex = 0;
            return;
        }

        // Explore neighbors
        for (int i = 0; i < 8; i++) {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];

            if (nx < 0 || nx >= Maze::WIDTH || ny < 0 || ny >= Maze::HEIGHT) {
                continue;
            }

            if (maze.isWall(nx, ny)) {
                continue;
            }

            // Prevent diagonal movement through corners
            if (i >= 4) {  // Diagonal directions
                // Check if both adjacent cells are walkable
                int diagX = (i == 4 || i == 6) ? 1 : -1;
                int diagY = (i == 4 || i == 5) ? 1 : -1;

                if (maze.isWall(current.x + diagX, current.y) ||
                    maze.isWall(current.x, current.y + diagY)) {
                    continue;  // Don't cut corners
                }
            }

            int nkey = ny * Maze::WIDTH + nx;
            if (closedSet.find(nkey) != closedSet.end()) {
                continue;
            }

            Node neighbor;
            neighbor.x = nx;
            neighbor.y = ny;
            neighbor.g = current.g + cost[i];
            neighbor.h = heuristic(nx, ny, goalX, goalY);
            neighbor.parent = {current.x, current.y};

            openSet.push(neighbor);
        }
    }
}

void Zombie::findRandomWanderTarget(const Maze& maze) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Try to find a random nearby position
    for (int attempt = 0; attempt < 20; attempt++) {
        std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI);
        std::uniform_real_distribution<float> distDist(50.0f, WANDER_RADIUS);

        float angle = angleDist(gen);
        float dist = distDist(gen);

        float targetX = x + std::cos(angle) * dist;
        float targetY = y + std::sin(angle) * dist;

        int targetTileX = static_cast<int>(targetX / Maze::TILE_SIZE);
        int targetTileY = static_cast<int>(targetY / Maze::TILE_SIZE);

        // Check if target is valid
        if (targetTileX > 0 && targetTileX < Maze::WIDTH - 1 &&
            targetTileY > 0 && targetTileY < Maze::HEIGHT - 1 &&
            !maze.isWall(targetTileX, targetTileY)) {

            int zombieTileX = static_cast<int>(x / Maze::TILE_SIZE);
            int zombieTileY = static_cast<int>(y / Maze::TILE_SIZE);

            findPath(zombieTileX, zombieTileY, targetTileX, targetTileY, maze);
            return;
        }
    }
}

bool Zombie::hasLineOfSight(float targetX, float targetY, const Maze& maze) const {
    // Use DDA raycasting to check for walls between zombie and target
    float dx = targetX - x;
    float dy = targetY - y;
    float distance = std::sqrt(dx * dx + dy * dy);

    if (distance < 1.0f) return true;  // Too close to matter

    // Normalize direction
    float dirX = dx / distance;
    float dirY = dy / distance;

    // Step through the ray
    float stepSize = Maze::TILE_SIZE / 4.0f;  // Check every quarter tile
    int steps = static_cast<int>(distance / stepSize);

    for (int i = 1; i < steps; i++) {
        float checkX = x + dirX * stepSize * i;
        float checkY = y + dirY * stepSize * i;

        int tileX = static_cast<int>(checkX / Maze::TILE_SIZE);
        int tileY = static_cast<int>(checkY / Maze::TILE_SIZE);

        // Check bounds
        if (tileX < 0 || tileX >= Maze::WIDTH || tileY < 0 || tileY >= Maze::HEIGHT) {
            return false;
        }

        // If we hit a wall, no line of sight
        if (maze.isWall(tileX, tileY)) {
            return false;
        }
    }

    return true;  // No walls in the way
}

void Zombie::update(float deltaTime, float playerX, float playerY, const Maze& maze, const std::vector<std::unique_ptr<Zombie>>* allZombies) {
    if (dead) {
        // Update death animation
        if (deathAnimTime < DEATH_ANIM_DURATION) {
            deathAnimTime += deltaTime;
        }
        return;
    }

    pathUpdateTimer += deltaTime;
    walkAnimTime += deltaTime * 6.0f;  // Walking animation speed
    wanderTimer += deltaTime;

    // Calculate distance to player for speed adjustment
    float playerDx = playerX - x;
    float playerDy = playerY - y;
    float distToPlayer = std::sqrt(playerDx * playerDx + playerDy * playerDy);

    // Determine if zombie should chase or wander using line-of-sight
    // Can see through walls up to 5 tiles (150 pixels)
    // Can see infinitely with clear line of sight (up to MAX_SIGHT_RANGE)
    bool canSeePlayer = false;
    if (distToPlayer <= THROUGH_WALL_DETECTION) {
        // Close enough to see through walls
        canSeePlayer = true;
    } else if (distToPlayer <= MAX_SIGHT_RANGE) {
        // Check for clear line of sight
        canSeePlayer = hasLineOfSight(playerX, playerY, maze);
    }

    if (canSeePlayer) {
        isChasing = true;
        wanderTimer = 0.0f;  // Reset wander timer when chasing
    } else {
        isChasing = false;
    }

    // Calculate dynamic speed based on distance to player (only when chasing)
    float currentSpeed;
    if (isChasing) {
        if (distToPlayer > SPEED_TRANSITION_DIST) {
            currentSpeed = BASE_SPEED * speedMultiplier;  // Fast when far away, modified by type
        } else {
            // Lerp between CLOSE_SPEED and BASE_SPEED
            float t = distToPlayer / SPEED_TRANSITION_DIST;
            currentSpeed = (CLOSE_SPEED + (BASE_SPEED - CLOSE_SPEED) * t) * speedMultiplier;
        }
    } else {
        // Slower when wandering
        currentSpeed = CLOSE_SPEED * 0.7f * speedMultiplier;
    }

    // Update path periodically
    if (pathUpdateTimer >= PATH_UPDATE_INTERVAL || path.empty()) {
        pathUpdateTimer = 0.0f;
        int zombieTileX = static_cast<int>(x / Maze::TILE_SIZE);
        int zombieTileY = static_cast<int>(y / Maze::TILE_SIZE);

        if (isChasing) {
            // Chase player
            int playerTileX = static_cast<int>(playerX / Maze::TILE_SIZE);
            int playerTileY = static_cast<int>(playerY / Maze::TILE_SIZE);
            findPath(zombieTileX, zombieTileY, playerTileX, playerTileY, maze);
        } else {
            // Wander randomly
            if (wanderTimer >= WANDER_INTERVAL) {
                wanderTimer = 0.0f;
                findRandomWanderTarget(maze);
            }
        }
    }

    // Follow path
    if (!path.empty() && pathIndex < static_cast<int>(path.size())) {
        auto [targetTileX, targetTileY] = path[pathIndex];
        float targetX = targetTileX * Maze::TILE_SIZE + Maze::TILE_SIZE / 2.0f;
        float targetY = targetTileY * Maze::TILE_SIZE + Maze::TILE_SIZE / 2.0f;

        float dx = targetX - x;
        float dy = targetY - y;
        float dist = std::sqrt(dx * dx + dy * dy);

        // If we're close to the waypoint, move to next one
        if (dist < Maze::TILE_SIZE / 4.0f) {
            pathIndex++;
            return;
        }

        // Move toward current waypoint with dynamic speed
        if (dist > 0) {
            dx /= dist;
            dy /= dist;

            // Calculate separation force from nearby zombies
            float separationX = 0.0f;
            float separationY = 0.0f;

            if (allZombies != nullptr) {
                for (const auto& other : *allZombies) {
                    if (other.get() != this && !other->isDead()) {
                        float otherDx = x - other->getX();
                        float otherDy = y - other->getY();
                        float otherDist = std::sqrt(otherDx * otherDx + otherDy * otherDy);

                        // Apply separation force if zombie is too close
                        if (otherDist > 0 && otherDist < SEPARATION_DISTANCE) {
                            float separationFactor = (SEPARATION_DISTANCE - otherDist) / SEPARATION_DISTANCE;
                            separationX += (otherDx / otherDist) * separationFactor;
                            separationY += (otherDy / otherDist) * separationFactor;
                        }
                    }
                }
            }

            // Combine pathfinding direction with separation
            float finalDx = dx + separationX * 0.5f;  // Give separation lower priority
            float finalDy = dy + separationY * 0.5f;

            // Normalize the combined direction
            float finalDist = std::sqrt(finalDx * finalDx + finalDy * finalDy);
            if (finalDist > 0) {
                finalDx /= finalDist;
                finalDy /= finalDist;

                // Update facing angle based on movement direction
                facingAngle = std::atan2(finalDy, finalDx);
            }

            float newX = x + finalDx * currentSpeed * deltaTime;
            float newY = y + finalDy * currentSpeed * deltaTime;

            // Simple collision check
            int tileX = static_cast<int>(newX / Maze::TILE_SIZE);
            int tileY = static_cast<int>(newY / Maze::TILE_SIZE);

            if (!maze.isWall(tileX, tileY)) {
                x = newX;
                y = newY;
            }
        }
    }
}

void Zombie::render(SDL_Renderer* renderer) const {
    int centerX = static_cast<int>(x);
    int centerY = static_cast<int>(y);

    // Death animation - fade and fall
    int alpha = 255;
    int deathOffset = 0;
    if (dead) {
        float deathProgress = deathAnimTime / DEATH_ANIM_DURATION;
        if (deathProgress > 1.0f) deathProgress = 1.0f;

        alpha = static_cast<int>(255 * (1.0f - deathProgress));
        deathOffset = static_cast<int>(deathProgress * 15);  // Fall down

        if (alpha < 10) return;  // Don't render if almost invisible
    }

    // Walking animation - sway and bob
    int walkBob = 0;
    int walkSway = 0;
    if (!dead) {
        walkBob = static_cast<int>(std::sin(walkAnimTime) * 1.5f);
        walkSway = static_cast<int>(std::cos(walkAnimTime * 0.5f) * 2.0f);
    }
    centerY += walkBob + deathOffset;
    centerX += walkSway;

    // Draw shadow (larger and darker for zombie - more menacing)
    SDL_Rect shadow = {
        centerX - 16,
        centerY + 10,
        32,
        12
    };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);  // Darker shadow
    SDL_RenderFillRect(renderer, &shadow);

    // === LEGS (hunched/bent) - 3D with shading ===
    // Left leg (slightly bent)
    SDL_Rect leftLeg = {centerX - 9, centerY + 1, 6, 11};
    SDL_SetRenderDrawColor(renderer, 60, 80, 50, 255);
    SDL_RenderFillRect(renderer, &leftLeg);

    // Left leg highlight (3D effect - light from top-left)
    SDL_Rect leftLegHighlight = {centerX - 9, centerY + 1, 3, 5};
    SDL_SetRenderDrawColor(renderer, 75, 95, 65, 255);
    SDL_RenderFillRect(renderer, &leftLegHighlight);

    // Left leg shadow (3D effect - bottom-right)
    SDL_Rect leftLegShadow = {centerX - 6, centerY + 7, 3, 5};
    SDL_SetRenderDrawColor(renderer, 45, 60, 35, 255);
    SDL_RenderFillRect(renderer, &leftLegShadow);

    SDL_SetRenderDrawColor(renderer, 40, 60, 30, 255);
    SDL_RenderDrawRect(renderer, &leftLeg);

    // Right leg (dragging)
    SDL_Rect rightLeg = {centerX + 3, centerY + 3, 6, 9};
    SDL_SetRenderDrawColor(renderer, 60, 80, 50, 255);
    SDL_RenderFillRect(renderer, &rightLeg);

    // Right leg highlight (3D effect)
    SDL_Rect rightLegHighlight = {centerX + 3, centerY + 3, 3, 4};
    SDL_SetRenderDrawColor(renderer, 75, 95, 65, 255);
    SDL_RenderFillRect(renderer, &rightLegHighlight);

    // Right leg shadow (3D effect)
    SDL_Rect rightLegShadow = {centerX + 6, centerY + 8, 3, 4};
    SDL_SetRenderDrawColor(renderer, 45, 60, 35, 255);
    SDL_RenderFillRect(renderer, &rightLegShadow);

    SDL_SetRenderDrawColor(renderer, 40, 60, 30, 255);
    SDL_RenderDrawRect(renderer, &rightLeg);

    // === BODY (hunched forward) ===
    SDL_Rect body = {centerX - 8, centerY - 7, 16, 11};

    // Color based on zombie type
    int bodyR, bodyG, bodyB;
    int decayR, decayG, decayB;
    int borderR, borderG, borderB;

    switch (type) {
        case ZombieType::FAST:
            // Darker yellow/orange tint - faster, agile, more menacing
            bodyR = 100; bodyG = 100; bodyB = 30;
            decayR = 70; decayG = 60; decayB = 10;
            borderR = 80; borderG = 70; borderB = 20;
            break;
        case ZombieType::TANK:
            // Much darker red/brown - tough, armored, terrifying
            bodyR = 80; bodyG = 40; bodyB = 40;
            decayR = 50; decayG = 20; decayB = 20;
            borderR = 60; borderG = 30; borderB = 30;
            break;
        case ZombieType::RUNNER:
            // Darker blood red - very fast, aggressive
            bodyR = 120; bodyG = 40; bodyB = 40;
            decayR = 80; decayG = 20; decayB = 20;
            borderR = 100; borderG = 30; borderB = 30;
            break;
        case ZombieType::NORMAL:
        default:
            // Darker green/brown - more dead-looking
            bodyR = 60; bodyG = 90; bodyB = 40;
            decayR = 35; decayG = 60; decayB = 25;
            borderR = 45; borderG = 70; borderB = 35;
            break;
    }

    SDL_SetRenderDrawColor(renderer, bodyR, bodyG, bodyB, 255);
    SDL_RenderFillRect(renderer, &body);

    // 3D body highlight (light from top-left) - more dramatic
    SDL_Rect bodyHighlight = {centerX - 8, centerY - 7, 8, 5};
    int highlightR = std::min(255, bodyR + 50);
    int highlightG = std::min(255, bodyG + 50);
    int highlightB = std::min(255, bodyB + 35);
    SDL_SetRenderDrawColor(renderer, highlightR, highlightG, highlightB, 255);
    SDL_RenderFillRect(renderer, &bodyHighlight);

    // 3D body shadow (bottom-right) - much darker and more prominent
    SDL_Rect bodyShadow = {centerX + 2, centerY - 1, 6, 5};
    int shadowR = std::max(0, bodyR - 45);
    int shadowG = std::max(0, bodyG - 45);
    int shadowB = std::max(0, bodyB - 35);
    SDL_SetRenderDrawColor(renderer, shadowR, shadowG, shadowB, 255);
    SDL_RenderFillRect(renderer, &bodyShadow);

    // Decay/damage marks on body
    SDL_Rect decay1 = {centerX - 6, centerY - 5, 4, 3};
    SDL_SetRenderDrawColor(renderer, decayR, decayG, decayB, 255);
    SDL_RenderFillRect(renderer, &decay1);

    SDL_Rect decay2 = {centerX + 2, centerY - 3, 3, 4};
    SDL_SetRenderDrawColor(renderer, decayR, decayG, decayB, 255);
    SDL_RenderFillRect(renderer, &decay2);

    // Body border (darker for depth)
    SDL_SetRenderDrawColor(renderer, borderR, borderG, borderB, 255);
    SDL_RenderDrawRect(renderer, &body);

    // === ARMS (outstretched forward, zombie-style) - 3D ===
    // Left arm reaching forward
    SDL_Rect leftArm = {centerX - 14, centerY - 6, 6, 11};
    SDL_SetRenderDrawColor(renderer, 100, 140, 80, 255);
    SDL_RenderFillRect(renderer, &leftArm);

    // Left arm highlight (3D cylinder effect)
    SDL_Rect leftArmHighlight = {centerX - 14, centerY - 6, 3, 9};
    SDL_SetRenderDrawColor(renderer, 120, 160, 100, 255);
    SDL_RenderFillRect(renderer, &leftArmHighlight);

    // Left arm shadow
    SDL_Rect leftArmShadow = {centerX - 10, centerY, 2, 5};
    SDL_SetRenderDrawColor(renderer, 70, 100, 60, 255);
    SDL_RenderFillRect(renderer, &leftArmShadow);

    SDL_SetRenderDrawColor(renderer, 70, 100, 60, 255);
    SDL_RenderDrawRect(renderer, &leftArm);

    // Left hand/claw - 3D
    SDL_Rect leftHand = {centerX - 15, centerY + 3, 4, 5};
    SDL_SetRenderDrawColor(renderer, 90, 120, 70, 255);
    SDL_RenderFillRect(renderer, &leftHand);

    // Left hand highlight
    SDL_Rect leftHandHighlight = {centerX - 15, centerY + 3, 2, 3};
    SDL_SetRenderDrawColor(renderer, 110, 140, 90, 255);
    SDL_RenderFillRect(renderer, &leftHandHighlight);

    // Right arm reaching forward
    SDL_Rect rightArm = {centerX + 8, centerY - 6, 6, 11};
    SDL_SetRenderDrawColor(renderer, 100, 140, 80, 255);
    SDL_RenderFillRect(renderer, &rightArm);

    // Right arm highlight (3D cylinder effect)
    SDL_Rect rightArmHighlight = {centerX + 8, centerY - 6, 3, 9};
    SDL_SetRenderDrawColor(renderer, 120, 160, 100, 255);
    SDL_RenderFillRect(renderer, &rightArmHighlight);

    // Right arm shadow
    SDL_Rect rightArmShadow = {centerX + 12, centerY, 2, 5};
    SDL_SetRenderDrawColor(renderer, 70, 100, 60, 255);
    SDL_RenderFillRect(renderer, &rightArmShadow);

    SDL_SetRenderDrawColor(renderer, 70, 100, 60, 255);
    SDL_RenderDrawRect(renderer, &rightArm);

    // Right hand/claw - 3D
    SDL_Rect rightHand = {centerX + 11, centerY + 3, 4, 5};
    SDL_SetRenderDrawColor(renderer, 90, 120, 70, 255);
    SDL_RenderFillRect(renderer, &rightHand);

    // Right hand highlight
    SDL_Rect rightHandHighlight = {centerX + 11, centerY + 3, 2, 3};
    SDL_SetRenderDrawColor(renderer, 110, 140, 90, 255);
    SDL_RenderFillRect(renderer, &rightHandHighlight);

    // === HEAD (decayed, menacing) ===
    SDL_Rect head = {centerX - 7, centerY - 16, 14, 13};

    // Head color based on zombie type - DARKER and more decayed
    int headR, headG, headB;
    switch (type) {
        case ZombieType::FAST:
            headR = 110; headG = 110; headB = 40;
            break;
        case ZombieType::TANK:
            headR = 90; headG = 50; headB = 50;
            break;
        case ZombieType::RUNNER:
            headR = 130; headG = 50; headB = 50;
            break;
        case ZombieType::NORMAL:
        default:
            headR = 75; headG = 110; headB = 65;
            break;
    }

    SDL_SetRenderDrawColor(renderer, headR, headG, headB, 255);
    SDL_RenderFillRect(renderer, &head);

    // 3D head highlight (sphere-like lighting from top-left)
    SDL_Rect headHighlight = {centerX - 7, centerY - 16, 7, 6};
    int headHighlightR = std::min(255, headR + 40);
    int headHighlightG = std::min(255, headG + 40);
    int headHighlightB = std::min(255, headB + 30);
    SDL_SetRenderDrawColor(renderer, headHighlightR, headHighlightG, headHighlightB, 255);
    SDL_RenderFillRect(renderer, &headHighlight);

    // 3D head shadow (sphere-like shading on bottom-right)
    SDL_Rect headShadow = {centerX + 1, centerY - 8, 6, 5};
    int headShadowR = std::max(0, headR - 35);
    int headShadowG = std::max(0, headG - 35);
    int headShadowB = std::max(0, headB - 30);
    SDL_SetRenderDrawColor(renderer, headShadowR, headShadowG, headShadowB, 255);
    SDL_RenderFillRect(renderer, &headShadow);

    // Decay marks on face (use body's decay colors)
    SDL_Rect faceDecay = {centerX - 5, centerY - 14, 3, 3};
    SDL_SetRenderDrawColor(renderer, decayR, decayG, decayB, 255);
    SDL_RenderFillRect(renderer, &faceDecay);

    // === EYES (TERRIFYING - intense glowing red with depth) ===
    // Left eye socket (PITCH BLACK and DEEP for maximum scary effect)
    SDL_Rect leftEyeSocket = {centerX - 7, centerY - 15, 5, 5};
    SDL_SetRenderDrawColor(renderer, 5, 5, 5, 255);  // Nearly black
    SDL_RenderFillRect(renderer, &leftEyeSocket);

    // Left eye outer GLOW (larger, more intense - scary red aura)
    SDL_Rect leftEyeOuterGlow = {centerX - 7, centerY - 15, 5, 5};
    SDL_SetRenderDrawColor(renderer, 200, 10, 10, 180);  // Intense red glow
    SDL_RenderFillRect(renderer, &leftEyeOuterGlow);

    // Left eye inner glow
    SDL_Rect leftEyeGlow = {centerX - 6, centerY - 14, 4, 4};
    SDL_SetRenderDrawColor(renderer, 220, 20, 20, 200);  // Brighter red glow
    SDL_RenderFillRect(renderer, &leftEyeGlow);

    // Left eye (BLAZING RED - menacing and glowing)
    SDL_Rect leftEye = {centerX - 5, centerY - 13, 3, 3};
    SDL_SetRenderDrawColor(renderer, 255, 20, 20, 255);  // Pure bright red
    SDL_RenderFillRect(renderer, &leftEye);

    // Left eye core (BLAZING INTENSE glow center)
    SDL_Rect leftEyeCore = {centerX - 4, centerY - 12, 2, 2};
    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);  // Ultra bright center
    SDL_RenderFillRect(renderer, &leftEyeCore);

    // Left eye highlight (3D sphere - bright spot for depth)
    SDL_Rect leftEyeHighlight = {centerX - 5, centerY - 13, 1, 1};
    SDL_SetRenderDrawColor(renderer, 255, 255, 200, 255);  // Nearly white highlight
    SDL_RenderFillRect(renderer, &leftEyeHighlight);

    // Right eye socket (PITCH BLACK and DEEP)
    SDL_Rect rightEyeSocket = {centerX + 2, centerY - 15, 5, 5};
    SDL_SetRenderDrawColor(renderer, 5, 5, 5, 255);  // Nearly black
    SDL_RenderFillRect(renderer, &rightEyeSocket);

    // Right eye outer GLOW (larger, more intense)
    SDL_Rect rightEyeOuterGlow = {centerX + 2, centerY - 15, 5, 5};
    SDL_SetRenderDrawColor(renderer, 200, 10, 10, 180);  // Intense red glow
    SDL_RenderFillRect(renderer, &rightEyeOuterGlow);

    // Right eye inner glow
    SDL_Rect rightEyeGlow = {centerX + 2, centerY - 14, 4, 4};
    SDL_SetRenderDrawColor(renderer, 220, 20, 20, 200);  // Brighter red glow
    SDL_RenderFillRect(renderer, &rightEyeGlow);

    // Right eye (BLAZING RED - menacing and glowing)
    SDL_Rect rightEye = {centerX + 2, centerY - 13, 3, 3};
    SDL_SetRenderDrawColor(renderer, 255, 20, 20, 255);  // Pure bright red
    SDL_RenderFillRect(renderer, &rightEye);

    // Right eye core (BLAZING INTENSE glow center)
    SDL_Rect rightEyeCore = {centerX + 3, centerY - 12, 2, 2};
    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);  // Ultra bright center
    SDL_RenderFillRect(renderer, &rightEyeCore);

    // Right eye highlight (3D sphere - bright spot for depth)
    SDL_Rect rightEyeHighlight = {centerX + 2, centerY - 13, 1, 1};
    SDL_SetRenderDrawColor(renderer, 255, 255, 200, 255);  // Nearly white highlight
    SDL_RenderFillRect(renderer, &rightEyeHighlight);

    // === MOUTH (SCARY - 3D with more teeth) ===
    // Mouth cavity (deeper shadow for 3D depth) - BIGGER and SCARIER
    SDL_Rect mouthCavity = {centerX - 4, centerY - 9, 8, 5};
    SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);  // Very dark
    SDL_RenderFillRect(renderer, &mouthCavity);

    // Mouth opening (gaping and menacing)
    SDL_Rect mouth = {centerX - 4, centerY - 9, 8, 4};
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderFillRect(renderer, &mouth);

    // MORE TEETH - jagged and scary (5 teeth across)
    SDL_Rect tooth1 = {centerX - 3, centerY - 9, 1, 2};
    SDL_SetRenderDrawColor(renderer, 200, 200, 180, 255);
    SDL_RenderFillRect(renderer, &tooth1);

    SDL_Rect tooth2 = {centerX - 1, centerY - 9, 1, 2};
    SDL_SetRenderDrawColor(renderer, 200, 200, 180, 255);
    SDL_RenderFillRect(renderer, &tooth2);

    SDL_Rect tooth3 = {centerX + 1, centerY - 9, 1, 2};
    SDL_SetRenderDrawColor(renderer, 200, 200, 180, 255);
    SDL_RenderFillRect(renderer, &tooth3);

    SDL_Rect tooth4 = {centerX + 3, centerY - 9, 1, 2};
    SDL_SetRenderDrawColor(renderer, 200, 200, 180, 255);
    SDL_RenderFillRect(renderer, &tooth4);

    // Bottom teeth (menacing)
    SDL_Rect bottomTooth1 = {centerX - 2, centerY - 6, 1, 1};
    SDL_SetRenderDrawColor(renderer, 180, 180, 160, 255);
    SDL_RenderFillRect(renderer, &bottomTooth1);

    SDL_Rect bottomTooth2 = {centerX + 1, centerY - 6, 1, 1};
    SDL_SetRenderDrawColor(renderer, 180, 180, 160, 255);
    SDL_RenderFillRect(renderer, &bottomTooth2);

    // === SCARS AND WOUNDS (scary details) ===
    // Scar across face
    SDL_Rect scar1 = {centerX - 6, centerY - 11, 5, 1};
    SDL_SetRenderDrawColor(renderer, std::max(0, headR - 50), std::max(0, headG - 50), std::max(0, headB - 40), 255);
    SDL_RenderFillRect(renderer, &scar1);

    // Vertical wound
    SDL_Rect wound1 = {centerX + 4, centerY - 14, 1, 4};
    SDL_SetRenderDrawColor(renderer, 120, 40, 40, 255);  // Dark red wound
    SDL_RenderFillRect(renderer, &wound1);

    // Gash on forehead
    SDL_Rect gash = {centerX - 2, centerY - 16, 3, 1};
    SDL_SetRenderDrawColor(renderer, 100, 30, 30, 255);  // Bloody gash
    SDL_RenderFillRect(renderer, &gash);

    // Jaw definition line (more skeletal)
    SDL_Rect jawLine = {centerX - 6, centerY - 5, 12, 1};
    SDL_SetRenderDrawColor(renderer, std::max(0, headR - 40), std::max(0, headG - 40), std::max(0, headB - 35), 255);
    SDL_RenderFillRect(renderer, &jawLine);

    // MORE GRUESOME DETAILS - dripping blood from mouth/wounds
    SDL_Rect bloodDrip1 = {centerX - 3, centerY - 4, 1, 3};
    SDL_SetRenderDrawColor(renderer, 140, 10, 10, 255);  // Dark blood
    SDL_RenderFillRect(renderer, &bloodDrip1);

    SDL_Rect bloodDrip2 = {centerX + 2, centerY - 4, 1, 4};
    SDL_SetRenderDrawColor(renderer, 130, 15, 10, 255);
    SDL_RenderFillRect(renderer, &bloodDrip2);

    // Exposed skull/bone on head (terrifying!)
    SDL_Rect exposedBone1 = {centerX - 6, centerY - 13, 2, 2};
    SDL_SetRenderDrawColor(renderer, 220, 220, 210, 255);  // Bone white
    SDL_RenderFillRect(renderer, &exposedBone1);

    SDL_Rect exposedBone2 = {centerX + 4, centerY - 12, 2, 3};
    SDL_SetRenderDrawColor(renderer, 215, 215, 205, 255);
    SDL_RenderFillRect(renderer, &exposedBone2);

    // Missing flesh patches (showing darker tissue underneath)
    SDL_Rect missingFlesh1 = {centerX - 4, centerY - 10, 2, 2};
    SDL_SetRenderDrawColor(renderer, std::max(0, headR - 60), std::max(0, headG - 60), std::max(0, headB - 50), 255);
    SDL_RenderFillRect(renderer, &missingFlesh1);

    SDL_Rect missingFlesh2 = {centerX + 1, centerY - 14, 2, 2};
    SDL_SetRenderDrawColor(renderer, std::max(0, headR - 65), std::max(0, headG - 65), std::max(0, headB - 55), 255);
    SDL_RenderFillRect(renderer, &missingFlesh2);

    // Head border (darker for depth)
    SDL_SetRenderDrawColor(renderer, headShadowR, headShadowG, headShadowB, 255);
    SDL_RenderDrawRect(renderer, &head);

    // Health bar (only show if damaged)
    if (!dead && health < maxHealth) {
        int barWidth = 28;  // Wider bar for more health
        int barHeight = 4;   // Taller bar
        int barX = centerX - barWidth / 2;
        int barY = centerY - 23;  // Above head

        // Background (dark red)
        SDL_Rect healthBg = {barX, barY, barWidth, barHeight};
        SDL_SetRenderDrawColor(renderer, 80, 20, 20, 255);
        SDL_RenderFillRect(renderer, &healthBg);

        // Health bar (bright green to red based on health percentage)
        int healthWidth = (barWidth * health) / maxHealth;
        SDL_Rect healthBar = {barX, barY, healthWidth, barHeight};

        // Color gradient based on health percentage
        float healthPercent = static_cast<float>(health) / maxHealth;
        if (healthPercent > 0.66f) {
            SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255); // Green (healthy)
        } else if (healthPercent > 0.33f) {
            SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);   // Yellow/orange (damaged)
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);   // Red (critical)
        }
        SDL_RenderFillRect(renderer, &healthBar);

        // Border with glow effect for stronger zombies
        SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
        SDL_RenderDrawRect(renderer, &healthBg);
    }
}

bool Zombie::checkCollision(float px, float py, float pRadius) const {
    if (dead) return false;

    float dx = px - x;
    float dy = py - y;
    float dist = std::sqrt(dx*dx + dy*dy);

    return dist < (radius + pRadius);
}
