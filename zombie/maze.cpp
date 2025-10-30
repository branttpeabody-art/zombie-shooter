#include "maze.h"
#include <random>
#include <algorithm>

Maze::Maze(MazeType type) : tiles(HEIGHT, std::vector<TileType>(WIDTH, TileType::Wall)), mazeType(type) {
    if (type == MazeType::CIRCULAR) {
        generateCircularMaze();
    } else if (type == MazeType::INFINITE) {
        generateInfiniteMaze();
    } else if (type == MazeType::SOLDIER) {
        generateSoldierMaze();
    } else {
        generateRandomMaze();
    }
}

void Maze::generateRandomMaze() {
    // Fill with walls
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            tiles[y][x] = TileType::Wall;
        }
    }

    // Start carving from position (1, 1)
    carvePassagesFrom(1, 1);

    // Randomly add 1-2 SMALL rooms (NOT in center!)
    int numRooms = 1 + (rand() % 2);  // 1 or 2 rooms
    int centerX = WIDTH / 2;
    int centerY = HEIGHT / 2;

    for (int room = 0; room < numRooms; room++) {
        // SMALLER room size between 5x5 and 8x8
        int roomWidth = 5 + (rand() % 4);
        int roomHeight = 5 + (rand() % 4);

        // Try to find position FAR from center (avoid middle 50% of map)
        int roomX, roomY;
        bool validPosition = false;
        for (int attempt = 0; attempt < 50 && !validPosition; attempt++) {
            roomX = 5 + (rand() % (WIDTH - roomWidth - 10));
            roomY = 5 + (rand() % (HEIGHT - roomHeight - 10));

            // Check if room center is far enough from map center
            int roomCenterX = roomX + roomWidth / 2;
            int roomCenterY = roomY + roomHeight / 2;
            int distX = std::abs(roomCenterX - centerX);
            int distY = std::abs(roomCenterY - centerY);

            // Room must be in outer areas (not within 8 tiles of center)
            if (distX > 8 || distY > 8) {
                validPosition = true;
            }
        }

        if (!validPosition) continue;  // Skip this room if can't find good position

        // Carve out the SMALL room
        for (int y = roomY; y < roomY + roomHeight && y < HEIGHT - 1; y++) {
            for (int x = roomX; x < roomX + roomWidth && x < WIDTH - 1; x++) {
                tiles[y][x] = TileType::Empty;
            }
        }
    }

    // NO CENTER ROOM - completely removed per user request

    // Ensure borders are walls
    for (int x = 0; x < WIDTH; x++) {
        tiles[0][x] = TileType::Wall;
        tiles[HEIGHT-1][x] = TileType::Wall;
    }
    for (int y = 0; y < HEIGHT; y++) {
        tiles[y][0] = TileType::Wall;
        tiles[y][WIDTH-1] = TileType::Wall;
    }

    // Set exit door (make sure it's not a wall)
    tiles[HEIGHT-2][WIDTH-2] = TileType::Exit;

    // Ensure area around exit is clear
    tiles[HEIGHT-3][WIDTH-2] = TileType::Empty;
    tiles[HEIGHT-2][WIDTH-3] = TileType::Empty;

    // Ensure start position is clear
    tiles[1][1] = TileType::Empty;
    tiles[1][2] = TileType::Empty;
    tiles[2][1] = TileType::Empty;
}

