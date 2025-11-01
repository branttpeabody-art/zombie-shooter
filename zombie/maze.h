#ifndef ZOMBIE_MAZE_H
#define ZOMBIE_MAZE_H

#include <SDL.h>
#include <vector>

enum class TileType {
    Empty,
    Wall,
    Exit,
    SafeRoom  // Blue safe room for evacuation events
};

enum class MazeType {
    STANDARD,
    CIRCULAR,
    INFINITE,
    SOLDIER  // Arena mode with open center and spawn points
};

struct Vec2 {
    float x, y;
};

class Maze {
public:
    static constexpr int TILE_SIZE = 30;
    static constexpr int WIDTH = 32;  // Nice middle ground
    static constexpr int HEIGHT = 24; // More manageable size

    Maze(MazeType type = MazeType::STANDARD);

    void render(SDL_Renderer* renderer) const;
    bool isWall(int x, int y) const;
    bool isExit(int x, int y) const;
    bool isSafeRoom(int x, int y) const;
    TileType getTile(int x, int y) const;

    Vec2 getPlayerStart() const;
    Vec2 getExitPos() const;
    Vec2 getSafeRoomPos() const;

    std::vector<Vec2> getRandomKeyPositions(int count) const;
    std::vector<Vec2> getRandomZombiePositions(int count, Vec2 playerPos) const;
    Vec2 getSpawnPositionAwayFromZombies(const std::vector<Vec2>& existingZombies, Vec2 playerPos) const;

    std::vector<Vec2> getAllExitPositions() const;
    int getRequiredKeyCount() const;
    int getRequiredKeyCount(int level) const;  // For progressive infinite mode

private:
    std::vector<std::vector<TileType>> tiles;
    MazeType mazeType;
    std::vector<Vec2> exitPositions;  // Store multiple exit positions for infinite mode
    Vec2 safeRoomPos;  // Blue safe room position for evacuation events
    void generateRandomMaze();
    void generateCircularMaze();
    void generateInfiniteMaze();
    void generateSoldierMaze();
    void carvePassagesFrom(int cx, int cy);
    bool isValidPosition(int x, int y) const;
};

#endif