void Maze::generateCircularMaze() {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Fill with walls initially
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            tiles[y][x] = TileType::Wall;
        }
    }

    // Center point
    int centerX = WIDTH / 2;
    int centerY = HEIGHT / 2;

    // Maximum radius - USE FULL SPACE (much larger)
    int maxRadius = std::min(WIDTH, HEIGHT) / 2 - 1;  // Changed from -2 to -1 for more space

    // Create circular area with concentric ring maze
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int dx = x - centerX;
            int dy = y - centerY;
            float distance = std::sqrt(dx * dx + dy * dy);

            // Create concentric rings with NARROW spacing for maze-like feel
            if (distance < maxRadius) {
                float normalizedDist = distance / maxRadius;  // 0 at center, 1 at edge
                // Much tighter ring spacing for more maze-like structure
                float ringSpacing = 2.0f + (1.0f - normalizedDist) * 1.5f;  // 3.5 at center, 2.0 at edge

                int ringNumber = static_cast<int>(distance / ringSpacing);
                float ringDist = distance - (ringNumber * ringSpacing);

                // Narrower passage width for more challenging maze
                float passageWidth = 1.2f + (1.0f - normalizedDist) * 0.8f;  // 2.0 at center, 1.2 at edge

                if (ringDist < passageWidth || distance < 8.0f) {  // Smaller center opening
                    tiles[y][x] = TileType::Empty;
                }
            }
        }
    }

    // Add MORE radial corridors (was 8, now 12)
    int numRadial = 12;  // More spokes for better connectivity
    for (int i = 0; i < numRadial; i++) {
        float angle = (i * 2.0f * M_PI) / numRadial;

        // Draw WIDER lines from center to edge
        for (float r = 0; r < maxRadius; r += 0.3f) {  // Smaller step for continuous line
            int x = centerX + static_cast<int>(r * std::cos(angle));
            int y = centerY + static_cast<int>(r * std::sin(angle));

            if (x >= 1 && x < WIDTH-1 && y >= 1 && y < HEIGHT-1) {
                // Make corridors MUCH WIDER (3x3 instead of 2x2)
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 1 && nx < WIDTH-1 && ny >= 1 && ny < HEIGHT-1) {
                            tiles[ny][nx] = TileType::Empty;
                        }
                    }
                }
            }
        }
    }

    // Add MANY MORE random openings to prevent isolated pockets (was 50, now 200)
    std::uniform_int_distribution<> distX(2, WIDTH-3);
    std::uniform_int_distribution<> distY(2, HEIGHT-3);
    for (int i = 0; i < 200; i++) {
        int x = distX(gen);
        int y = distY(gen);
        int dx = x - centerX;
        int dy = y - centerY;
        float distance = std::sqrt(dx * dx + dy * dy);

        // Open up more areas
        if (distance < maxRadius - 2 && distance > 4) {
            // Create 2x2 openings instead of single tiles
            for (int oy = 0; oy <= 1; oy++) {
                for (int ox = 0; ox <= 1; ox++) {
                    int nx = x + ox;
                    int ny = y + oy;
                    if (nx >= 1 && nx < WIDTH-1 && ny >= 1 && ny < HEIGHT-1) {
                        tiles[ny][nx] = TileType::Empty;
                    }
                }
            }
        }
    }

    // Add random connections between rings to prevent isolated pockets
    for (int i = 0; i < 100; i++) {
        int x = distX(gen);
        int y = distY(gen);
        int dx = x - centerX;
        int dy = y - centerY;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance < maxRadius - 1) {
            // Carve path in random direction
            int dirX = (gen() % 3) - 1;  // -1, 0, or 1
            int dirY = (gen() % 3) - 1;
            for (int step = 0; step < 3; step++) {
                int nx = x + dirX * step;
                int ny = y + dirY * step;
                if (nx >= 1 && nx < WIDTH-1 && ny >= 1 && ny < HEIGHT-1) {
                    tiles[ny][nx] = TileType::Empty;
                }
            }
        }
    }

    // Clear center room (moderate size for maze feel)
    int centerRoomRadius = 8;  // Moderate central room for more maze
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int dx = x - centerX;
            int dy = y - centerY;
            float distance = std::sqrt(dx * dx + dy * dy);

            if (distance <= centerRoomRadius) {
                tiles[y][x] = TileType::Empty;
            }
        }
    }

    // Add dead ends and branching paths for maze complexity
    for (int i = 0; i < 150; i++) {
        int x = distX(gen);
        int y = distY(gen);
        int dx = x - centerX;
        int dy = y - centerY;
        float distance = std::sqrt(dx * dx + dy * dy);

        if (distance < maxRadius - 3 && distance > centerRoomRadius + 2) {
            // Create small dead-end corridors
            int dirX = (gen() % 3) - 1;
            int dirY = (gen() % 3) - 1;
            if (dirX != 0 || dirY != 0) {  // Ensure we have a direction
                for (int step = 0; step < 4; step++) {
                    int nx = x + dirX * step;
                    int ny = y + dirY * step;
                    if (nx >= 1 && nx < WIDTH-1 && ny >= 1 && ny < HEIGHT-1) {
                        tiles[ny][nx] = TileType::Empty;
                    }
                }
            }
        }
    }

    // Place exit in the center and ensure area around it is clear
    tiles[centerY][centerX] = TileType::Exit;

    // Clear 5x5 area around exit to make it visible
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            int nx = centerX + dx;
            int ny = centerY + dy;
            if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT) {
                if (dx == 0 && dy == 0) {
                    tiles[ny][nx] = TileType::Exit;  // Keep center as exit
                } else {
                    tiles[ny][nx] = TileType::Empty;  // Clear surrounding area
                }
            }
        }
    }

    // Ensure borders are walls
    for (int x = 0; x < WIDTH; x++) {
        tiles[0][x] = TileType::Wall;
        tiles[HEIGHT-1][x] = TileType::Wall;
    }
    for (int y = 0; y < HEIGHT; y++) {
        tiles[y][0] = TileType::Wall;
        tiles[y][WIDTH-1] = TileType::Wall;
    }

    // Ensure LARGER start area on outer edge is clear (was 2x2, now 4x4)
    for (int y = 1; y <= 4; y++) {
        for (int x = 1; x <= 4; x++) {
            tiles[y][x] = TileType::Empty;
        }
    }
}

void Maze::carvePassagesFrom(int cx, int cy) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Directions: N, S, E, W
    int dx[] = {0, 0, 1, -1};
    int dy[] = {-1, 1, 0, 0};

    // Shuffle directions for randomness
    std::vector<int> directions = {0, 1, 2, 3};
    std::shuffle(directions.begin(), directions.end(), gen);

    tiles[cy][cx] = TileType::Empty;

    for (int dir : directions) {
        int nx = cx + dx[dir] * 2;
        int ny = cy + dy[dir] * 2;

        if (nx > 0 && nx < WIDTH-1 && ny > 0 && ny < HEIGHT-1 && tiles[ny][nx] == TileType::Wall) {
            // Carve passage
            tiles[cy + dy[dir]][cx + dx[dir]] = TileType::Empty;
            carvePassagesFrom(nx, ny);
        }
    }
}

bool Maze::isValidPosition(int x, int y) const {
    return x > 0 && x < WIDTH-1 && y > 0 && y < HEIGHT-1 &&
           tiles[y][x] == TileType::Empty;
}

std::vector<Vec2> Maze::getRandomKeyPositions(int count) const {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::vector<Vec2> positions;
    std::vector<std::pair<int, int>> validTiles;

    // Collect all valid (empty) tiles that are not near start or exit
    for (int y = 3; y < HEIGHT-3; y++) {
        for (int x = 3; x < WIDTH-3; x++) {
            if (tiles[y][x] == TileType::Empty) {
                // Not too close to start (1,1) or exit (WIDTH-2, HEIGHT-2)
                if ((x > 5 || y > 5) && (x < WIDTH-5 || y < HEIGHT-5)) {
                    validTiles.push_back({x, y});
                }
            }
        }
    }

    // Shuffle and pick random positions
    std::shuffle(validTiles.begin(), validTiles.end(), gen);

    for (int i = 0; i < count && i < static_cast<int>(validTiles.size()); i++) {
        auto [x, y] = validTiles[i];
        positions.push_back({x * TILE_SIZE + TILE_SIZE/2.0f, y * TILE_SIZE + TILE_SIZE/2.0f});
    }

    return positions;
}

std::vector<Vec2> Maze::getRandomZombiePositions(int count, Vec2 playerPos) const {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::vector<Vec2> positions;
    std::vector<std::pair<int, int>> validTiles;

    // Convert player position to tile coordinates
    int playerTileX = static_cast<int>(playerPos.x / TILE_SIZE);
    int playerTileY = static_cast<int>(playerPos.y / TILE_SIZE);

    // Collect all valid (empty) tiles that are far from player spawn
    for (int y = 2; y < HEIGHT-2; y++) {
        for (int x = 2; x < WIDTH-2; x++) {
            if (tiles[y][x] == TileType::Empty) {
                // Calculate distance from player spawn
                int dx = x - playerTileX;
                int dy = y - playerTileY;
                int distSq = dx * dx + dy * dy;

                // Spawn zombies at least 8 tiles away from player (much safer starting distance)
                if (distSq > 64) {  // sqrt(64) = 8 tiles minimum
                    validTiles.push_back({x, y});
                }
            }
        }
    }

    // Shuffle and pick random positions
    std::shuffle(validTiles.begin(), validTiles.end(), gen);

    for (int i = 0; i < count && i < static_cast<int>(validTiles.size()); i++) {
        auto [x, y] = validTiles[i];
        positions.push_back({x * TILE_SIZE + TILE_SIZE/2.0f, y * TILE_SIZE + TILE_SIZE/2.0f});
    }

    return positions;
}

void Maze::render(SDL_Renderer* renderer) const {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            SDL_Rect rect = {x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE};

            switch (tiles[y][x]) {
                case TileType::Wall: {
                    // Shadow layer (offset bottom-right for depth)
                    SDL_Rect shadow = {rect.x + 4, rect.y + 4, rect.w, rect.h};
                    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 180);
                    SDL_RenderFillRect(renderer, &shadow);

                    // Base wall color
                    SDL_SetRenderDrawColor(renderer, 90, 90, 110, 255);
                    SDL_RenderFillRect(renderer, &rect);

                    // Top face (3D illusion)
                    SDL_Rect topFace = {rect.x, rect.y, rect.w, 6};
                    SDL_SetRenderDrawColor(renderer, 140, 140, 160, 255);
                    SDL_RenderFillRect(renderer, &topFace);

                    // Left face
                    SDL_Rect leftFace = {rect.x, rect.y, 6, rect.h};
                    SDL_SetRenderDrawColor(renderer, 115, 115, 135, 255);
                    SDL_RenderFillRect(renderer, &leftFace);

                    // Right face (darker)
                    SDL_Rect rightFace = {rect.x + rect.w - 6, rect.y, 6, rect.h};
                    SDL_SetRenderDrawColor(renderer, 70, 70, 85, 255);
                    SDL_RenderFillRect(renderer, &rightFace);

                    // Bottom face (darkest)
                    SDL_Rect bottomFace = {rect.x, rect.y + rect.h - 6, rect.w, 6};
                    SDL_SetRenderDrawColor(renderer, 55, 55, 70, 255);
                    SDL_RenderFillRect(renderer, &bottomFace);

                    // Corner highlights for extra depth
                    SDL_Rect cornerLight = {rect.x + 2, rect.y + 2, 10, 10};
                    SDL_SetRenderDrawColor(renderer, 160, 160, 180, 255);
                    SDL_RenderFillRect(renderer, &cornerLight);

                    // Border
                    SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
                    SDL_RenderDrawRect(renderer, &rect);
                    break;
                }
                case TileType::Exit: {
                    // Door background (wood texture)
                    SDL_SetRenderDrawColor(renderer, 101, 67, 33, 255);
                    SDL_RenderFillRect(renderer, &rect);

                    // Door panels (raised sections)
                    SDL_Rect topPanel = {rect.x + 4, rect.y + 3, rect.w - 8, 10};
                    SDL_SetRenderDrawColor(renderer, 121, 85, 45, 255);
                    SDL_RenderFillRect(renderer, &topPanel);
                    SDL_SetRenderDrawColor(renderer, 80, 55, 25, 255);
                    SDL_RenderDrawRect(renderer, &topPanel);

                    SDL_Rect bottomPanel = {rect.x + 4, rect.y + rect.h - 13, rect.w - 8, 10};
                    SDL_SetRenderDrawColor(renderer, 121, 85, 45, 255);
                    SDL_RenderFillRect(renderer, &bottomPanel);
                    SDL_SetRenderDrawColor(renderer, 80, 55, 25, 255);
                    SDL_RenderDrawRect(renderer, &bottomPanel);

                    // Door handle/lock plate (metallic)
                    SDL_Rect lockPlate = {rect.x + rect.w/2 - 6, rect.y + rect.h/2 - 4, 12, 8};
                    SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
                    SDL_RenderFillRect(renderer, &lockPlate);
                    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                    SDL_RenderDrawRect(renderer, &lockPlate);

                    // Keyhole (dark center)
                    SDL_Rect keyhole = {rect.x + rect.w/2 - 2, rect.y + rect.h/2 - 2, 4, 4};
                    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
                    SDL_RenderFillRect(renderer, &keyhole);

                    // Glow effect around door (unlocked state indicator)
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 100);
                    SDL_Rect glowTop = {rect.x, rect.y - 2, rect.w, 2};
                    SDL_RenderFillRect(renderer, &glowTop);
                    SDL_Rect glowBottom = {rect.x, rect.y + rect.h, rect.w, 2};
                    SDL_RenderFillRect(renderer, &glowBottom);
                    SDL_Rect glowLeft = {rect.x - 2, rect.y, 2, rect.h};
                    SDL_RenderFillRect(renderer, &glowLeft);
                    SDL_Rect glowRight = {rect.x + rect.w, rect.y, 2, rect.h};
                    SDL_RenderFillRect(renderer, &glowRight);

                    // Door frame
                    SDL_SetRenderDrawColor(renderer, 60, 40, 20, 255);
                    SDL_RenderDrawRect(renderer, &rect);
                    break;
                }
                case TileType::Empty: {
                    // Floor with slight grid pattern
                    SDL_SetRenderDrawColor(renderer, 35, 35, 35, 255);
                    SDL_RenderFillRect(renderer, &rect);

                    // Grid lines for depth
                    SDL_SetRenderDrawColor(renderer, 45, 45, 45, 255);
                    SDL_RenderDrawRect(renderer, &rect);
                    break;
                }
            }
        }
    }
}

bool Maze::isWall(int x, int y) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return true;
    return tiles[y][x] == TileType::Wall;
}

bool Maze::isExit(int x, int y) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return false;
    return tiles[y][x] == TileType::Exit;
}

TileType Maze::getTile(int x, int y) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return TileType::Wall;
    return tiles[y][x];
}

Vec2 Maze::getExitPos() const {
    return {(WIDTH-2) * TILE_SIZE + TILE_SIZE/2.0f, (HEIGHT-2) * TILE_SIZE + TILE_SIZE/2.0f};
}

Vec2 Maze::getSpawnPositionAwayFromZombies(const std::vector<Vec2>& existingZombies, Vec2 playerPos) const {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::vector<std::pair<int, int>> validTiles;

    // Convert player position to tile coordinates
    int playerTileX = static_cast<int>(playerPos.x / TILE_SIZE);
    int playerTileY = static_cast<int>(playerPos.y / TILE_SIZE);

    // Collect all valid (empty) tiles
    for (int y = 2; y < HEIGHT-2; y++) {
        for (int x = 2; x < WIDTH-2; x++) {
            if (tiles[y][x] == TileType::Empty) {
                // Must be far from player
                int dx = x - playerTileX;
                int dy = y - playerTileY;
                int distToPlayerSq = dx * dx + dy * dy;

                if (distToPlayerSq > 100) {  // At least 10 tiles from player
                    // Check distance to all existing zombies
                    float minZombieDist = 999999.0f;
                    for (const auto& zombiePos : existingZombies) {
                        int zx = static_cast<int>(zombiePos.x / TILE_SIZE);
                        int zy = static_cast<int>(zombiePos.y / TILE_SIZE);
                        int zdx = x - zx;
                        int zdy = y - zy;
                        float zombieDist = std::sqrt(static_cast<float>(zdx * zdx + zdy * zdy));
                        if (zombieDist < minZombieDist) {
                            minZombieDist = zombieDist;
                        }
                    }

                    // Prefer locations far from other zombies
                    if (minZombieDist > 8.0f) {  // At least 8 tiles from nearest zombie
                        validTiles.push_back({x, y});
                    }
                }
            }
        }
    }

    // If no ideal spots, lower the requirements
    if (validTiles.empty()) {
        for (int y = 2; y < HEIGHT-2; y++) {
            for (int x = 2; x < WIDTH-2; x++) {
                if (tiles[y][x] == TileType::Empty) {
                    int dx = x - playerTileX;
                    int dy = y - playerTileY;
                    int distToPlayerSq = dx * dx + dy * dy;
                    if (distToPlayerSq > 64) {  // At least 8 tiles from player
                        validTiles.push_back({x, y});
                    }
                }
            }
        }
    }

    // Pick random position
    if (!validTiles.empty()) {
        std::uniform_int_distribution<> dis(0, validTiles.size() - 1);
        auto [x, y] = validTiles[dis(gen)];
        return {x * TILE_SIZE + TILE_SIZE/2.0f, y * TILE_SIZE + TILE_SIZE/2.0f};
    }

    // Fallback
    return {5 * TILE_SIZE + TILE_SIZE/2.0f, 5 * TILE_SIZE + TILE_SIZE/2.0f};
}

Vec2 Maze::getPlayerStart() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::vector<std::pair<int, int>> validTiles;

    int centerX = WIDTH / 2;
    int centerY = HEIGHT / 2;
    int centerRoomRadius = 5;  // Stay away from 5+ tiles around center (for standard mode)

    // Collect all valid (empty) tiles that are not near the exit or center room
    for (int y = 2; y < HEIGHT-3; y++) {
        for (int x = 2; x < WIDTH-3; x++) {
            if (tiles[y][x] == TileType::Empty) {
                // Not too close to exit
                int distToExit = std::abs(x - (WIDTH-2)) + std::abs(y - (HEIGHT-2));

                // For STANDARD mode only: also avoid center room
                bool tooCloseToCenter = false;
                if (mazeType == MazeType::STANDARD) {
                    int distToCenter = std::max(std::abs(x - centerX), std::abs(y - centerY));
                    tooCloseToCenter = (distToCenter <= centerRoomRadius);
                }

                if (distToExit > 5 && !tooCloseToCenter) {
                    validTiles.push_back({x, y});
                }
            }
        }
    }

    if (validTiles.empty()) {
        // Fallback to corner if no valid tiles found
        return {2 * TILE_SIZE + TILE_SIZE/2.0f, 2 * TILE_SIZE + TILE_SIZE/2.0f};
    }

    // Pick a random tile
    std::uniform_int_distribution<> dis(0, validTiles.size() - 1);
    auto [x, y] = validTiles[dis(gen)];

    return {x * TILE_SIZE + TILE_SIZE/2.0f, y * TILE_SIZE + TILE_SIZE/2.0f};
}

void Maze::generateInfiniteMaze() {
    // Infinite mode uses standard maze generation
    // The "infinite" aspect is handled by regenerating the maze when player completes it
    generateRandomMaze();
}

void Maze::generateSoldierMaze() {
    // SOLDIER MODE: Large open arena in center with 4 corner rooms connected by wide hallways
    // Fill with walls first
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            tiles[y][x] = TileType::Wall;
        }
    }

    // Define arena boundaries (large open center area - 50% of map)
    int arenaLeft = WIDTH / 4;
    int arenaRight = 3 * WIDTH / 4;
    int arenaTop = HEIGHT / 4;
    int arenaBottom = 3 * HEIGHT / 4;

    // Create large open arena in center
    for (int y = arenaTop; y <= arenaBottom; y++) {
        for (int x = arenaLeft; x <= arenaRight; x++) {
            tiles[y][x] = TileType::Empty;
        }
    }

    // Add some cover/obstacles in the arena (scattered walls for tactical gameplay)
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distObstacles(0, 100);
    for (int y = arenaTop + 3; y < arenaBottom - 3; y += 4) {
        for (int x = arenaLeft + 3; x < arenaRight - 3; x += 4) {
            if (distObstacles(gen) < 25) {  // 25% chance for cover
                // Create small 2x2 cover
                tiles[y][x] = TileType::Wall;
                tiles[y][x+1] = TileType::Wall;
                tiles[y+1][x] = TileType::Wall;
                tiles[y+1][x+1] = TileType::Wall;
            }
        }
    }

    // Create 4 SIMPLE corner rooms (just open rectangles, not mazes)
    // Top-left corner room
    for (int y = 3; y < arenaTop - 2; y++) {
        for (int x = 3; x < arenaLeft - 2; x++) {
            tiles[y][x] = TileType::Empty;
        }
    }

    // Top-right corner room
    for (int y = 3; y < arenaTop - 2; y++) {
        for (int x = arenaRight + 3; x < WIDTH - 3; x++) {
            tiles[y][x] = TileType::Empty;
        }
    }

    // Bottom-left corner room
    for (int y = arenaBottom + 3; y < HEIGHT - 3; y++) {
        for (int x = 3; x < arenaLeft - 2; x++) {
            tiles[y][x] = TileType::Empty;
        }
    }

    // Bottom-right corner room
    for (int y = arenaBottom + 3; y < HEIGHT - 3; y++) {
        for (int x = arenaRight + 3; x < WIDTH - 3; x++) {
            tiles[y][x] = TileType::Empty;
        }
    }

    // Create VERY WIDE hallways from each corner to the arena center
    // Each hallway is 9 tiles wide for easy zombie passage

    // Top-left: Horizontal hallway at corner room mid-height
    int tlHallY = (3 + arenaTop - 2) / 2;
    for (int x = 3; x <= arenaLeft; x++) {
        for (int dy = -4; dy <= 4; dy++) {
            int y = tlHallY + dy;
            if (y >= 1 && y < HEIGHT - 1) {
                tiles[y][x] = TileType::Empty;
            }
        }
    }

    // Top-right: Horizontal hallway
    int trHallY = (3 + arenaTop - 2) / 2;
    for (int x = arenaRight; x < WIDTH - 3; x++) {
        for (int dy = -4; dy <= 4; dy++) {
            int y = trHallY + dy;
            if (y >= 1 && y < HEIGHT - 1) {
                tiles[y][x] = TileType::Empty;
            }
        }
    }

    // Bottom-left: Horizontal hallway
    int blHallY = (arenaBottom + 3 + HEIGHT - 3) / 2;
    for (int x = 3; x <= arenaLeft; x++) {
        for (int dy = -4; dy <= 4; dy++) {
            int y = blHallY + dy;
            if (y >= 1 && y < HEIGHT - 1) {
                tiles[y][x] = TileType::Empty;
            }
        }
    }

    // Bottom-right: Horizontal hallway
    int brHallY = (arenaBottom + 3 + HEIGHT - 3) / 2;
    for (int x = arenaRight; x < WIDTH - 3; x++) {
        for (int dy = -4; dy <= 4; dy++) {
            int y = brHallY + dy;
            if (y >= 1 && y < HEIGHT - 1) {
                tiles[y][x] = TileType::Empty;
            }
        }
    }

    // Add 2-block-wide hallways connecting all corner rooms
    // Top horizontal hallway (connecting top-left to top-right)
    int topHallY = (3 + arenaTop - 2) / 2;
    for (int x = 3; x < WIDTH - 3; x++) {
        for (int dy = 0; dy <= 1; dy++) {
            int y = topHallY + dy;
            if (y >= 1 && y < HEIGHT - 1) {
                tiles[y][x] = TileType::Empty;
            }
        }
    }

    // Bottom horizontal hallway (connecting bottom-left to bottom-right)
    int bottomHallY = (arenaBottom + 3 + HEIGHT - 3) / 2;
    for (int x = 3; x < WIDTH - 3; x++) {
        for (int dy = 0; dy <= 1; dy++) {
            int y = bottomHallY + dy;
            if (y >= 1 && y < HEIGHT - 1) {
                tiles[y][x] = TileType::Empty;
            }
        }
    }

    // Left vertical hallway (connecting top-left to bottom-left)
    int leftHallX = (3 + arenaLeft - 2) / 2;
    for (int y = 3; y < HEIGHT - 3; y++) {
        for (int dx = 0; dx <= 1; dx++) {
            int x = leftHallX + dx;
            if (x >= 1 && x < WIDTH - 1) {
                tiles[y][x] = TileType::Empty;
            }
        }
    }

    // Right vertical hallway (connecting top-right to bottom-right)
    int rightHallX = (arenaRight + 3 + WIDTH - 3) / 2;
    for (int y = 3; y < HEIGHT - 3; y++) {
        for (int dx = 0; dx <= 1; dx++) {
            int x = rightHallX + dx;
            if (x >= 1 && x < WIDTH - 1) {
                tiles[y][x] = TileType::Empty;
            }
        }
    }

    // No exit in soldier mode (wave survival)
    // Borders remain walls
}

std::vector<Vec2> Maze::getAllExitPositions() const {
    if (mazeType == MazeType::INFINITE) {
        return exitPositions;
    }

    // For non-infinite modes, return single exit position
    std::vector<Vec2> positions;

    if (mazeType == MazeType::CIRCULAR) {
        // Center exit for circular
        positions.push_back({(WIDTH/2) * TILE_SIZE + TILE_SIZE/2.0f, (HEIGHT/2) * TILE_SIZE + TILE_SIZE/2.0f});
    } else {
        // Corner exit for standard
        positions.push_back({(WIDTH-2) * TILE_SIZE + TILE_SIZE/2.0f, (HEIGHT-2) * TILE_SIZE + TILE_SIZE/2.0f});
    }

    return positions;
}

int Maze::getRequiredKeyCount() const {
    return 5;  // All modes use 5 keys (except infinite with level progression)
}

int Maze::getRequiredKeyCount(int level) const {
    if (mazeType == MazeType::INFINITE) {
        // Progressive: 1 key at level 1, 2 keys at level 2, ... cap at 7 keys
        return std::min(level, 7);
    }
    return getRequiredKeyCount();  // Default for other modes
}
