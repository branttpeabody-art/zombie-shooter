#include "game.h"
#include "maze.h"
#include "player.h"
#include "zombie.h"
#include "bullet.h"
#include "key.h"
#include "weapon.h"
#include "healthboost.h"
#include <SDL_mixer.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>
#include <string>
#include <cstdlib>

namespace {
    // Sound effects
    Mix_Chunk* shootSound = nullptr;
    Mix_Chunk* zombieDeathSound = nullptr;
    Mix_Chunk* keySound = nullptr;
    Mix_Chunk* playerDeathSound = nullptr;
    Mix_Chunk* zombieGroanSound = nullptr;  // Creepy zombie groan
    Mix_Chunk* zombieMoanSound = nullptr;   // Low zombie moan
    Mix_Chunk* proximityBeepSound = nullptr; // Beep for nearby zombies

    // Generate a simple beep sound
    Mix_Chunk* createBeepSound(int frequency, int duration, int volume) {
        int sampleRate = 22050;
        int samples = (sampleRate * duration) / 1000;

        Uint8* buffer = new Uint8[samples];
        for (int i = 0; i < samples; i++) {
            double time = static_cast<double>(i) / sampleRate;
            double value = std::sin(2.0 * M_PI * frequency * time) * volume;
            buffer[i] = static_cast<Uint8>(128 + value);
        }

        Mix_Chunk* chunk = new Mix_Chunk();
        chunk->allocated = 1;
        chunk->abuf = buffer;
        chunk->alen = samples;
        chunk->volume = MIX_MAX_VOLUME;

        return chunk;
    }

    Mix_Chunk* createGunshotSound(int duration, int volume) {
        int sampleRate = 22050;
        int samples = (sampleRate * duration) / 1000;

        Uint8* buffer = new Uint8[samples];
        std::srand(12345);  // Fixed seed for consistent sound

        for (int i = 0; i < samples; i++) {
            // White noise
            float noise = (static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f);

            // Exponential decay envelope for realistic bang
            float envelope = std::exp(-static_cast<float>(i) / (samples / 8.0f));

            // Add some low frequency boom
            double time = static_cast<double>(i) / sampleRate;
            float boom = std::sin(2.0 * M_PI * 80.0 * time) * 0.3f * envelope;

            // Combine noise and boom
            float value = (noise * 0.7f + boom) * envelope * volume;

            // Clamp and convert to Uint8
            int intValue = static_cast<int>(value) + 128;
            if (intValue < 0) intValue = 0;
            if (intValue > 255) intValue = 255;
            buffer[i] = static_cast<Uint8>(intValue);
        }

        Mix_Chunk* chunk = new Mix_Chunk();
        chunk->allocated = 1;
        chunk->abuf = buffer;
        chunk->alen = samples;
        chunk->volume = MIX_MAX_VOLUME;

        return chunk;
    }

    // Create creepy zombie groan sound (low frequency warbling)
    Mix_Chunk* createZombieGroanSound(int duration, int volume) {
        int sampleRate = 22050;
        int samples = (sampleRate * duration) / 1000;

        Uint8* buffer = new Uint8[samples];

        for (int i = 0; i < samples; i++) {
            double time = static_cast<double>(i) / sampleRate;

            // Multiple low frequencies for creepy effect
            float base = std::sin(2.0 * M_PI * 80.0 * time);     // Deep bass
            float warble = std::sin(2.0 * M_PI * 120.0 * time);  // Warbling
            float rasp = std::sin(2.0 * M_PI * 200.0 * time) * 0.3f;  // Raspy overlay

            // Add some noise for texture
            float noise = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * 0.15f;

            // Fade in and out envelope
            float envelope = std::sin(M_PI * static_cast<float>(i) / samples);

            // Combine all elements
            float value = (base * 0.5f + warble * 0.3f + rasp + noise) * envelope * volume;

            int intValue = static_cast<int>(value) + 128;
            if (intValue < 0) intValue = 0;
            if (intValue > 255) intValue = 255;
            buffer[i] = static_cast<Uint8>(intValue);
        }

        Mix_Chunk* chunk = new Mix_Chunk();
        chunk->allocated = 1;
        chunk->abuf = buffer;
        chunk->alen = samples;
        chunk->volume = MIX_MAX_VOLUME;

        return chunk;
    }

    void initSounds() {
        shootSound = createGunshotSound(150, 60);            // Realistic gunshot bang
        zombieDeathSound = createBeepSound(200, 300, 40);    // Low pitch, longer
        keySound = createBeepSound(1200, 150, 35);           // Very high pitch
        playerDeathSound = createBeepSound(150, 500, 50);    // Very low pitch, long
        zombieGroanSound = createZombieGroanSound(800, 25);  // Creepy long groan
        zombieMoanSound = createZombieGroanSound(600, 20);   // Shorter moan
        proximityBeepSound = createBeepSound(800, 80, 30);   // Short, moderate pitch beep
    }

    void cleanupSounds() {
        if (shootSound) {
            delete[] shootSound->abuf;
            delete shootSound;
        }
        if (zombieDeathSound) {
            delete[] zombieDeathSound->abuf;
            delete zombieDeathSound;
        }
        if (keySound) {
            delete[] keySound->abuf;
            delete keySound;
        }
        if (playerDeathSound) {
            delete[] playerDeathSound->abuf;
            delete playerDeathSound;
        }
        if (zombieGroanSound) {
            delete[] zombieGroanSound->abuf;
            delete zombieGroanSound;
        }
        if (zombieMoanSound) {
            delete[] zombieMoanSound->abuf;
            delete zombieMoanSound;
        }
        if (proximityBeepSound) {
            delete[] proximityBeepSound->abuf;
            delete proximityBeepSound;
        }
    }

    // Helper function to randomly select a zombie type
    ZombieType getRandomZombieType() {
        int roll = rand() % 100;  // 0-99

        // Distribution:
        // 50% NORMAL (0-49)
        // 25% FAST (50-74)
        // 15% TANK (75-89)
        // 10% RUNNER (90-99)

        if (roll < 50) {
            return ZombieType::NORMAL;
        } else if (roll < 75) {
            return ZombieType::FAST;
        } else if (roll < 90) {
            return ZombieType::TANK;
        } else {
            return ZombieType::RUNNER;
        }
    }

    // High Score System
    struct HighScoreEntry {
        int score;
        int level;
        std::string mazeType;
        std::string difficulty;
    };

    std::vector<HighScoreEntry> highScores;
    constexpr int MAX_HIGH_SCORES = 10;

    std::string getHighScorePath() {
        const char* home = std::getenv("HOME");
        if (home) {
            return std::string(home) + "/.zombie_highscores.txt";
        }
        return ".zombie_highscores.txt";  // Fallback to current directory
    }

    void loadHighScores() {
        highScores.clear();
        std::ifstream file(getHighScorePath());
        if (!file.is_open()) return;

        std::string line;
        while (std::getline(file, line) && highScores.size() < MAX_HIGH_SCORES) {
            std::istringstream iss(line);
            HighScoreEntry entry;
            if (iss >> entry.score >> entry.level >> entry.mazeType >> entry.difficulty) {
                highScores.push_back(entry);
            }
        }
        file.close();
    }

    void saveHighScores() {
        std::ofstream file(getHighScorePath());
        if (!file.is_open()) {
            std::cerr << "Failed to save high scores" << std::endl;
            return;
        }

        for (const auto& entry : highScores) {
            file << entry.score << " " << entry.level << " "
                 << entry.mazeType << " " << entry.difficulty << "\n";
        }
        file.close();
    }

    void addHighScore(int score, int level, const std::string& mazeType, const std::string& difficulty) {
        HighScoreEntry newEntry = {score, level, mazeType, difficulty};

        // Add to list
        highScores.push_back(newEntry);

        // Sort by score (descending)
        std::sort(highScores.begin(), highScores.end(),
                  [](const HighScoreEntry& a, const HighScoreEntry& b) {
                      return a.score > b.score;
                  });

        // Keep only top MAX_HIGH_SCORES
        if (highScores.size() > MAX_HIGH_SCORES) {
            highScores.resize(MAX_HIGH_SCORES);
        }

        saveHighScores();
    }

    bool isHighScore(int score) {
        return highScores.size() < MAX_HIGH_SCORES || score > highScores.back().score;
    }

    std::string mazeTypeToString(MazeType type) {
        switch (type) {
            case MazeType::STANDARD: return "Standard";
            case MazeType::CIRCULAR: return "Circular";
            case MazeType::INFINITE: return "Infinite";
            case MazeType::SOLDIER: return "Soldier";
            default: return "Unknown";
        }
    }

    std::string difficultyToString(Difficulty diff) {
        switch (diff) {
            case Difficulty::EASY: return "Easy";
            case Difficulty::NORMAL: return "Normal";
            case Difficulty::HARD: return "Hard";
            case Difficulty::TESTING: return "Testing";
            default: return "Unknown";
        }
    }

    struct PlayState {
        std::unique_ptr<Maze> maze;
        std::unique_ptr<Player> player;
        std::vector<std::unique_ptr<Zombie>> zombies;
        std::vector<std::unique_ptr<Key>> keys;
        std::vector<std::unique_ptr<Bullet>> bullets;
        std::vector<std::unique_ptr<WeaponPickup>> weaponPickups;
        std::vector<std::unique_ptr<HealthBoost>> healthBoosts;
        Uint32 deathTime = 0;

        // Score tracking
        int score = 0;
        int zombiesKilled = 0;
        int totalScore = 0;  // Persistent score across respawns
        Uint32 gameStartTime = 0;

        // Zombie spawning
        float spawnTimer = 0.0f;
        int totalZombiesSpawned = 0;
        int initialZombieCount = 15;
        int minZombieCount = 8;
        int maxZombieCount = 25;
        int zombieMaxHealth = 3;  // Health per zombie based on difficulty

        // Screen shake
        float screenShake = 0.0f;
        float shakeOffsetX = 0.0f;
        float shakeOffsetY = 0.0f;

        // Proximity beep system
        float proximityBeepTimer = 0.0f;
        float proximityBeepInterval = 2.0f;  // Start at 2 seconds

        // HUD toggles
        bool showScore = true;
        bool showMinimap = true;
        bool showArrow = true;

        // Infinite mode tracking
        int currentLevel = 1;  // Track which level/wave the player is on
        MazeType mazeType = MazeType::STANDARD;  // Track the current maze type

        // Soldier mode wave tracking
        int currentWave = 1;
        bool waveActive = false;
        float waveDelayTimer = 0.0f;
        float waveDelay = 5.0f;  // 5 seconds between waves

        // Difficulty tracking (for testing mode checks)
        Difficulty difficulty = Difficulty::NORMAL;

        // Testing/Debug panel (F1 to toggle, only works in TESTING mode)
        bool showTestingPanel = false;
        bool godMode = false;  // Immortality
        int selectedWeaponSpawn = 0;  // For weapon spawning dropdown
        int selectedZombieType = 0;  // For zombie spawning
        bool spawnAtCrosshair = false;  // If true, spawn at crosshair; if false, spawn at player

        // Blood Moon event (happens every 2 minutes for 30 seconds)
        float bloodMoonTimer = 0.0f;
        bool bloodMoonActive = false;
        float bloodMoonDuration = 30.0f;  // 30 seconds of blood moon
        float bloodMoonInterval = 120.0f;  // Every 2 minutes (120 seconds)
        float bloodMoonSpawnMultiplier = 8.0f;  // Spawn 8x more zombies during blood moon!

        // Blue Alert event - player must evacuate to safe room
        float blueAlertTimer = 0.0f;
        bool blueAlertActive = false;
        float blueAlertDuration = 45.0f;  // 45 seconds to reach safe room
        float blueAlertInterval = 180.0f;  // Every 3 minutes (180 seconds)
        int blueRoomX = -1, blueRoomY = -1;  // Safe room coordinates
        bool inSafeRoom = false;  // Is player currently in safe room
        bool safeRoomLocked = false;  // Room locks after alert ends

        // Hunter Phase - dark entities chase player after failed evacuation
        bool hunterPhaseActive = false;
        float hunterPhaseTimer = 0.0f;
        float hunterPhaseDuration = 60.0f;  // 1 minute of terror!
        std::vector<std::unique_ptr<Zombie>> hunters;  // Dark fast entities
    };

    struct MenuState {
        GameState currentState = GameState::MENU;
        Difficulty difficulty = Difficulty::NORMAL;
        MazeType mazeType = MazeType::STANDARD;
        int menuSelection = 0;  // 0=Start, 1=Maze Type, 2=Difficulty, 3=Controls, 4=Quit
        int mazeTypeSelection = 0;  // 0=Standard, 1=Circular, 2=Infinite, 3=Soldier
        int difficultySelection = 1;  // 0=Easy, 1=Normal, 2=Hard, 3=Testing
        int pauseSelection = 0;  // 0=Resume, 1=Restart, 2=Difficulty, 3=Main Menu, 4=Quit
        bool devModeUnlocked = false;  // Whether dev mode has been unlocked
        std::string codeEntry = "";  // Current code being entered
        bool codeError = false;  // Whether there was an error
    };

    constexpr int INITIAL_ZOMBIE_COUNT = 15;  // Start with more zombies for bigger maze
    constexpr int MIN_ZOMBIE_COUNT = 8;  // Spawn more when below this
    constexpr int MAX_ZOMBIE_COUNT = 25;  // Don't spawn more than this
    constexpr float SPAWN_CHECK_INTERVAL = 5.0f;  // Check every 5 seconds

    // Simple text rendering using rectangles
    void renderChar(SDL_Renderer* renderer, char c, int x, int y, int size) {
        // Simple 5x7 bitmap font patterns
        bool pixels[7][5] = {false};

        switch(c) {
            case 'A':
                pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][4] = true;
                pixels[3][0] = pixels[3][1] = pixels[3][2] = pixels[3][3] = pixels[3][4] = true;
                pixels[4][0] = pixels[4][4] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][0] = pixels[6][4] = true;
                break;
            case 'B':
                pixels[0][0] = pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][4] = true;
                pixels[3][0] = pixels[3][1] = pixels[3][2] = pixels[3][3] = true;
                pixels[4][0] = pixels[4][4] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][0] = pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case 'C':
                pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = true;
                pixels[3][0] = true;
                pixels[4][0] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case 'D':
                pixels[0][0] = pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][4] = true;
                pixels[3][0] = pixels[3][4] = true;
                pixels[4][0] = pixels[4][4] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][0] = pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case 'E':
                pixels[0][0] = pixels[0][1] = pixels[0][2] = pixels[0][3] = pixels[0][4] = true;
                pixels[1][0] = true;
                pixels[2][0] = true;
                pixels[3][0] = pixels[3][1] = pixels[3][2] = pixels[3][3] = true;
                pixels[4][0] = true;
                pixels[5][0] = true;
                pixels[6][0] = pixels[6][1] = pixels[6][2] = pixels[6][3] = pixels[6][4] = true;
                break;
            case 'F':
                pixels[0][0] = pixels[0][1] = pixels[0][2] = pixels[0][3] = pixels[0][4] = true;
                pixels[1][0] = true;
                pixels[2][0] = true;
                pixels[3][0] = pixels[3][1] = pixels[3][2] = pixels[3][3] = true;
                pixels[4][0] = true;
                pixels[5][0] = true;
                pixels[6][0] = true;
                break;
            case 'G':
                pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = true;
                pixels[3][0] = pixels[3][2] = pixels[3][3] = pixels[3][4] = true;
                pixels[4][0] = pixels[4][4] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case 'H':
                pixels[0][0] = pixels[0][4] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][4] = true;
                pixels[3][0] = pixels[3][1] = pixels[3][2] = pixels[3][3] = pixels[3][4] = true;
                pixels[4][0] = pixels[4][4] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][0] = pixels[6][4] = true;
                break;
            case 'I':
                pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][2] = true;
                pixels[2][2] = true;
                pixels[3][2] = true;
                pixels[4][2] = true;
                pixels[5][2] = true;
                pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case 'L':
                pixels[0][0] = true;
                pixels[1][0] = true;
                pixels[2][0] = true;
                pixels[3][0] = true;
                pixels[4][0] = true;
                pixels[5][0] = true;
                pixels[6][0] = pixels[6][1] = pixels[6][2] = pixels[6][3] = pixels[6][4] = true;
                break;
            case 'M':
                pixels[0][0] = pixels[0][4] = true;
                pixels[1][0] = pixels[1][1] = pixels[1][3] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][2] = pixels[2][4] = true;
                pixels[3][0] = pixels[3][4] = true;
                pixels[4][0] = pixels[4][4] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][0] = pixels[6][4] = true;
                break;
            case 'N':
                pixels[0][0] = pixels[0][4] = true;
                pixels[1][0] = pixels[1][1] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][2] = pixels[2][4] = true;
                pixels[3][0] = pixels[3][2] = pixels[3][4] = true;
                pixels[4][0] = pixels[4][3] = pixels[4][4] = true;
                pixels[5][0] = pixels[5][3] = pixels[5][4] = true;
                pixels[6][0] = pixels[6][4] = true;
                break;
            case 'O':
                pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][4] = true;
                pixels[3][0] = pixels[3][4] = true;
                pixels[4][0] = pixels[4][4] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case 'P':
                pixels[0][0] = pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][4] = true;
                pixels[3][0] = pixels[3][1] = pixels[3][2] = pixels[3][3] = true;
                pixels[4][0] = true;
                pixels[5][0] = true;
                pixels[6][0] = true;
                break;
            case 'Q':
                pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][4] = true;
                pixels[3][0] = pixels[3][4] = true;
                pixels[4][0] = pixels[4][2] = pixels[4][4] = true;
                pixels[5][0] = pixels[5][3] = pixels[5][4] = true;
                pixels[6][1] = pixels[6][2] = pixels[6][3] = pixels[6][4] = true;
                break;
            case 'R':
                pixels[0][0] = pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][4] = true;
                pixels[3][0] = pixels[3][1] = pixels[3][2] = pixels[3][3] = true;
                pixels[4][0] = pixels[4][2] = true;
                pixels[5][0] = pixels[5][3] = true;
                pixels[6][0] = pixels[6][4] = true;
                break;
            case 'S':
                pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = true;
                pixels[3][1] = pixels[3][2] = pixels[3][3] = true;
                pixels[4][4] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case 'T':
                pixels[0][0] = pixels[0][1] = pixels[0][2] = pixels[0][3] = pixels[0][4] = true;
                pixels[1][2] = true;
                pixels[2][2] = true;
                pixels[3][2] = true;
                pixels[4][2] = true;
                pixels[5][2] = true;
                pixels[6][2] = true;
                break;
            case 'U':
                pixels[0][0] = pixels[0][4] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][4] = true;
                pixels[3][0] = pixels[3][4] = true;
                pixels[4][0] = pixels[4][4] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case 'Y':
                pixels[0][0] = pixels[0][4] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][1] = pixels[2][3] = true;
                pixels[3][2] = true;
                pixels[4][2] = true;
                pixels[5][2] = true;
                pixels[6][2] = true;
                break;
            case 'W':
                pixels[0][0] = pixels[0][4] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][4] = true;
                pixels[3][0] = pixels[3][4] = true;
                pixels[4][0] = pixels[4][2] = pixels[4][4] = true;
                pixels[5][0] = pixels[5][1] = pixels[5][3] = pixels[5][4] = true;
                pixels[6][0] = pixels[6][4] = true;
                break;
            case 'Z':
                pixels[0][0] = pixels[0][1] = pixels[0][2] = pixels[0][3] = pixels[0][4] = true;
                pixels[1][4] = true;
                pixels[2][3] = true;
                pixels[3][2] = true;
                pixels[4][1] = true;
                pixels[5][0] = true;
                pixels[6][0] = pixels[6][1] = pixels[6][2] = pixels[6][3] = pixels[6][4] = true;
                break;
            case ' ':
                break;
            case '0':
                pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][4] = true;
                pixels[3][0] = pixels[3][4] = true;
                pixels[4][0] = pixels[4][4] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case '1':
                pixels[0][2] = true;
                pixels[1][1] = pixels[1][2] = true;
                pixels[2][2] = true;
                pixels[3][2] = true;
                pixels[4][2] = true;
                pixels[5][2] = true;
                pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case '2':
                pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][4] = true;
                pixels[3][1] = pixels[3][2] = pixels[3][3] = true;
                pixels[4][0] = true;
                pixels[5][0] = true;
                pixels[6][0] = pixels[6][1] = pixels[6][2] = pixels[6][3] = pixels[6][4] = true;
                break;
            case '3':
                pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][4] = true;
                pixels[3][1] = pixels[3][2] = pixels[3][3] = true;
                pixels[4][4] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case '4':
                pixels[0][0] = pixels[0][4] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][4] = true;
                pixels[3][0] = pixels[3][1] = pixels[3][2] = pixels[3][3] = pixels[3][4] = true;
                pixels[4][4] = true;
                pixels[5][4] = true;
                pixels[6][4] = true;
                break;
            case '5':
                pixels[0][0] = pixels[0][1] = pixels[0][2] = pixels[0][3] = pixels[0][4] = true;
                pixels[1][0] = true;
                pixels[2][0] = true;
                pixels[3][0] = pixels[3][1] = pixels[3][2] = pixels[3][3] = true;
                pixels[4][4] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case '6':
                pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = true;
                pixels[2][0] = true;
                pixels[3][0] = pixels[3][1] = pixels[3][2] = pixels[3][3] = true;
                pixels[4][0] = pixels[4][4] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case '7':
                pixels[0][0] = pixels[0][1] = pixels[0][2] = pixels[0][3] = pixels[0][4] = true;
                pixels[1][4] = true;
                pixels[2][3] = true;
                pixels[3][3] = true;
                pixels[4][2] = true;
                pixels[5][2] = true;
                pixels[6][1] = true;
                break;
            case '8':
                pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][4] = true;
                pixels[3][1] = pixels[3][2] = pixels[3][3] = true;
                pixels[4][0] = pixels[4][4] = true;
                pixels[5][0] = pixels[5][4] = true;
                pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case '9':
                pixels[0][1] = pixels[0][2] = pixels[0][3] = true;
                pixels[1][0] = pixels[1][4] = true;
                pixels[2][0] = pixels[2][4] = true;
                pixels[3][1] = pixels[3][2] = pixels[3][3] = pixels[3][4] = true;
                pixels[4][4] = true;
                pixels[5][4] = true;
                pixels[6][1] = pixels[6][2] = pixels[6][3] = true;
                break;
            case '/':
                pixels[0][4] = true;
                pixels[1][4] = true;
                pixels[2][3] = true;
                pixels[3][2] = true;
                pixels[4][1] = true;
                pixels[5][0] = true;
                pixels[6][0] = true;
                break;
            case ':':
                pixels[2][2] = true;
                pixels[4][2] = true;
                break;
            default:
                break;
        }

        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 5; col++) {
                if (pixels[row][col]) {
                    SDL_Rect pixel = {x + col * size, y + row * size, size, size};
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }
    }

    void renderText(SDL_Renderer* renderer, const char* text, int x, int y, int size) {
        int currentX = x;
        for (int i = 0; text[i] != '\0'; i++) {
            renderChar(renderer, text[i], currentX, y, size);
            currentX += 6 * size;  // Character width + spacing
        }
    }

    void applyDifficulty(PlayState& state, Difficulty difficulty) {
        state.difficulty = difficulty;  // Store difficulty in PlayState

        switch (difficulty) {
            case Difficulty::EASY:
                state.initialZombieCount = 10;
                state.minZombieCount = 5;
                state.maxZombieCount = 15;
                state.zombieMaxHealth = 2;  // Easy: 2 hits to kill
                break;
            case Difficulty::NORMAL:
                state.initialZombieCount = 15;
                state.minZombieCount = 8;
                state.maxZombieCount = 25;
                state.zombieMaxHealth = 3;  // Normal: 3 hits to kill
                break;
            case Difficulty::HARD:
                state.initialZombieCount = 20;
                state.minZombieCount = 12;
                state.maxZombieCount = 35;
                state.zombieMaxHealth = 5;  // Hard: 5 hits to kill
                break;
            case Difficulty::TESTING:
                state.initialZombieCount = 2;
                state.minZombieCount = 0;
                state.maxZombieCount = 5;
                state.zombieMaxHealth = 1;  // Testing: 1 hit to kill
                // Weapon setup happens AFTER player is created (see initializeGame)
                break;
        }
    }

    void renderMainMenu(SDL_Renderer* renderer, const MenuState& menu) {
        // Title box
        SDL_Rect titleBox = {Game::SCREEN_WIDTH/2 - 200, 100, 400, 80};
        SDL_SetRenderDrawColor(renderer, 100, 100, 150, 255);
        SDL_RenderFillRect(renderer, &titleBox);
        SDL_SetRenderDrawColor(renderer, 200, 200, 255, 255);
        SDL_RenderDrawRect(renderer, &titleBox);

        // Title text
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        renderText(renderer, "ZOMBIE MAZE", Game::SCREEN_WIDTH/2 - 120, 120, 3);

        // Menu options
        const char* options[] = {"START GAME", "MAZE TYPE", "DIFFICULTY", "CONTROLS", "QUIT"};
        for (int i = 0; i < 5; i++) {
            int y = 200 + i * 62;
            SDL_Rect optionBox = {Game::SCREEN_WIDTH/2 - 150, y, 300, 55};

            if (i == menu.menuSelection) {
                // Selected option
                SDL_SetRenderDrawColor(renderer, 150, 200, 255, 255);
                SDL_RenderFillRect(renderer, &optionBox);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &optionBox);

                // Draw text in black for selected
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            } else {
                // Unselected option
                SDL_SetRenderDrawColor(renderer, 60, 60, 80, 255);
                SDL_RenderFillRect(renderer, &optionBox);
                SDL_SetRenderDrawColor(renderer, 120, 120, 150, 255);
                SDL_RenderDrawRect(renderer, &optionBox);

                // Draw text in light color for unselected
                SDL_SetRenderDrawColor(renderer, 180, 180, 200, 255);
            }

            // Calculate text position (centered)
            int textLen = 0;
            for (const char* c = options[i]; *c; c++) textLen++;
            int textX = Game::SCREEN_WIDTH/2 - (textLen * 6 * 2) / 2;
            renderText(renderer, options[i], textX, y + 18, 2);
        }

        // Controls hint at bottom
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        renderText(renderer, "W S ARROWS   ENTER", Game::SCREEN_WIDTH/2 - 108, 600, 2);
    }

    void renderDifficultySelect(SDL_Renderer* renderer, const MenuState& menu) {
        // Title
        SDL_Rect titleBox = {Game::SCREEN_WIDTH/2 - 200, 100, 400, 80};
        SDL_SetRenderDrawColor(renderer, 100, 150, 100, 255);
        SDL_RenderFillRect(renderer, &titleBox);
        SDL_SetRenderDrawColor(renderer, 200, 255, 200, 255);
        SDL_RenderDrawRect(renderer, &titleBox);

        // Title text
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        renderText(renderer, "SELECT DIFFICULTY", Game::SCREEN_WIDTH/2 - 144, 120, 3);

        // Difficulty options
        const char* options[] = {"EASY", "NORMAL", "HARD", "TESTING"};
        const char* descriptions[] = {
            "FEWER ZOMBIES",
            "BALANCED",
            "MANY ZOMBIES",
            "GOD MODE   ALL WEAPONS"
        };

        for (int i = 0; i < 4; i++) {
            int y = 230 + i * 85;
            SDL_Rect optionBox = {Game::SCREEN_WIDTH/2 - 200, y, 400, 70};

            if (i == menu.difficultySelection) {
                // Selected difficulty
                SDL_SetRenderDrawColor(renderer, 150, 255, 150, 255);
                SDL_RenderFillRect(renderer, &optionBox);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &optionBox);

                // Text color for selected
                SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
            } else {
                // Unselected difficulty
                SDL_SetRenderDrawColor(renderer, 60, 80, 60, 255);
                SDL_RenderFillRect(renderer, &optionBox);
                SDL_SetRenderDrawColor(renderer, 120, 150, 120, 255);
                SDL_RenderDrawRect(renderer, &optionBox);

                // Text color for unselected
                SDL_SetRenderDrawColor(renderer, 150, 180, 150, 255);
            }

            // Render option name
            int textLen = 0;
            for (const char* c = options[i]; *c; c++) textLen++;
            int textX = Game::SCREEN_WIDTH/2 - (textLen * 6 * 2) / 2;
            renderText(renderer, options[i], textX, y + 10, 2);

            // Render description
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            textLen = 0;
            for (const char* c = descriptions[i]; *c; c++) textLen++;
            textX = Game::SCREEN_WIDTH/2 - (textLen * 6 * 1) / 2;
            renderText(renderer, descriptions[i], textX, y + 48, 1);
        }

        // Back hint
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        renderText(renderer, "ESC TO GO BACK", Game::SCREEN_WIDTH/2 - 84, 610, 2);
    }

    void renderMazeTypeSelect(SDL_Renderer* renderer, const MenuState& menu) {
        // Title
        SDL_Rect titleBox = {Game::SCREEN_WIDTH/2 - 200, 100, 400, 80};
        SDL_SetRenderDrawColor(renderer, 100, 150, 100, 255);
        SDL_RenderFillRect(renderer, &titleBox);
        SDL_SetRenderDrawColor(renderer, 200, 255, 200, 255);
        SDL_RenderDrawRect(renderer, &titleBox);

        // Title text
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        renderText(renderer, "SELECT MAZE TYPE", Game::SCREEN_WIDTH/2 - 132, 120, 3);

        // Maze type options
        const char* options[] = {"STANDARD", "CIRCULAR", "INFINITE", "SOLDIER"};
        const char* descriptions[] = {
            "CLASSIC MAZE   EXIT AT EDGE",
            "CONCENTRIC RINGS   EXIT IN CENTER",
            "ENDLESS MODE   REGENERATE ON WIN",
            "WAVE ARENA   INFINITE AMMO AR"
        };

        for (int i = 0; i < 4; i++) {
            int y = 260 + i * 120;
            SDL_Rect optionBox = {Game::SCREEN_WIDTH/2 - 200, y, 400, 90};

            if (i == menu.mazeTypeSelection) {
                // Selected maze type
                SDL_SetRenderDrawColor(renderer, 150, 255, 150, 255);
                SDL_RenderFillRect(renderer, &optionBox);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &optionBox);

                // Text color for selected
                SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
            } else {
                // Unselected maze type
                SDL_SetRenderDrawColor(renderer, 60, 80, 60, 255);
                SDL_RenderFillRect(renderer, &optionBox);
                SDL_SetRenderDrawColor(renderer, 120, 150, 120, 255);
                SDL_RenderDrawRect(renderer, &optionBox);

                // Text color for unselected
                SDL_SetRenderDrawColor(renderer, 150, 180, 150, 255);
            }

            // Render option name
            int textLen = 0;
            for (const char* c = options[i]; *c; c++) textLen++;
            int textX = Game::SCREEN_WIDTH/2 - (textLen * 6 * 2) / 2;
            renderText(renderer, options[i], textX, y + 15, 2);

            // Render description
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            textLen = 0;
            for (const char* c = descriptions[i]; *c; c++) textLen++;
            textX = Game::SCREEN_WIDTH/2 - (textLen * 6 * 1) / 2;
            renderText(renderer, descriptions[i], textX, y + 55, 1);
        }

        // Back hint
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        renderText(renderer, "ESC TO GO BACK", Game::SCREEN_WIDTH/2 - 84, 580, 2);
    }

    void renderCodeEntry(SDL_Renderer* renderer, const MenuState& menu) {
        // Title
        SDL_Rect titleBox = {Game::SCREEN_WIDTH/2 - 250, 100, 500, 80};
        SDL_SetRenderDrawColor(renderer, 150, 100, 100, 255);
        SDL_RenderFillRect(renderer, &titleBox);
        SDL_SetRenderDrawColor(renderer, 255, 200, 200, 255);
        SDL_RenderDrawRect(renderer, &titleBox);

        // Title text
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        renderText(renderer, "ENTER DEV MODE CODE", Game::SCREEN_WIDTH/2 - 114, 120, 3);

        // Code entry box
        SDL_Rect codeBox = {Game::SCREEN_WIDTH/2 - 200, 250, 400, 80};
        if (menu.codeError) {
            SDL_SetRenderDrawColor(renderer, 150, 50, 50, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 60, 60, 80, 255);
        }
        SDL_RenderFillRect(renderer, &codeBox);
        SDL_SetRenderDrawColor(renderer, 150, 150, 200, 255);
        SDL_RenderDrawRect(renderer, &codeBox);

        // Entered code text
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        std::string displayCode = menu.codeEntry;
        // Add cursor
        if (SDL_GetTicks() % 1000 < 500) {
            displayCode += "_";
        }
        int textLen = displayCode.length();
        int textX = Game::SCREEN_WIDTH/2 - (textLen * 6 * 3) / 2;
        renderText(renderer, displayCode.c_str(), textX, 270, 3);

        // Error message
        if (menu.codeError) {
            SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
            renderText(renderer, "INCORRECT CODE", Game::SCREEN_WIDTH/2 - 84, 360, 2);
        }

        // Instructions
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        renderText(renderer, "TYPE CODE   ENTER TO SUBMIT", Game::SCREEN_WIDTH/2 - 162, 450, 2);
        renderText(renderer, "ESC TO GO BACK", Game::SCREEN_WIDTH/2 - 84, 500, 2);
    }

    void renderControlsScreen(SDL_Renderer* renderer) {
        // Title box
        SDL_Rect titleBox = {Game::SCREEN_WIDTH/2 - 250, 60, 500, 80};
        SDL_SetRenderDrawColor(renderer, 120, 100, 150, 255);
        SDL_RenderFillRect(renderer, &titleBox);
        SDL_SetRenderDrawColor(renderer, 220, 200, 255, 255);
        SDL_RenderDrawRect(renderer, &titleBox);

        // Title text
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        renderText(renderer, "CONTROLS", Game::SCREEN_WIDTH/2 - 64, 80, 3);

        // Controls info boxes
        struct ControlInfo {
            const char* keys;
            const char* action;
        };

        ControlInfo controls[] = {
            {"WASD   ARROWS", "MOVE PLAYER"},
            {"LEFT MOUSE", "SHOOT"},
            {"P   ESC", "PAUSE GAME"},
            {"R", "RESTART  AFTER DEATH WIN"}
        };

        int startY = 180;
        for (int i = 0; i < 4; i++) {
            int y = startY + i * 90;

            // Control box
            SDL_Rect controlBox = {Game::SCREEN_WIDTH/2 - 240, y, 480, 70};
            SDL_SetRenderDrawColor(renderer, 70, 60, 90, 255);
            SDL_RenderFillRect(renderer, &controlBox);
            SDL_SetRenderDrawColor(renderer, 140, 120, 180, 255);
            SDL_RenderDrawRect(renderer, &controlBox);

            // Keys (left side)
            SDL_SetRenderDrawColor(renderer, 200, 200, 255, 255);
            int keyLen = 0;
            for (const char* c = controls[i].keys; *c; c++) keyLen++;
            renderText(renderer, controls[i].keys, Game::SCREEN_WIDTH/2 - 220, y + 15, 2);

            // Separator
            SDL_SetRenderDrawColor(renderer, 140, 120, 180, 255);
            SDL_RenderDrawLine(renderer, Game::SCREEN_WIDTH/2, y + 10, Game::SCREEN_WIDTH/2, y + 60);

            // Action (right side)
            SDL_SetRenderDrawColor(renderer, 180, 255, 180, 255);
            renderText(renderer, controls[i].action, Game::SCREEN_WIDTH/2 + 20, y + 15, 2);

            // Description on second line for longer text
            if (i == 3) {
                SDL_SetRenderDrawColor(renderer, 150, 200, 150, 255);
                renderText(renderer, "AFTER DEATH WIN", Game::SCREEN_WIDTH/2 + 20, y + 40, 1);
            }
        }

        // Objective section
        SDL_Rect objBox = {Game::SCREEN_WIDTH/2 - 240, 560, 480, 100};
        SDL_SetRenderDrawColor(renderer, 90, 70, 60, 255);
        SDL_RenderFillRect(renderer, &objBox);
        SDL_SetRenderDrawColor(renderer, 180, 140, 120, 255);
        SDL_RenderDrawRect(renderer, &objBox);

        SDL_SetRenderDrawColor(renderer, 255, 220, 150, 255);
        renderText(renderer, "OBJECTIVE", Game::SCREEN_WIDTH/2 - 72, 570, 2);

        SDL_SetRenderDrawColor(renderer, 200, 180, 150, 255);
        renderText(renderer, "COLLECT ALL 5 KEYS", Game::SCREEN_WIDTH/2 - 108, 600, 1);
        renderText(renderer, "SURVIVE THE ZOMBIES", Game::SCREEN_WIDTH/2 - 114, 620, 1);
        renderText(renderer, "REACH THE EXIT DOOR", Game::SCREEN_WIDTH/2 - 114, 640, 1);

        // Back hint
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        renderText(renderer, "ESC TO GO BACK", Game::SCREEN_WIDTH/2 - 84, 690, 2);
    }

    void renderPauseMenu(SDL_Renderer* renderer, const MenuState& menu) {
        // Semi-transparent overlay
        SDL_Rect overlay = {0, 0, Game::SCREEN_WIDTH, Game::SCREEN_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_RenderFillRect(renderer, &overlay);

        // Pause title
        SDL_Rect titleBox = {Game::SCREEN_WIDTH/2 - 150, 120, 300, 60};
        SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
        SDL_RenderFillRect(renderer, &titleBox);
        SDL_SetRenderDrawColor(renderer, 200, 200, 255, 255);
        SDL_RenderDrawRect(renderer, &titleBox);

        // Title text
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        renderText(renderer, "PAUSED", Game::SCREEN_WIDTH/2 - 48, 135, 3);

        // Menu options
        const char* options[] = {"RESUME", "RESTART", "DIFFICULTY", "MAIN MENU", "QUIT"};
        for (int i = 0; i < 5; i++) {
            int y = 220 + i * 70;
            SDL_Rect optionBox = {Game::SCREEN_WIDTH/2 - 140, y, 280, 55};

            if (i == menu.pauseSelection) {
                // Selected option
                SDL_SetRenderDrawColor(renderer, 150, 200, 255, 255);
                SDL_RenderFillRect(renderer, &optionBox);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(renderer, &optionBox);

                // Text color for selected
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            } else {
                // Unselected option
                SDL_SetRenderDrawColor(renderer, 60, 60, 80, 255);
                SDL_RenderFillRect(renderer, &optionBox);
                SDL_SetRenderDrawColor(renderer, 120, 120, 150, 255);
                SDL_RenderDrawRect(renderer, &optionBox);

                // Text color for unselected
                SDL_SetRenderDrawColor(renderer, 180, 180, 200, 255);
            }

            // Render option text
            int textLen = 0;
            for (const char* c = options[i]; *c; c++) textLen++;
            int textX = Game::SCREEN_WIDTH/2 - (textLen * 6 * 2) / 2;
            renderText(renderer, options[i], textX, y + 16, 2);
        }

        // Controls hint
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        renderText(renderer, "W S ARROWS   ENTER", Game::SCREEN_WIDTH/2 - 108, 580, 2);
    }

    // Check if there's a clear line of sight between two points (no walls in between)
    bool hasLineOfSight(const Maze* maze, float x1, float y1, float x2, float y2) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float distance = std::sqrt(dx * dx + dy * dy);

        // Normalize direction
        dx /= distance;
        dy /= distance;

        // Step along the ray and check for walls
        float step = 5.0f;  // Check every 5 units
        for (float t = 0; t < distance; t += step) {
            float checkX = x1 + dx * t;
            float checkY = y1 + dy * t;

            int tileX = static_cast<int>(checkX / Maze::TILE_SIZE);
            int tileY = static_cast<int>(checkY / Maze::TILE_SIZE);

            if (maze->isWall(tileX, tileY)) {
                return false;  // Wall found, no line of sight
            }
        }

        return true;  // No walls found, clear line of sight
    }

    void renderFirstPersonView(SDL_Renderer* renderer, const PlayState& state) {
        // Guard against null player (game not initialized yet)
        if (!state.player) {
            // Render black screen if player not initialized
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            return;
        }

        const int SCREEN_WIDTH = Game::SCREEN_WIDTH;
        const int SCREEN_HEIGHT = Game::SCREEN_HEIGHT;
        const float FOV = 75.0f * M_PI / 180.0f;  // 75 degree field of view - wider for better visibility
        const int NUM_RAYS = SCREEN_WIDTH;

        float playerX = state.player->getX();
        float playerY = state.player->getY();
        float playerAngle = state.player->getAngle();
        float playerPitch = state.player->getPitch();

        // Apply screen shake to camera angle
        float shakeAngle = state.screenShake * ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        playerAngle += shakeAngle;

        // Calculate pitch offset for vertical look
        int pitchOffset = static_cast<int>(playerPitch * SCREEN_HEIGHT * 1.5f);

        // Render ceiling with dark, oppressive gradient (very dark)
        // Ceiling extends from top of screen to horizon (adjusted by pitch)
        int horizonLine = SCREEN_HEIGHT / 2 + pitchOffset;
        for (int y = 0; y < horizonLine && y < SCREEN_HEIGHT; y++) {
            float gradient = (float)y / (horizonLine > 0 ? horizonLine : 1);
            int r = 5 + (int)(5 * gradient);
            int g = 5 + (int)(5 * gradient);
            int b = 10 + (int)(5 * gradient);
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderDrawLine(renderer, 0, y, SCREEN_WIDTH, y);
        }

        // Render floor with very dark gradient (almost black)
        // Floor extends from horizon to bottom of screen
        for (int y = horizonLine; y < SCREEN_HEIGHT; y++) {
            if (y < 0) continue;
            float gradient = (float)(y - horizonLine) / ((SCREEN_HEIGHT - horizonLine) > 0 ? (SCREEN_HEIGHT - horizonLine) : 1);
            int baseColor = 3 + (int)(8 * gradient);
            SDL_SetRenderDrawColor(renderer, baseColor, baseColor, baseColor - 2, 255);
            SDL_RenderDrawLine(renderer, 0, y, SCREEN_WIDTH, y);
        }

        // Cast rays for each column of the screen
        for (int x = 0; x < NUM_RAYS; x++) {
            // Calculate ray angle
            float rayAngle = playerAngle - (FOV / 2.0f) + (static_cast<float>(x) / NUM_RAYS) * FOV;

            float rayDirX = std::cos(rayAngle);
            float rayDirY = std::sin(rayAngle);

            // DDA raycasting
            float rayX = playerX;
            float rayY = playerY;
            float deltaX = std::abs(1.0f / rayDirX);
            float deltaY = std::abs(1.0f / rayDirY);

            int mapX = static_cast<int>(rayX / Maze::TILE_SIZE);
            int mapY = static_cast<int>(rayY / Maze::TILE_SIZE);

            int stepX = (rayDirX > 0) ? 1 : -1;
            int stepY = (rayDirY > 0) ? 1 : -1;

            float sideDistX = (rayDirX > 0) ? ((mapX + 1) * Maze::TILE_SIZE - rayX) / Maze::TILE_SIZE * deltaX
                                             : (rayX - mapX * Maze::TILE_SIZE) / Maze::TILE_SIZE * deltaX;
            float sideDistY = (rayDirY > 0) ? ((mapY + 1) * Maze::TILE_SIZE - rayY) / Maze::TILE_SIZE * deltaY
                                             : (rayY - mapY * Maze::TILE_SIZE) / Maze::TILE_SIZE * deltaY;

            bool hit = false;
            int side = 0;  // 0 = vertical wall, 1 = horizontal wall
            float perpWallDist = 0.0f;
            bool isSafeRoomWall = false;  // Track if we hit a safe room wall

            // DDA algorithm
            while (!hit && perpWallDist < 2000.0f) {
                if (sideDistX < sideDistY) {
                    sideDistX += deltaX;
                    mapX += stepX;
                    side = 0;
                } else {
                    sideDistY += deltaY;
                    mapY += stepY;
                    side = 1;
                }

                if (state.maze->isWall(mapX, mapY)) {
                    hit = true;
                    // Check if this is a safe room wall (but locked safe rooms act as walls)
                    isSafeRoomWall = state.maze->isSafeRoom(mapX, mapY) && !state.safeRoomLocked;
                }
            }

            // Calculate perpendicular wall distance
            if (side == 0) {
                perpWallDist = (mapX * Maze::TILE_SIZE - rayX + (1 - stepX) * Maze::TILE_SIZE / 2) / rayDirX;
            } else {
                perpWallDist = (mapY * Maze::TILE_SIZE - rayY + (1 - stepY) * Maze::TILE_SIZE / 2) / rayDirY;
            }

            // Calculate wall height on screen
            int wallHeight = (int)(SCREEN_HEIGHT / (perpWallDist + 0.1f) * Maze::TILE_SIZE);

            // Apply pitch offset for vertical look
            int drawStart = SCREEN_HEIGHT / 2 - wallHeight / 2 + pitchOffset;
            int drawEnd = SCREEN_HEIGHT / 2 + wallHeight / 2 + pitchOffset;

            if (drawStart < 0) drawStart = 0;
            if (drawEnd >= SCREEN_HEIGHT) drawEnd = SCREEN_HEIGHT - 1;

            // Color based on wall type, side, and distance
            int baseColor, colorR, colorG, colorB;
            float distanceFade = std::max(0.0f, 1.0f - perpWallDist / 400.0f);  // Fog falloff
            distanceFade = distanceFade * distanceFade;  // Square it for exponential falloff

            if (isSafeRoomWall) {
                // BLUE SAFE ROOM WALLS - bright and glowing
                int blueBase = side == 0 ? 180 : 150;  // Brighter blue for safe room
                colorR = static_cast<int>(50 * distanceFade);   // Low red
                colorG = static_cast<int>(120 * distanceFade);  // Medium green
                colorB = static_cast<int>(blueBase * distanceFade);  // High blue

                // Add pulsing glow effect to safe room
                float pulseAmount = 0.5f + 0.5f * std::sin(SDL_GetTicks() * 0.003f);
                colorB = std::min(255, static_cast<int>(colorB * (1.0f + 0.3f * pulseAmount)));
            } else {
                // Normal walls - dark and creepy
                baseColor = side == 0 ? 35 : 25;  // Very dark gray/brown walls
                int color = static_cast<int>(baseColor * distanceFade);

                // Add subtle texture variation based on wall position (blood stains, decay)
                int colorVariation = ((mapX * 7 + mapY * 13) % 8) - 4;
                color = std::max(0, std::min(255, color + colorVariation));

                // Add eerie red tint to some walls (blood stained)
                int redTint = ((mapX * 11 + mapY * 17) % 20) > 15 ? 10 : 0;

                colorR = color + redTint;
                colorG = color * 0.8f;
                colorB = color * 0.8f;
            }

            // Calculate texture coordinate for brick pattern
            float wallX;
            if (side == 0) {
                wallX = rayY + perpWallDist * rayDirY;
            } else {
                wallX = rayX + perpWallDist * rayDirX;
            }
            wallX -= std::floor(wallX);

            // Draw main wall column
            SDL_SetRenderDrawColor(renderer, colorR, colorG, colorB, 255);
            SDL_RenderDrawLine(renderer, x, drawStart, x, drawEnd);

            int wallHeightPx = drawEnd - drawStart;

            if (isSafeRoomWall) {
                // Safe room gets glowing highlights instead of bricks
                // Add bright vertical highlights to make it glow
                if (((int)(wallX * 8) % 2) == 0) {  // Vertical glow lines
                    for (int y = drawStart; y < drawEnd; y += 2) {
                        SDL_SetRenderDrawColor(renderer,
                            std::min(255, colorR + 30),
                            std::min(255, colorG + 50),
                            std::min(255, colorB + 60),
                            255);
                        SDL_RenderDrawPoint(renderer, x, y);
                    }
                }
            } else {
                // Normal walls get brick pattern and blood
                // Draw optimized brick pattern (horizontal mortar lines) - darker mortar
                int brickRows = 6;
                for (int i = 1; i < brickRows; i++) {
                    int mortarY = drawStart + (wallHeightPx * i) / brickRows;
                    if (mortarY >= drawStart && mortarY < drawEnd) {
                        SDL_SetRenderDrawColor(renderer, colorR * 0.3f, colorG * 0.3f, colorB * 0.3f, 255);
                        SDL_RenderDrawPoint(renderer, x, mortarY);
                    }
                }

                // Draw vertical mortar lines based on texture coordinate - darker
                int brickCol = (int)(wallX * 4);  // 4 bricks horizontally
                if ((wallX * 4.0f - brickCol) < 0.1f) {  // Vertical mortar
                    for (int y = drawStart; y < drawEnd; y += 3) {  // Every 3rd pixel for performance
                        SDL_SetRenderDrawColor(renderer, colorR * 0.3f, colorG * 0.3f, colorB * 0.3f, 255);
                        SDL_RenderDrawPoint(renderer, x, y);
                    }
                }

                // Add creepy blood drips on some walls
                int redTint = ((mapX * 11 + mapY * 17) % 20) > 15 ? 10 : 0;
                if (redTint > 0 && wallHeightPx > 30) {
                    int dripY = drawStart + wallHeightPx / 3;
                    SDL_SetRenderDrawColor(renderer, 60, 10, 10, static_cast<int>(200 * distanceFade));
                    SDL_RenderDrawPoint(renderer, x, dripY);
                    SDL_RenderDrawPoint(renderer, x, dripY + 1);
                }
            }
        }

        // Render sprites (zombies, keys, weapons, bullets) in 3D
        // Collect all visible sprites with distance
        struct Sprite {
            float x, y;
            float distance;
            int type;  // 0=zombie, 1=key, 2=weapon, 3=bullet, 4=exit
            SDL_Color color;
            const Zombie* zombie;  // Pointer to zombie for health bar rendering
            int health;
            int maxHealth;
        };
        std::vector<Sprite> sprites;

        // Add zombies
        for (const auto& zombie : state.zombies) {
            if (!zombie->isDead()) {
                float dx = zombie->getX() - playerX;
                float dy = zombie->getY() - playerY;
                float distance = std::sqrt(dx * dx + dy * dy);

                // Check line-of-sight for all zombies (no wall-hacking)
                if (distance < 2000.0f && hasLineOfSight(state.maze.get(), playerX, playerY, zombie->getX(), zombie->getY())) {
                    sprites.push_back({zombie->getX(), zombie->getY(), distance, 0, {100, 255, 100, 255},
                                     zombie.get(), zombie->getHealth(), zombie->getMaxHealth()});  // Store zombie pointer and health
                }
            }
        }

        // Add hunters (scary dark entities with red eyes)
        for (const auto& hunter : state.hunters) {
            if (!hunter->isDead()) {
                float dx = hunter->getX() - playerX;
                float dy = hunter->getY() - playerY;
                float distance = std::sqrt(dx * dx + dy * dy);

                // Hunters need line-of-sight (can't see through walls)
                if (distance < 2000.0f && hasLineOfSight(state.maze.get(), playerX, playerY, hunter->getX(), hunter->getY())) {
                    sprites.push_back({hunter->getX(), hunter->getY(), distance, 0, {30, 30, 35, 255},  // Very dark gray/black
                                     hunter.get(), hunter->getHealth(), hunter->getMaxHealth()});
                }
            }
        }

        // Add bullets
        for (const auto& bullet : state.bullets) {
            if (bullet->isActive()) {
                float dx = bullet->getX() - playerX;
                float dy = bullet->getY() - playerY;
                float distance = std::sqrt(dx * dx + dy * dy);
                if (distance < 1000.0f) {
                    sprites.push_back({bullet->getX(), bullet->getY(), distance, 3, {255, 255, 100, 255}, nullptr, 0, 0});
                }
            }
        }

        // Add keys
        for (const auto& key : state.keys) {
            if (!key->isCollected()) {
                float dx = key->getX() - playerX;
                float dy = key->getY() - playerY;
                float distance = std::sqrt(dx * dx + dy * dy);
                if (distance < 1000.0f && hasLineOfSight(state.maze.get(), playerX, playerY, key->getX(), key->getY())) {
                    sprites.push_back({key->getX(), key->getY(), distance, 1, {255, 255, 0, 255}, nullptr, 0, 0});
                }
            }
        }

        // Add weapon pickups
        for (const auto& weapon : state.weaponPickups) {
            if (!weapon->isCollected()) {
                float dx = weapon->getX() - playerX;
                float dy = weapon->getY() - playerY;
                float distance = std::sqrt(dx * dx + dy * dy);
                if (distance < 1000.0f && hasLineOfSight(state.maze.get(), playerX, playerY, weapon->getX(), weapon->getY())) {
                    sprites.push_back({weapon->getX(), weapon->getY(), distance, 2,
                        weapon->getIsAmmo() ? SDL_Color{255, 200, 50, 255} : SDL_Color{100, 200, 255, 255}, nullptr, 0, 0});
                }
            }
        }

        // Add health boosts
        for (const auto& healthBoost : state.healthBoosts) {
            if (!healthBoost->isCollected()) {
                float dx = healthBoost->getX() - playerX;
                float dy = healthBoost->getY() - playerY;
                float distance = std::sqrt(dx * dx + dy * dy);
                if (distance < 1000.0f && hasLineOfSight(state.maze.get(), playerX, playerY, healthBoost->getX(), healthBoost->getY())) {
                    sprites.push_back({healthBoost->getX(), healthBoost->getY(), distance, 5, {255, 50, 50, 255}, nullptr, 0, 0});
                }
            }
        }

        // Add exit door
        Vec2 exitPos = state.maze->getExitPos();
        float exitDx = exitPos.x - playerX;
        float exitDy = exitPos.y - playerY;
        float exitDistance = std::sqrt(exitDx * exitDx + exitDy * exitDy);
        if (exitDistance < 2000.0f && hasLineOfSight(state.maze.get(), playerX, playerY, exitPos.x, exitPos.y)) {
            sprites.push_back({exitPos.x, exitPos.y, exitDistance, 4, {0, 255, 0, 255}, nullptr, 0, 0});
        }

        // Sort sprites by distance (back to front)
        std::sort(sprites.begin(), sprites.end(), [](const Sprite& a, const Sprite& b) {
            return a.distance > b.distance;
        });

        // Calculate elapsed time for highlighting (10+ mins for keys, 12+ mins for exit)
        Uint32 currentTime = SDL_GetTicks();
        float elapsedMinutes = (currentTime - state.gameStartTime) / 60000.0f;
        bool highlightKeys = elapsedMinutes >= 10.0f;
        bool highlightExit = elapsedMinutes >= 12.0f;

        // Render sprites
        for (const auto& sprite : sprites) {
            // Calculate sprite position relative to player
            float dx = sprite.x - playerX;
            float dy = sprite.y - playerY;

            // Transform to camera space
            float invDet = 1.0f / (std::cos(playerAngle + M_PI/2) * std::sin(playerAngle) -
                                   std::sin(playerAngle + M_PI/2) * std::cos(playerAngle));
            float transformX = invDet * (std::sin(playerAngle) * dx - std::cos(playerAngle) * dy);
            float transformY = invDet * (-std::sin(playerAngle + M_PI/2) * dx + std::cos(playerAngle + M_PI/2) * dy);

            // Skip if behind player
            if (transformY <= 0.1f) continue;

            // Calculate screen position
            int spriteScreenX = static_cast<int>((SCREEN_WIDTH / 2) * (1 + transformX / transformY / std::tan(FOV/2)));

            // Calculate sprite size (zombies are ENORMOUS, keys are 10x normal)
            float heightMultiplier, widthMultiplier;

            // Hunters (dark entities) are TALL and THIN like Endermen
            if (sprite.type == 0 && sprite.color.r < 50 && sprite.color.g < 50 && sprite.color.b < 50) {
                heightMultiplier = 35.0f;  // VERY tall like Enderman
                widthMultiplier = 8.0f;    // Thin/narrow
            } else if (sprite.type == 0) {
                heightMultiplier = 20.0f;  // Zombies normal size
                widthMultiplier = 20.0f;
            } else if (sprite.type == 1) {
                heightMultiplier = 5.0f;  // Keys
                widthMultiplier = 5.0f;
            } else if (sprite.type == 2) {
                heightMultiplier = 5.0f;  // Weapon pickups
                widthMultiplier = 5.0f;
            } else if (sprite.type == 4) {
                heightMultiplier = 5.0f;  // Exit door
                widthMultiplier = 5.0f;
            } else if (sprite.type == 5) {
                heightMultiplier = 5.0f;  // Health boosts
                widthMultiplier = 5.0f;
            } else {
                heightMultiplier = 0.5f;  // Other sprites
                widthMultiplier = 0.5f;
            }
            int spriteHeight = static_cast<int>(SCREEN_HEIGHT / transformY * heightMultiplier);
            int spriteWidth = static_cast<int>(SCREEN_HEIGHT / transformY * widthMultiplier);

            // Apply pitch offset for vertical look
            int drawStartY = SCREEN_HEIGHT / 2 - spriteHeight / 2 + pitchOffset;
            int drawEndY = SCREEN_HEIGHT / 2 + spriteHeight / 2 + pitchOffset;
            int drawStartX = spriteScreenX - spriteWidth / 2;
            int drawEndX = spriteScreenX + spriteWidth / 2;

            // Clamp to screen
            if (drawStartX < 0) drawStartX = 0;
            if (drawEndX >= SCREEN_WIDTH) drawEndX = SCREEN_WIDTH - 1;
            if (drawStartY < 0) drawStartY = 0;
            if (drawEndY >= SCREEN_HEIGHT) drawEndY = SCREEN_HEIGHT - 1;

            int width = drawEndX - drawStartX;
            int height = drawEndY - drawStartY;

            // === DISTANCE-BASED FOG ON SPRITES ===
            // Calculate fog intensity based on distance
            float spriteFogFactor = sprite.distance / 400.0f;  // 0 = close, 1+ = far
            spriteFogFactor = std::min(1.0f, spriteFogFactor);
            int fogOverlayAlpha = static_cast<int>(spriteFogFactor * spriteFogFactor * 180);  // Exponential

            // Check if this is a hunter (dark entity)
            bool isHunter = (sprite.type == 0 && sprite.color.r < 50 && sprite.color.g < 50 && sprite.color.b < 50);

            if (isHunter) {
                // === HUNTER - SHADOWY ENDERMAN ===
                // Pure dark shadowy silhouette - no body parts, just darkness with eyes

                // Main shadow body - very dark, slightly transparent for gloomy effect
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_Rect hunterBody = {drawStartX, drawStartY, width, height};
                SDL_SetRenderDrawColor(renderer, 15, 15, 20, 240);  // Very dark, slightly transparent
                SDL_RenderFillRect(renderer, &hunterBody);

                // Add darker gradient from top to bottom for depth
                for (int i = 0; i < height / 3; i++) {
                    int alpha = 240 - (i * 2);
                    SDL_Rect gradientSlice = {drawStartX, drawStartY + i, width, 1};
                    SDL_SetRenderDrawColor(renderer, 10, 10, 15, alpha);
                    SDL_RenderFillRect(renderer, &gradientSlice);
                }

                // Shadowy aura around the hunter
                SDL_Rect aura1 = {drawStartX - 4, drawStartY - 4, width + 8, height + 8};
                SDL_SetRenderDrawColor(renderer, 5, 5, 10, 60);
                SDL_RenderFillRect(renderer, &aura1);

                SDL_Rect aura2 = {drawStartX - 2, drawStartY - 2, width + 4, height + 4};
                SDL_SetRenderDrawColor(renderer, 8, 8, 12, 100);
                SDL_RenderFillRect(renderer, &aura2);

                // === GLOWING EYES - THE ONLY BRIGHT FEATURE ===
                int eyeSize = std::max(5, width / 3);
                int eyeY = drawStartY + height / 5;  // High on the head
                int eyeSpacing = width / 5;

                // Left eye - intense red/white glow (multiple layers)
                SDL_Rect leftEyeGlow1 = {drawStartX + width/2 - eyeSpacing - eyeSize - 5, eyeY - 5, eyeSize + 10, eyeSize + 10};
                SDL_SetRenderDrawColor(renderer, 255, 30, 30, 80);
                SDL_RenderFillRect(renderer, &leftEyeGlow1);

                SDL_Rect leftEyeGlow2 = {drawStartX + width/2 - eyeSpacing - eyeSize - 2, eyeY - 2, eyeSize + 4, eyeSize + 4};
                SDL_SetRenderDrawColor(renderer, 255, 60, 60, 160);
                SDL_RenderFillRect(renderer, &leftEyeGlow2);

                SDL_Rect leftEye = {drawStartX + width/2 - eyeSpacing - eyeSize/2, eyeY, eyeSize, eyeSize};
                SDL_SetRenderDrawColor(renderer, 255, 230, 230, 255);  // Bright white/red core
                SDL_RenderFillRect(renderer, &leftEye);

                // Right eye - intense red/white glow
                SDL_Rect rightEyeGlow1 = {drawStartX + width/2 + eyeSpacing - 5, eyeY - 5, eyeSize + 10, eyeSize + 10};
                SDL_SetRenderDrawColor(renderer, 255, 30, 30, 80);
                SDL_RenderFillRect(renderer, &rightEyeGlow1);

                SDL_Rect rightEyeGlow2 = {drawStartX + width/2 + eyeSpacing - 2, eyeY - 2, eyeSize + 4, eyeSize + 4};
                SDL_SetRenderDrawColor(renderer, 255, 60, 60, 160);
                SDL_RenderFillRect(renderer, &rightEyeGlow2);

                SDL_Rect rightEye = {drawStartX + width/2 + eyeSpacing - eyeSize/2, eyeY, eyeSize, eyeSize};
                SDL_SetRenderDrawColor(renderer, 255, 230, 230, 255);  // Bright white/red core
                SDL_RenderFillRect(renderer, &rightEye);

                // Floating red particles around hunter for supernatural effect
                Uint32 particleTime = SDL_GetTicks();
                for (int i = 0; i < 3; i++) {
                    float particlePhase = (particleTime / 500.0f) + i * 2.0f;
                    int particleX = drawStartX + width/2 + static_cast<int>(std::sin(particlePhase) * width);
                    int particleY = drawStartY + height/3 + static_cast<int>(std::cos(particlePhase * 1.3f) * height/2);
                    SDL_Rect particle = {particleX, particleY, 3, 3};
                    SDL_SetRenderDrawColor(renderer, 255, 50, 50, 180);
                    SDL_RenderFillRect(renderer, &particle);
                }

                // Reset blend mode
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            } else if (sprite.type == 0) {
                // === WALKING ANIMATION ===
                // Create bobbing/swaying motion based on time and zombie position
                Uint32 animTime = SDL_GetTicks();
                float animPhase = (animTime / 300.0f) + (sprite.x + sprite.y) / 100.0f;  // Each zombie slightly offset
                float bobAmount = std::sin(animPhase) * (height / 40.0f);  // Vertical bob
                float swayAmount = std::cos(animPhase * 0.5f) * (width / 30.0f);  // Horizontal sway

                // === DETERMINE ZOMBIE VIEW ANGLE ===
                // Calculate angle from player to zombie
                float angleToZombie = std::atan2(sprite.y - playerY, sprite.x - playerX);
                // Get zombie's facing direction
                float zombieFacing = sprite.zombie->getFacingAngle();
                // Calculate relative angle (what angle player sees zombie from, relative to zombie's facing)
                float relativeAngle = angleToZombie - zombieFacing;
                // Normalize to -PI to PI range
                while (relativeAngle > M_PI) relativeAngle -= 2.0f * M_PI;
                while (relativeAngle < -M_PI) relativeAngle += 2.0f * M_PI;

                // Determine view: 0=front, 1=left, 2=back, 3=right
                int zombieView = 0;
                float absAngle = std::abs(relativeAngle);
                if (absAngle < M_PI / 4.0f) {
                    zombieView = 0;  // Front view (facing player)
                } else if (absAngle > 3.0f * M_PI / 4.0f) {
                    zombieView = 2;  // Back view (facing away)
                } else if (relativeAngle > 0) {
                    zombieView = 1;  // Left side view
                } else {
                    zombieView = 3;  // Right side view
                }

                // === ZOMBIE HEAD - Humanoid with 3D shading ===
                int headSize = height / 4;
                int headX = drawStartX + width / 2 - headSize / 2 + static_cast<int>(swayAmount);
                int headY = drawStartY + static_cast<int>(bobAmount);

                // Head base (dark decayed color)
                SDL_Rect head = {headX, headY, headSize, headSize};
                SDL_SetRenderDrawColor(renderer, 45, 65, 45, 255);
                SDL_RenderFillRect(renderer, &head);

                // 3D shading - light from top-left
                SDL_Rect headHighlight = {headX, headY, headSize*2/3, headSize/2};
                SDL_SetRenderDrawColor(renderer, 60, 80, 60, 255);  // Lighter
                SDL_RenderFillRect(renderer, &headHighlight);

                SDL_Rect headShadow = {headX + headSize/2, headY + headSize/2, headSize/2, headSize/2};
                SDL_SetRenderDrawColor(renderer, 30, 50, 30, 255);  // Darker
                SDL_RenderFillRect(renderer, &headShadow);

                // === ZOMBIE FACE ===
                // EXPOSED BONE/SKULL patches (terrifying!)
                SDL_Rect bone1 = {headX + headSize/6, headY + headSize/8, headSize/4, headSize/5};
                SDL_Rect bone2 = {headX + 2*headSize/3, headY + headSize/3, headSize/5, headSize/4};
                SDL_SetRenderDrawColor(renderer, 200, 200, 190, 255);  // Bone white
                SDL_RenderFillRect(renderer, &bone1);
                SDL_RenderFillRect(renderer, &bone2);

                // === RENDER FEATURES BASED ON VIEW ===
                if (zombieView == 0) {
                    // FRONT VIEW - Show both eyes and mouth
                    int eyeY = headY + headSize/3;
                    int eyeSize = headSize/6;

                    // Left eye - glowing red
                    SDL_Rect leftEyeSocket = {headX + headSize/5, eyeY, eyeSize, eyeSize};
                    SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);  // Dark socket
                    SDL_RenderFillRect(renderer, &leftEyeSocket);

                    SDL_Rect leftEyeGlow = {headX + headSize/5 + eyeSize/4, eyeY + eyeSize/4, eyeSize/2, eyeSize/2};
                    SDL_SetRenderDrawColor(renderer, 255, 20, 20, 255);  // Bright red
                    SDL_RenderFillRect(renderer, &leftEyeGlow);

                    // Right eye - glowing red
                    SDL_Rect rightEyeSocket = {headX + 3*headSize/5, eyeY, eyeSize, eyeSize};
                    SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);  // Dark socket
                    SDL_RenderFillRect(renderer, &rightEyeSocket);

                    SDL_Rect rightEyeGlow = {headX + 3*headSize/5 + eyeSize/4, eyeY + eyeSize/4, eyeSize/2, eyeSize/2};
                    SDL_SetRenderDrawColor(renderer, 255, 20, 20, 255);  // Bright red
                    SDL_RenderFillRect(renderer, &rightEyeGlow);

                    // Mouth
                    SDL_Rect mouth = {headX + headSize/3, headY + 2*headSize/3, headSize/3, headSize/8};
                    SDL_SetRenderDrawColor(renderer, 15, 5, 5, 255);  // Dark open mouth
                    SDL_RenderFillRect(renderer, &mouth);

                } else if (zombieView == 1) {
                    // LEFT SIDE VIEW - Show one eye on left side
                    int eyeY = headY + headSize/3;
                    int eyeSize = headSize/6;

                    SDL_Rect sideEyeSocket = {headX + headSize/8, eyeY, eyeSize, eyeSize};
                    SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
                    SDL_RenderFillRect(renderer, &sideEyeSocket);

                    SDL_Rect sideEyeGlow = {headX + headSize/8 + eyeSize/4, eyeY + eyeSize/4, eyeSize/2, eyeSize/2};
                    SDL_SetRenderDrawColor(renderer, 255, 20, 20, 255);
                    SDL_RenderFillRect(renderer, &sideEyeGlow);

                    // Side mouth
                    SDL_Rect sideMouth = {headX + headSize/8, headY + 2*headSize/3, headSize/4, headSize/8};
                    SDL_SetRenderDrawColor(renderer, 15, 5, 5, 255);
                    SDL_RenderFillRect(renderer, &sideMouth);

                } else if (zombieView == 3) {
                    // RIGHT SIDE VIEW - Show one eye on right side
                    int eyeY = headY + headSize/3;
                    int eyeSize = headSize/6;

                    SDL_Rect sideEyeSocket = {headX + 5*headSize/8, eyeY, eyeSize, eyeSize};
                    SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
                    SDL_RenderFillRect(renderer, &sideEyeSocket);

                    SDL_Rect sideEyeGlow = {headX + 5*headSize/8 + eyeSize/4, eyeY + eyeSize/4, eyeSize/2, eyeSize/2};
                    SDL_SetRenderDrawColor(renderer, 255, 20, 20, 255);
                    SDL_RenderFillRect(renderer, &sideEyeGlow);

                    // Side mouth
                    SDL_Rect sideMouth = {headX + 5*headSize/8, headY + 2*headSize/3, headSize/4, headSize/8};
                    SDL_SetRenderDrawColor(renderer, 15, 5, 5, 255);
                    SDL_RenderFillRect(renderer, &sideMouth);

                } else {
                    // BACK VIEW (zombieView == 2) - No eyes or mouth visible, show back of head
                    // Add some detail to back of head (maybe hair/damage)
                    SDL_Rect backDetail1 = {headX + headSize/4, headY + headSize/4, headSize/2, headSize/6};
                    SDL_SetRenderDrawColor(renderer, 30, 45, 30, 255);  // Darker detail
                    SDL_RenderFillRect(renderer, &backDetail1);

                    SDL_Rect backDetail2 = {headX + headSize/3, headY + headSize/2, headSize/3, headSize/6};
                    SDL_SetRenderDrawColor(renderer, 35, 50, 35, 255);
                    SDL_RenderFillRect(renderer, &backDetail2);
                }

                // === ZOMBIE BODY - Humanoid torso with 3D shading ===
                int bodyWidth = width * 3 / 5;
                int bodyHeight = height / 2;
                int bodyX = drawStartX + width / 2 - bodyWidth / 2 + static_cast<int>(swayAmount * 0.7f);
                int bodyY = drawStartY + headSize + static_cast<int>(bobAmount);

                // Body base (dark decayed color)
                SDL_Rect body = {bodyX, bodyY, bodyWidth, bodyHeight};
                SDL_SetRenderDrawColor(renderer, 45, 65, 45, 255);
                SDL_RenderFillRect(renderer, &body);

                // 3D shading - light from top-left
                SDL_Rect bodyHighlight = {bodyX, bodyY, bodyWidth*2/3, bodyHeight/2};
                SDL_SetRenderDrawColor(renderer, 60, 80, 60, 255);  // Lighter
                SDL_RenderFillRect(renderer, &bodyHighlight);

                SDL_Rect bodyShadow = {bodyX + bodyWidth/3, bodyY + bodyHeight/2, bodyWidth*2/3, bodyHeight/2};
                SDL_SetRenderDrawColor(renderer, 30, 50, 30, 255);  // Darker
                SDL_RenderFillRect(renderer, &bodyShadow);

                // Blood stains on body
                SDL_Rect bloodStain1 = {bodyX + bodyWidth/4, bodyY + bodyHeight/4, bodyWidth/3, bodyHeight/3};
                SDL_Rect bloodStain2 = {bodyX + bodyWidth/6, bodyY + bodyHeight/2, bodyWidth/3, bodyHeight/4};
                SDL_SetRenderDrawColor(renderer, 100, 15, 15, 255);  // Dark blood
                SDL_RenderFillRect(renderer, &bloodStain1);
                SDL_RenderFillRect(renderer, &bloodStain2);

                // === ZOMBIE ARMS - Simple with 3D shading ===
                int armWidth = width / 6;
                int armHeight = bodyHeight * 3 / 4;
                float armSwing = std::sin(animPhase) * (width / 15.0f);  // Arm swing animation

                // Left arm
                int leftArmX = bodyX - armWidth + static_cast<int>(armSwing);
                SDL_Rect leftArm = {leftArmX, bodyY + bodyHeight/6, armWidth, armHeight};
                SDL_SetRenderDrawColor(renderer, 45, 65, 45, 255);
                SDL_RenderFillRect(renderer, &leftArm);
                // Left arm highlight (lighter on left side)
                SDL_Rect leftArmHighlight = {leftArmX, bodyY + bodyHeight/6, armWidth/2, armHeight/2};
                SDL_SetRenderDrawColor(renderer, 60, 80, 60, 255);
                SDL_RenderFillRect(renderer, &leftArmHighlight);

                // Right arm
                int rightArmX = bodyX + bodyWidth - static_cast<int>(armSwing);
                SDL_Rect rightArm = {rightArmX, bodyY + bodyHeight/6, armWidth, armHeight};
                SDL_SetRenderDrawColor(renderer, 45, 65, 45, 255);
                SDL_RenderFillRect(renderer, &rightArm);
                // Right arm shadow (darker on right side)
                SDL_Rect rightArmShadow = {rightArmX + armWidth/2, bodyY + bodyHeight/6 + armHeight/2, armWidth/2, armHeight/2};
                SDL_SetRenderDrawColor(renderer, 30, 50, 30, 255);
                SDL_RenderFillRect(renderer, &rightArmShadow);

                // === ZOMBIE LEGS - Simple with 3D shading ===
                int legWidth = bodyWidth / 3;
                int legHeight = height / 3;
                int legY = bodyY + bodyHeight;
                float legOffset = std::sin(animPhase) * (width / 20.0f);  // Leg alternation

                // Left leg
                int leftLegX = bodyX + bodyWidth/6 + static_cast<int>(legOffset);
                SDL_Rect leftLeg = {leftLegX, legY, legWidth, legHeight};
                SDL_SetRenderDrawColor(renderer, 40, 60, 40, 255);
                SDL_RenderFillRect(renderer, &leftLeg);
                SDL_Rect leftLegHighlight = {leftLegX, legY, legWidth/2, legHeight/2};
                SDL_SetRenderDrawColor(renderer, 55, 75, 55, 255);
                SDL_RenderFillRect(renderer, &leftLegHighlight);

                // Right leg
                int rightLegX = bodyX + bodyWidth/2 - static_cast<int>(legOffset);
                SDL_Rect rightLeg = {rightLegX, legY, legWidth, legHeight};
                SDL_SetRenderDrawColor(renderer, 40, 60, 40, 255);
                SDL_RenderFillRect(renderer, &rightLeg);
                SDL_Rect rightLegShadow = {rightLegX + legWidth/2, legY + legHeight/2, legWidth/2, legHeight/2};
                SDL_SetRenderDrawColor(renderer, 25, 45, 25, 255);
                SDL_RenderFillRect(renderer, &rightLegShadow);

                // === 3D DEPTH: LARGE SHADOW UNDER ZOMBIE ===
                int shadowWidth = width + 10;
                int shadowHeight = height / 8;
                SDL_Rect zombieShadow = {drawStartX - 5, drawEndY - shadowHeight, shadowWidth, shadowHeight};
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 140);  // Dark shadow
                SDL_RenderFillRect(renderer, &zombieShadow);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                // Zombies now rendered as humanoid with 3D perspective shading!

                // === APPLY DISTANCE FOG TO ZOMBIE ===
                if (fogOverlayAlpha > 0) {
                    SDL_Rect zombieFog = {drawStartX, drawStartY, width, height};
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 30, 30, 35, fogOverlayAlpha);
                    SDL_RenderFillRect(renderer, &zombieFog);
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                }
            } else if (sprite.type == 1) {
                // Key - draw as big floating gold key
                int keyHeadSize = width / 2;
                int keyShaftWidth = width / 5;
                int keyShaftHeight = height / 2;

                int centerX = drawStartX + width / 2;
                int centerY = drawStartY + height / 3;

                // Add intense pulsing glow effect when highlighted (>10 min elapsed)
                if (highlightKeys) {
                    float pulseAmount = (std::sin(currentTime / 150.0f) + 1.0f) / 2.0f;  // Fast pulse 0-1
                    int glowExpansion = static_cast<int>(pulseAmount * 30);

                    // Outer glow layer (bright yellow/white)
                    SDL_Rect outerGlow = {drawStartX - 20 - glowExpansion, drawStartY - 20 - glowExpansion,
                                          width + 40 + glowExpansion * 2, height + 40 + glowExpansion * 2};
                    SDL_SetRenderDrawColor(renderer, 255, 255, 100, static_cast<int>(60 + pulseAmount * 100));
                    SDL_RenderFillRect(renderer, &outerGlow);

                    // Middle glow layer
                    SDL_Rect middleGlow = {drawStartX - 10 - glowExpansion/2, drawStartY - 10 - glowExpansion/2,
                                           width + 20 + glowExpansion, height + 20 + glowExpansion};
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, static_cast<int>(100 + pulseAmount * 120));
                    SDL_RenderFillRect(renderer, &middleGlow);

                    // Inner bright glow
                    SDL_Rect innerGlow = {drawStartX - 5, drawStartY - 5, width + 10, height + 10};
                    SDL_SetRenderDrawColor(renderer, 255, 255, 200, static_cast<int>(140 + pulseAmount * 115));
                    SDL_RenderFillRect(renderer, &innerGlow);
                }

                // Key head (circular top)
                SDL_Rect keyHead = {centerX - keyHeadSize/2, centerY - keyHeadSize/2, keyHeadSize, keyHeadSize};
                SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);  // Gold
                SDL_RenderFillRect(renderer, &keyHead);

                // Key head hole
                int holeSize = keyHeadSize / 3;
                SDL_Rect keyHole = {centerX - holeSize/2, centerY - holeSize/2, holeSize, holeSize};
                SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
                SDL_RenderFillRect(renderer, &keyHole);

                // Key shaft (vertical part)
                SDL_Rect keyShaft = {centerX - keyShaftWidth/2, centerY + keyHeadSize/2, keyShaftWidth, keyShaftHeight};
                SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);  // Gold
                SDL_RenderFillRect(renderer, &keyShaft);

                // Key teeth (notches at bottom)
                int toothWidth = keyShaftWidth * 2;
                int toothHeight = height / 8;
                SDL_Rect tooth1 = {centerX + keyShaftWidth/2, centerY + keyHeadSize/2 + keyShaftHeight/3, toothWidth, toothHeight};
                SDL_Rect tooth2 = {centerX + keyShaftWidth/2, centerY + keyHeadSize/2 + 2*keyShaftHeight/3, toothWidth, toothHeight};
                SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
                SDL_RenderFillRect(renderer, &tooth1);
                SDL_RenderFillRect(renderer, &tooth2);

                // Gold highlights (shinier gold on edges)
                SDL_Rect highlight1 = {centerX - keyHeadSize/4, centerY - keyHeadSize/4, keyHeadSize/3, keyHeadSize/3};
                SDL_SetRenderDrawColor(renderer, 255, 245, 150, 255);
                SDL_RenderFillRect(renderer, &highlight1);

                // Dark outline
                SDL_SetRenderDrawColor(renderer, 180, 150, 0, 255);
                SDL_RenderDrawRect(renderer, &keyHead);
                SDL_RenderDrawRect(renderer, &keyShaft);

                // Normal glow effect around key (always visible)
                SDL_Rect keyGlow = {drawStartX - 4, drawStartY - 4, width + 8, height + 8};
                SDL_SetRenderDrawColor(renderer, 255, 255, 150, 100);
                SDL_RenderDrawRect(renderer, &keyGlow);
            } else if (sprite.type == 3) {
                // Bullet - draw as bright projectile
                int bulletSize = std::max(4, height / 4);
                int bulletX = drawStartX + width / 2 - bulletSize / 2;
                int bulletY = drawStartY + height / 2 - bulletSize / 2;

                // Bright yellow bullet with glow
                SDL_Rect bulletCore = {bulletX, bulletY, bulletSize, bulletSize};
                SDL_SetRenderDrawColor(renderer, 255, 255, 150, 255);
                SDL_RenderFillRect(renderer, &bulletCore);

                // Glow effect
                SDL_Rect bulletGlow = {bulletX - 2, bulletY - 2, bulletSize + 4, bulletSize + 4};
                SDL_SetRenderDrawColor(renderer, 255, 200, 50, 180);
                SDL_RenderDrawRect(renderer, &bulletGlow);
            } else if (sprite.type == 4) {
                // Exit door - draw as large glowing green door
                int doorWidth = width;
                int doorHeight = height;
                int doorX = drawStartX;
                int doorY = drawStartY;

                // Add INTENSE pulsing glow effect when highlighted (>12 min elapsed)
                if (highlightExit) {
                    float pulseAmount = (std::sin(currentTime / 120.0f) + 1.0f) / 2.0f;  // Slower pulse 0-1
                    int glowExpansion = static_cast<int>(pulseAmount * 40);

                    // Outer glow layer (bright green/white beacon)
                    SDL_Rect outerGlow = {doorX - 30 - glowExpansion, doorY - 30 - glowExpansion,
                                          doorWidth + 60 + glowExpansion * 2, doorHeight + 60 + glowExpansion * 2};
                    SDL_SetRenderDrawColor(renderer, 100, 255, 100, static_cast<int>(70 + pulseAmount * 120));
                    SDL_RenderFillRect(renderer, &outerGlow);

                    // Middle glow layer (intense green)
                    SDL_Rect middleGlow = {doorX - 15 - glowExpansion/2, doorY - 15 - glowExpansion/2,
                                           doorWidth + 30 + glowExpansion, doorHeight + 30 + glowExpansion};
                    SDL_SetRenderDrawColor(renderer, 50, 255, 50, static_cast<int>(120 + pulseAmount * 135));
                    SDL_RenderFillRect(renderer, &middleGlow);

                    // Inner bright glow (nearly white)
                    SDL_Rect innerGlow = {doorX - 8, doorY - 8, doorWidth + 16, doorHeight + 16};
                    SDL_SetRenderDrawColor(renderer, 200, 255, 200, static_cast<int>(150 + pulseAmount * 105));
                    SDL_RenderFillRect(renderer, &innerGlow);
                }

                // Door frame (dark green)
                SDL_Rect doorFrame = {doorX, doorY, doorWidth, doorHeight};
                SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
                SDL_RenderFillRect(renderer, &doorFrame);

                // Door panels (brighter green)
                int panelWidth = doorWidth / 2 - doorWidth / 10;
                int panelHeight = doorHeight - doorHeight / 5;
                SDL_Rect leftPanel = {doorX + doorWidth / 20, doorY + doorHeight / 10, panelWidth, panelHeight};
                SDL_Rect rightPanel = {doorX + doorWidth / 2 + doorWidth / 20, doorY + doorHeight / 10, panelWidth, panelHeight};
                SDL_SetRenderDrawColor(renderer, 50, 200, 50, 255);
                SDL_RenderFillRect(renderer, &leftPanel);
                SDL_RenderFillRect(renderer, &rightPanel);

                // Door handles (gold)
                int handleSize = doorWidth / 15;
                SDL_Rect leftHandle = {doorX + doorWidth / 2 - handleSize - doorWidth / 10, doorY + doorHeight / 2 - handleSize, handleSize * 2, handleSize * 2};
                SDL_Rect rightHandle = {doorX + doorWidth / 2 + doorWidth / 10 - handleSize, doorY + doorHeight / 2 - handleSize, handleSize * 2, handleSize * 2};
                SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
                SDL_RenderFillRect(renderer, &leftHandle);
                SDL_RenderFillRect(renderer, &rightHandle);

                // Glowing exit sign on top
                int signWidth = doorWidth / 2;
                int signHeight = doorHeight / 8;
                SDL_Rect exitSign = {doorX + doorWidth / 4, doorY + doorHeight / 20, signWidth, signHeight};
                SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
                SDL_RenderFillRect(renderer, &exitSign);

                // Bright glow effect
                SDL_Rect doorGlow1 = {doorX - 4, doorY - 4, doorWidth + 8, doorHeight + 8};
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 150);
                SDL_RenderDrawRect(renderer, &doorGlow1);
                SDL_Rect doorGlow2 = {doorX - 8, doorY - 8, doorWidth + 16, doorHeight + 16};
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 80);
                SDL_RenderDrawRect(renderer, &doorGlow2);

                // Dark outline for definition
                SDL_SetRenderDrawColor(renderer, 0, 80, 0, 255);
                SDL_RenderDrawRect(renderer, &doorFrame);
            } else if (sprite.type == 5) {
                // Health boost - draw as first aid kit with red cross
                int boxWidth = width;
                int boxHeight = height;
                int boxX = drawStartX;
                int boxY = drawStartY;

                // White box background
                SDL_Rect box = {boxX, boxY, boxWidth, boxHeight};
                SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
                SDL_RenderFillRect(renderer, &box);

                // Red cross - horizontal bar
                int crossThickness = boxHeight / 5;
                int crossLength = boxWidth * 3 / 4;
                SDL_Rect horizBar = {boxX + (boxWidth - crossLength) / 2, boxY + boxHeight / 2 - crossThickness / 2, crossLength, crossThickness};
                SDL_SetRenderDrawColor(renderer, 220, 20, 20, 255);
                SDL_RenderFillRect(renderer, &horizBar);

                // Red cross - vertical bar
                SDL_Rect vertBar = {boxX + boxWidth / 2 - crossThickness / 2, boxY + (boxHeight - crossLength) / 2, crossThickness, crossLength};
                SDL_SetRenderDrawColor(renderer, 220, 20, 20, 255);
                SDL_RenderFillRect(renderer, &vertBar);

                // Box border (dark red)
                SDL_SetRenderDrawColor(renderer, 150, 20, 20, 255);
                SDL_RenderDrawRect(renderer, &box);

                // Add subtle glow effect
                SDL_Rect glow = {boxX - 2, boxY - 2, boxWidth + 4, boxHeight + 4};
                SDL_SetRenderDrawColor(renderer, 255, 100, 100, 120);
                SDL_RenderDrawRect(renderer, &glow);
            } else {
                // Check if this is a hunter (tall, thin, dark Enderman-like entity)
                bool isHunter = (sprite.type == 0 && sprite.color.r < 50 && sprite.color.g < 50 && sprite.color.b < 50);

                if (isHunter) {
                    // === HUNTER - ENDERMAN STYLE ===
                    // Very dark, tall, thin body with subtle shading
                    SDL_Rect hunterBody = {drawStartX, drawStartY, width, height};
                    SDL_SetRenderDrawColor(renderer, 25, 25, 30, 255);  // Very dark gray/black
                    SDL_RenderFillRect(renderer, &hunterBody);

                    // Darker outline for depth
                    SDL_SetRenderDrawColor(renderer, 10, 10, 15, 255);  // Nearly black outline
                    SDL_RenderDrawRect(renderer, &hunterBody);

                    // Subtle vertical highlights on edges (makes it look 3D)
                    SDL_Rect leftEdge = {drawStartX + 1, drawStartY, 1, height};
                    SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255);
                    SDL_RenderFillRect(renderer, &leftEdge);

                    SDL_Rect rightEdge = {drawStartX + width - 2, drawStartY, 1, height};
                    SDL_SetRenderDrawColor(renderer, 15, 15, 20, 255);
                    SDL_RenderFillRect(renderer, &rightEdge);

                    // === TERRIFYING GLOWING RED/WHITE EYES ===
                    // Eyes positioned high on the body (like Enderman)
                    int eyeSize = std::max(4, width / 3);  // Bigger eyes for thin face
                    int eyeY = drawStartY + height / 6;  // High up on the head
                    int eyeSpacing = width / 4;

                    // Left eye - intense red glow
                    SDL_Rect leftEyeGlow = {drawStartX + eyeSpacing - eyeSize - 3, eyeY - 3, eyeSize + 6, eyeSize + 6};
                    SDL_SetRenderDrawColor(renderer, 255, 40, 40, 140);  // Strong red glow
                    SDL_RenderFillRect(renderer, &leftEyeGlow);

                    SDL_Rect leftEyeInner = {drawStartX + eyeSpacing - eyeSize - 1, eyeY - 1, eyeSize + 2, eyeSize + 2};
                    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 200);  // Medium glow
                    SDL_RenderFillRect(renderer, &leftEyeInner);

                    SDL_Rect leftEye = {drawStartX + eyeSpacing - eyeSize/2, eyeY, eyeSize, eyeSize};
                    SDL_SetRenderDrawColor(renderer, 255, 220, 220, 255);  // Bright white/red core
                    SDL_RenderFillRect(renderer, &leftEye);

                    // Right eye - intense red glow
                    SDL_Rect rightEyeGlow = {drawStartX + width - eyeSpacing - 3, eyeY - 3, eyeSize + 6, eyeSize + 6};
                    SDL_SetRenderDrawColor(renderer, 255, 40, 40, 140);  // Strong red glow
                    SDL_RenderFillRect(renderer, &rightEyeGlow);

                    SDL_Rect rightEyeInner = {drawStartX + width - eyeSpacing - 1, eyeY - 1, eyeSize + 2, eyeSize + 2};
                    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 200);  // Medium glow
                    SDL_RenderFillRect(renderer, &rightEyeInner);

                    SDL_Rect rightEye = {drawStartX + width - eyeSpacing + eyeSize/2, eyeY, eyeSize, eyeSize};
                    SDL_SetRenderDrawColor(renderer, 255, 220, 220, 255);  // Bright white/red core
                    SDL_RenderFillRect(renderer, &rightEye);

                    // Particle effect - small red dots floating around hunter
                    if ((rand() % 3) == 0) {  // Random particles
                        int particleX = drawStartX + (rand() % width);
                        int particleY = drawStartY + (rand() % height);
                        SDL_Rect particle = {particleX, particleY, 2, 2};
                        SDL_SetRenderDrawColor(renderer, 255, 50, 50, 150);
                        SDL_RenderFillRect(renderer, &particle);
                    }
                } else {
                    // Weapons and other sprites - draw as colored rectangle
                    SDL_Rect spriteRect = {drawStartX, drawStartY, width, height};
                    SDL_SetRenderDrawColor(renderer, sprite.color.r, sprite.color.g, sprite.color.b, sprite.color.a);
                    SDL_RenderFillRect(renderer, &spriteRect);

                    // Draw darker outline
                    SDL_SetRenderDrawColor(renderer, sprite.color.r/2, sprite.color.g/2, sprite.color.b/2, 255);
                    SDL_RenderDrawRect(renderer, &spriteRect);
                }
            }

            // Draw health bar above zombies
            if (sprite.type == 0 && sprite.zombie != nullptr && sprite.maxHealth > 0) {
                int barWidth = width;
                int barHeight = 8;
                int barX = drawStartX;
                int barY = drawStartY - barHeight - 4;  // Position above zombie

                // Background (dark red)
                SDL_Rect barBg = {barX, barY, barWidth, barHeight};
                SDL_SetRenderDrawColor(renderer, 60, 0, 0, 200);
                SDL_RenderFillRect(renderer, &barBg);

                // Health fill (red to green gradient based on health)
                float healthPercent = static_cast<float>(sprite.health) / static_cast<float>(sprite.maxHealth);
                int fillWidth = static_cast<int>(barWidth * healthPercent);
                if (fillWidth > 0) {
                    SDL_Rect barFill = {barX, barY, fillWidth, barHeight};
                    // Color gradient: red (low health) to yellow (mid health) to green (full health)
                    int r = healthPercent < 0.5f ? 255 : static_cast<int>(255 * (1.0f - (healthPercent - 0.5f) * 2.0f));
                    int g = healthPercent < 0.5f ? static_cast<int>(255 * healthPercent * 2.0f) : 255;
                    SDL_SetRenderDrawColor(renderer, r, g, 0, 220);
                    SDL_RenderFillRect(renderer, &barFill);
                }

                // Border (white)
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
                SDL_RenderDrawRect(renderer, &barBg);
            }
        }

        // Render weapon in hands (Doom-style)
        WeaponType currentWeapon = state.player->getCurrentWeapon();
        int weaponWidth = 200;
        int weaponHeight = 250;
        int weaponX = (SCREEN_WIDTH - weaponWidth) / 2 + static_cast<int>(state.shakeOffsetX);
        int weaponY = SCREEN_HEIGHT - weaponHeight + 50 + static_cast<int>(state.shakeOffsetY);

        // Weapon recoil effect
        if (state.screenShake > 0.1f) {
            weaponY += static_cast<int>(state.screenShake * 30.0f);  // Kick weapon up when shooting
        }

        // Draw weapon based on type
        if (currentWeapon == WeaponType::SHOTGUN) {
            // Shotgun (M870 style pump-action)
            // Stock
            SDL_Rect stock = {weaponX + 20, weaponY + 140, 70, 50};
            SDL_SetRenderDrawColor(renderer, 70, 50, 30, 255);
            SDL_RenderFillRect(renderer, &stock);

            // Receiver/body
            SDL_Rect body = {weaponX + 80, weaponY + 125, 90, 50};
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &body);

            // Long barrel
            SDL_Rect barrel = {weaponX + 160, weaponY + 130, 40, 30};
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            SDL_RenderFillRect(renderer, &barrel);

            // Barrel end
            SDL_Rect barrelEnd = {weaponX + 190, weaponY + 135, 10, 20};
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_RenderFillRect(renderer, &barrelEnd);

            // Pump/foregrip (wood)
            SDL_Rect pump = {weaponX + 110, weaponY + 170, 50, 35};
            SDL_SetRenderDrawColor(renderer, 80, 60, 40, 255);
            SDL_RenderFillRect(renderer, &pump);

            // Handle
            SDL_Rect handle = {weaponX + 90, weaponY + 165, 30, 45};
            SDL_SetRenderDrawColor(renderer, 60, 40, 20, 255);
            SDL_RenderFillRect(renderer, &handle);

            // Trigger guard
            SDL_Rect trigger = {weaponX + 100, weaponY + 155, 20, 15};
            SDL_SetRenderDrawColor(renderer, 45, 45, 45, 255);
            SDL_RenderDrawRect(renderer, &trigger);

            // Metal highlights
            SDL_Rect highlight1 = {weaponX + 165, weaponY + 135, 20, 10};
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &highlight1);
        } else if (currentWeapon == WeaponType::PISTOL) {
            // Pistol
            // Handle
            SDL_Rect handle = {weaponX + 85, weaponY + 150, 30, 70};
            SDL_SetRenderDrawColor(renderer, 60, 40, 20, 255);
            SDL_RenderFillRect(renderer, &handle);

            // Barrel
            SDL_Rect barrel = {weaponX + 60, weaponY + 100, 80, 50};
            SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
            SDL_RenderFillRect(renderer, &barrel);

            // Barrel highlight
            SDL_Rect barrelHighlight = {weaponX + 70, weaponY + 110, 50, 20};
            SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);
            SDL_RenderFillRect(renderer, &barrelHighlight);

            // Trigger guard
            SDL_Rect trigger = {weaponX + 90, weaponY + 140, 20, 15};
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderDrawRect(renderer, &trigger);
        } else if (currentWeapon == WeaponType::ASSAULT_RIFLE) {
            // Assault Rifle
            // Stock
            SDL_Rect stock = {weaponX + 20, weaponY + 130, 60, 40};
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &stock);

            // Body
            SDL_Rect body = {weaponX + 70, weaponY + 110, 100, 60};
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            SDL_RenderFillRect(renderer, &body);

            // Barrel
            SDL_Rect barrel = {weaponX + 150, weaponY + 120, 50, 30};
            SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
            SDL_RenderFillRect(renderer, &barrel);

            // Magazine
            SDL_Rect mag = {weaponX + 110, weaponY + 170, 30, 50};
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_RenderFillRect(renderer, &mag);

            // Handle
            SDL_Rect handle = {weaponX + 90, weaponY + 150, 25, 40};
            SDL_SetRenderDrawColor(renderer, 60, 40, 20, 255);
            SDL_RenderFillRect(renderer, &handle);
        } else if (currentWeapon == WeaponType::GRENADE_LAUNCHER) {
            // Grenade Launcher
            // Body
            SDL_Rect body = {weaponX + 50, weaponY + 120, 120, 60};
            SDL_SetRenderDrawColor(renderer, 70, 70, 50, 255);
            SDL_RenderFillRect(renderer, &body);

            // Large barrel tube
            SDL_Rect barrel = {weaponX + 140, weaponY + 100, 60, 100};
            SDL_SetRenderDrawColor(renderer, 80, 80, 60, 255);
            SDL_RenderFillRect(renderer, &barrel);

            // Barrel end (dark)
            SDL_Rect barrelEnd = {weaponX + 180, weaponY + 110, 20, 80};
            SDL_SetRenderDrawColor(renderer, 40, 40, 30, 255);
            SDL_RenderFillRect(renderer, &barrelEnd);

            // Handle
            SDL_Rect handle = {weaponX + 80, weaponY + 160, 30, 50};
            SDL_SetRenderDrawColor(renderer, 60, 40, 20, 255);
            SDL_RenderFillRect(renderer, &handle);
        } else if (currentWeapon == WeaponType::SMG) {
            // SMG (Compact submachine gun)
            // Stock (small and compact)
            SDL_Rect stock = {weaponX + 30, weaponY + 135, 50, 35};
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_RenderFillRect(renderer, &stock);

            // Body (compact)
            SDL_Rect body = {weaponX + 70, weaponY + 115, 80, 55};
            SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
            SDL_RenderFillRect(renderer, &body);

            // Short barrel
            SDL_Rect barrel = {weaponX + 135, weaponY + 125, 45, 30};
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            SDL_RenderFillRect(renderer, &barrel);

            // Large magazine (extended)
            SDL_Rect mag = {weaponX + 100, weaponY + 170, 35, 60};
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
            SDL_RenderFillRect(renderer, &mag);

            // Handle
            SDL_Rect handle = {weaponX + 90, weaponY + 155, 25, 40};
            SDL_SetRenderDrawColor(renderer, 50, 40, 30, 255);
            SDL_RenderFillRect(renderer, &handle);

            // Highlight
            SDL_Rect highlight = {weaponX + 140, weaponY + 130, 30, 10};
            SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
            SDL_RenderFillRect(renderer, &highlight);
        } else if (currentWeapon == WeaponType::SNIPER) {
            // Sniper Rifle (Long and precise)
            // Stock
            SDL_Rect stock = {weaponX + 10, weaponY + 130, 70, 45};
            SDL_SetRenderDrawColor(renderer, 60, 45, 30, 255);
            SDL_RenderFillRect(renderer, &stock);

            // Body/receiver
            SDL_Rect body = {weaponX + 70, weaponY + 115, 90, 55};
            SDL_SetRenderDrawColor(renderer, 55, 55, 55, 255);
            SDL_RenderFillRect(renderer, &body);

            // Very long barrel
            SDL_Rect barrel = {weaponX + 145, weaponY + 125, 80, 28};
            SDL_SetRenderDrawColor(renderer, 65, 65, 65, 255);
            SDL_RenderFillRect(renderer, &barrel);

            // Scope (large)
            SDL_Rect scope = {weaponX + 90, weaponY + 85, 60, 30};
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_RenderFillRect(renderer, &scope);

            // Scope lens
            SDL_Rect scopeLens = {weaponX + 140, weaponY + 92, 10, 16};
            SDL_SetRenderDrawColor(renderer, 100, 150, 200, 200);
            SDL_RenderFillRect(renderer, &scopeLens);

            // Barrel highlight
            SDL_Rect barrelHighlight = {weaponX + 150, weaponY + 132, 60, 10};
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderFillRect(renderer, &barrelHighlight);

            // Bipod
            SDL_Rect bipod1 = {weaponX + 130, weaponY + 153, 3, 25};
            SDL_Rect bipod2 = {weaponX + 145, weaponY + 153, 3, 25};
            SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
            SDL_RenderFillRect(renderer, &bipod1);
            SDL_RenderFillRect(renderer, &bipod2);
        } else if (currentWeapon == WeaponType::FLAMETHROWER) {
            // Flamethrower (Bulky and dangerous)
            // Fuel tank (large red tank)
            SDL_Rect tank = {weaponX + 30, weaponY + 90, 80, 100};
            SDL_SetRenderDrawColor(renderer, 150, 50, 50, 255);
            SDL_RenderFillRect(renderer, &tank);

            // Tank highlight
            SDL_Rect tankHighlight = {weaponX + 40, weaponY + 100, 30, 40};
            SDL_SetRenderDrawColor(renderer, 180, 70, 70, 255);
            SDL_RenderFillRect(renderer, &tankHighlight);

            // Tank straps
            SDL_Rect strap1 = {weaponX + 35, weaponY + 120, 70, 5};
            SDL_Rect strap2 = {weaponX + 35, weaponY + 150, 70, 5};
            SDL_SetRenderDrawColor(renderer, 80, 80, 60, 255);
            SDL_RenderFillRect(renderer, &strap1);
            SDL_RenderFillRect(renderer, &strap2);

            // Nozzle body
            SDL_Rect nozzleBody = {weaponX + 100, weaponY + 130, 70, 40};
            SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
            SDL_RenderFillRect(renderer, &nozzleBody);

            // Nozzle tip (brass/bronze)
            SDL_Rect nozzleTip = {weaponX + 160, weaponY + 138, 40, 24};
            SDL_SetRenderDrawColor(renderer, 150, 120, 60, 255);
            SDL_RenderFillRect(renderer, &nozzleTip);

            // Pilot light (small orange glow)
            SDL_Rect pilotLight = {weaponX + 195, weaponY + 145, 5, 10};
            SDL_SetRenderDrawColor(renderer, 255, 150, 0, 255);
            SDL_RenderFillRect(renderer, &pilotLight);

            // Handle
            SDL_Rect handle = {weaponX + 110, weaponY + 160, 25, 45};
            SDL_SetRenderDrawColor(renderer, 50, 40, 30, 255);
            SDL_RenderFillRect(renderer, &handle);
        }

        // Draw crosshair in center of screen
        int centerX = SCREEN_WIDTH / 2;
        int centerY = SCREEN_HEIGHT / 2;
        int crosshairSize = 15;
        int crosshairThickness = 2;
        int crosshairGap = 5;

        // Crosshair with black outline for visibility
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        // Horizontal line (left)
        SDL_Rect crosshairHL1 = {centerX - crosshairSize - 1, centerY - crosshairThickness/2 - 1,
                                  crosshairSize - crosshairGap + 2, crosshairThickness + 2};
        SDL_RenderFillRect(renderer, &crosshairHL1);
        // Horizontal line (right)
        SDL_Rect crosshairHL2 = {centerX + crosshairGap - 1, centerY - crosshairThickness/2 - 1,
                                  crosshairSize - crosshairGap + 2, crosshairThickness + 2};
        SDL_RenderFillRect(renderer, &crosshairHL2);
        // Vertical line (top)
        SDL_Rect crosshairVL1 = {centerX - crosshairThickness/2 - 1, centerY - crosshairSize - 1,
                                  crosshairThickness + 2, crosshairSize - crosshairGap + 2};
        SDL_RenderFillRect(renderer, &crosshairVL1);
        // Vertical line (bottom)
        SDL_Rect crosshairVL2 = {centerX - crosshairThickness/2 - 1, centerY + crosshairGap - 1,
                                  crosshairThickness + 2, crosshairSize - crosshairGap + 2};
        SDL_RenderFillRect(renderer, &crosshairVL2);

        // White crosshair on top
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
        // Horizontal line (left)
        SDL_Rect crosshairH1 = {centerX - crosshairSize, centerY - crosshairThickness/2,
                                 crosshairSize - crosshairGap, crosshairThickness};
        SDL_RenderFillRect(renderer, &crosshairH1);
        // Horizontal line (right)
        SDL_Rect crosshairH2 = {centerX + crosshairGap, centerY - crosshairThickness/2,
                                 crosshairSize - crosshairGap, crosshairThickness};
        SDL_RenderFillRect(renderer, &crosshairH2);
        // Vertical line (top)
        SDL_Rect crosshairV1 = {centerX - crosshairThickness/2, centerY - crosshairSize,
                                 crosshairThickness, crosshairSize - crosshairGap};
        SDL_RenderFillRect(renderer, &crosshairV1);
        // Vertical line (bottom)
        SDL_Rect crosshairV2 = {centerX - crosshairThickness/2, centerY + crosshairGap,
                                 crosshairThickness, crosshairSize - crosshairGap};
        SDL_RenderFillRect(renderer, &crosshairV2);

        // Center dot
        SDL_Rect crosshairDot = {centerX - 1, centerY - 1, 2, 2};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150);
        SDL_RenderFillRect(renderer, &crosshairDot);

        // === THICK ATMOSPHERIC FOG OVERLAY ===
        // Add VERY DENSE creepy fog that obscures everything
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        // Full screen heavy fog base layer
        SDL_Rect fullFog = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 30, 30, 35, 120);  // MUCH thicker base fog
        SDL_RenderFillRect(renderer, &fullFog);

        // Add pulsing fog effect for atmosphere
        Uint32 time = SDL_GetTicks();
        float fogPulse = (std::sin(time / 500.0f) + 1.0f) / 2.0f;
        int pulseAlpha = static_cast<int>(40 + fogPulse * 30);

        SDL_SetRenderDrawColor(renderer, 25, 25, 30, pulseAlpha);
        SDL_RenderFillRect(renderer, &fullFog);

        // Extra thick fog at edges (vignette effect)
        const int VIGNETTE_SIZE = 150;
        for (int i = 0; i < VIGNETTE_SIZE; i++) {
            float vignetteStrength = (float)i / VIGNETTE_SIZE;
            int vignetteAlpha = static_cast<int>(vignetteStrength * 100);

            SDL_SetRenderDrawColor(renderer, 10, 10, 15, vignetteAlpha);

            // Top
            SDL_RenderDrawLine(renderer, 0, i, SCREEN_WIDTH, i);
            // Bottom
            SDL_RenderDrawLine(renderer, 0, SCREEN_HEIGHT - i - 1, SCREEN_WIDTH, SCREEN_HEIGHT - i - 1);
            // Left
            SDL_RenderDrawLine(renderer, i, 0, i, SCREEN_HEIGHT);
            // Right
            SDL_RenderDrawLine(renderer, SCREEN_WIDTH - i - 1, 0, SCREEN_WIDTH - i - 1, SCREEN_HEIGHT);
        }

        // === ZOMBIE EYES GLOW THROUGH FOG (BUT NOT WALLS!) ===
        // Render glowing red eyes ONLY for zombies with line of sight
        for (const auto& zombie : state.zombies) {
            if (!zombie->isDead()) {
                float dx = zombie->getX() - playerX;
                float dy = zombie->getY() - playerY;
                float distance = std::sqrt(dx * dx + dy * dy);

                // Show eyes ONLY if close AND have line of sight (NO WALL HACKS!)
                if (distance < 1000.0f && hasLineOfSight(state.maze.get(), playerX, playerY, zombie->getX(), zombie->getY())) {
                    // Transform zombie position to screen space
                    float invDet = 1.0f / (std::cos(playerAngle + M_PI/2) * std::sin(playerAngle) -
                                           std::sin(playerAngle + M_PI/2) * std::cos(playerAngle));
                    float transformX = invDet * (std::sin(playerAngle) * dx - std::cos(playerAngle) * dy);
                    float transformY = invDet * (-std::sin(playerAngle + M_PI/2) * dx + std::cos(playerAngle + M_PI/2) * dy);

                    // Only render if in front of player
                    if (transformY > 0.1f) {
                        int screenX = static_cast<int>((SCREEN_WIDTH / 2) * (1 + transformX / transformY / std::tan(FOV/2)));

                        // Calculate eye glow size (smaller when further away)
                        int glowSize = static_cast<int>(800.0f / transformY);
                        if (glowSize < 2) glowSize = 2;
                        if (glowSize > 25) glowSize = 25;

                        int screenY = SCREEN_HEIGHT / 2 - glowSize;

                        // Pulsing glow effect
                        Uint32 time = SDL_GetTicks();
                        float pulse = (std::sin(time / 200.0f + distance / 100.0f) + 1.0f) / 2.0f;

                        // Draw GLOWING RED EYES that pierce through fog!
                        int eyeGlowAlpha = static_cast<int>(150 + pulse * 80);
                        int eyeSeparation = glowSize / 2;

                        // Left eye - outer glow (red halo)
                        SDL_Rect leftEyeGlow = {screenX - eyeSeparation - glowSize, screenY - glowSize/2,
                                                glowSize * 2, glowSize * 2};
                        SDL_SetRenderDrawColor(renderer, 255, 50, 50, eyeGlowAlpha / 3);
                        SDL_RenderFillRect(renderer, &leftEyeGlow);

                        // Left eye - bright center
                        SDL_Rect leftEye = {screenX - eyeSeparation - glowSize/2, screenY,
                                           glowSize, glowSize};
                        SDL_SetRenderDrawColor(renderer, 255, 20, 20, eyeGlowAlpha);
                        SDL_RenderFillRect(renderer, &leftEye);

                        // Right eye - outer glow
                        SDL_Rect rightEyeGlow = {screenX + eyeSeparation - glowSize, screenY - glowSize/2,
                                                 glowSize * 2, glowSize * 2};
                        SDL_SetRenderDrawColor(renderer, 255, 50, 50, eyeGlowAlpha / 3);
                        SDL_RenderFillRect(renderer, &rightEyeGlow);

                        // Right eye - bright center
                        SDL_Rect rightEye = {screenX + eyeSeparation - glowSize/2, screenY,
                                            glowSize, glowSize};
                        SDL_SetRenderDrawColor(renderer, 255, 20, 20, eyeGlowAlpha);
                        SDL_RenderFillRect(renderer, &rightEye);
                    }
                }
            }
        }

        // Reset blend mode
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    void renderZombieDirectionArrow(SDL_Renderer* renderer, const PlayState& state) {
        const int SCREEN_WIDTH = Game::SCREEN_WIDTH;
        const int SCREEN_HEIGHT = Game::SCREEN_HEIGHT;
        const float FOV = 75.0f * M_PI / 180.0f;  // Match render FOV

        // Find nearest zombie
        float nearestDist = std::numeric_limits<float>::max();
        const Zombie* nearestZombie = nullptr;

        for (const auto& zombie : state.zombies) {
            if (!zombie->isDead()) {
                float dx = zombie->getX() - state.player->getX();
                float dy = zombie->getY() - state.player->getY();
                float dist = std::sqrt(dx * dx + dy * dy);

                if (dist < nearestDist) {
                    nearestDist = dist;
                    nearestZombie = zombie.get();
                }
            }
        }

        if (!nearestZombie) return;  // No zombies alive

        float playerX = state.player->getX();
        float playerY = state.player->getY();
        float playerAngle = state.player->getAngle();

        // Calculate angle to nearest zombie
        float dx = nearestZombie->getX() - playerX;
        float dy = nearestZombie->getY() - playerY;
        float zombieAngle = std::atan2(dy, dx);

        // Place arrow at fixed distance on floor (100 units from player)
        const float ARROW_DISTANCE = 100.0f;
        float arrowX = playerX + std::cos(zombieAngle) * ARROW_DISTANCE;
        float arrowY = playerY + std::sin(zombieAngle) * ARROW_DISTANCE;

        // Transform arrow to camera space
        float arrowDx = arrowX - playerX;
        float arrowDy = arrowY - playerY;

        float invDet = 1.0f / (std::cos(playerAngle + M_PI/2) * std::sin(playerAngle) -
                               std::sin(playerAngle + M_PI/2) * std::cos(playerAngle));
        float transformX = invDet * (std::sin(playerAngle) * arrowDx - std::cos(playerAngle) * arrowDy);
        float transformY = invDet * (-std::sin(playerAngle + M_PI/2) * arrowDx + std::cos(playerAngle + M_PI/2) * arrowDy);

        // Skip if behind player
        if (transformY <= 0.1f) return;

        // Calculate screen position (centered horizontally at bottom of screen)
        int screenX = (int)((SCREEN_WIDTH / 2) * (1 + transformX / transformY / std::tan(FOV/2)));

        // Place arrow on floor (lower 1/4 of screen, scaled by distance)
        int baseY = SCREEN_HEIGHT - SCREEN_HEIGHT / 8;  // Near bottom of screen
        int arrowSize = (int)(200.0f / transformY);  // Size scales with distance
        arrowSize = std::max(20, std::min(80, arrowSize));  // Clamp size

        // Calculate arrow rotation (always points toward zombie relative to player view)
        float arrowRotation = zombieAngle - playerAngle;

        // Draw arrow pointing in direction of zombie
        float cosA = std::cos(arrowRotation);
        float sinA = std::sin(arrowRotation);

        // Arrow dimensions
        int shaftLength = arrowSize / 2;
        int headLength = arrowSize / 3;
        int headWidth = arrowSize / 3;

        // Arrow center point
        int centerX = screenX;
        int centerY = baseY;

        // Draw background circle
        SDL_Rect bgCircle = {centerX - arrowSize/2, centerY - arrowSize/2, arrowSize, arrowSize};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 180);
        SDL_RenderFillRect(renderer, &bgCircle);

        // Arrow shaft
        int shaftEndX = centerX + (int)(cosA * shaftLength);
        int shaftEndY = centerY + (int)(sinA * shaftLength);

        SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
        for (int i = -2; i <= 2; i++) {
            float perpCos = std::cos(arrowRotation + M_PI / 2.0f);
            float perpSin = std::sin(arrowRotation + M_PI / 2.0f);
            int offsetX1 = centerX + (int)(perpCos * i);
            int offsetY1 = centerY + (int)(perpSin * i);
            int offsetX2 = shaftEndX + (int)(perpCos * i);
            int offsetY2 = shaftEndY + (int)(perpSin * i);
            SDL_RenderDrawLine(renderer, offsetX1, offsetY1, offsetX2, offsetY2);
        }

        // Arrowhead
        int tipX = centerX + (int)(cosA * (shaftLength + headLength));
        int tipY = centerY + (int)(sinA * (shaftLength + headLength));

        float perpCos = std::cos(arrowRotation + M_PI / 2.0f);
        float perpSin = std::sin(arrowRotation + M_PI / 2.0f);
        int head1X = shaftEndX + (int)(perpCos * headWidth);
        int head1Y = shaftEndY + (int)(perpSin * headWidth);
        int head2X = shaftEndX - (int)(perpCos * headWidth);
        int head2Y = shaftEndY - (int)(perpSin * headWidth);

        SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);

        // Draw filled arrowhead triangle
        for (int y = std::min({tipY, head1Y, head2Y}); y <= std::max({tipY, head1Y, head2Y}); y++) {
            for (int x = std::min({tipX, head1X, head2X}); x <= std::max({tipX, head1X, head2X}); x++) {
                float denom = (head1Y - head2Y) * (tipX - head2X) + (head2X - head1X) * (tipY - head2Y);
                if (std::abs(denom) < 0.001f) continue;

                float a = ((head1Y - head2Y) * (x - head2X) + (head2X - head1X) * (y - head2Y)) / denom;
                float b = ((head2Y - tipY) * (x - head2X) + (tipX - head2X) * (y - head2Y)) / denom;
                float c = 1.0f - a - b;

                if (a >= 0 && b >= 0 && c >= 0) {
                    SDL_RenderDrawPoint(renderer, x, y);
                }
            }
        }

        // Outline
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
        SDL_RenderDrawLine(renderer, tipX, tipY, head1X, head1Y);
        SDL_RenderDrawLine(renderer, tipX, tipY, head2X, head2Y);
        SDL_RenderDrawLine(renderer, head1X, head1Y, head2X, head2Y);

        // Draw border
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderDrawRect(renderer, &bgCircle);
    }

    void renderMinimap(SDL_Renderer* renderer, const PlayState& state) {
        const int MINIMAP_SIZE = 150;
        const int MINIMAP_X = 10;
        const int MINIMAP_Y = Game::SCREEN_HEIGHT - MINIMAP_SIZE - 10;

        // Background
        SDL_Rect minimapBg = {MINIMAP_X - 2, MINIMAP_Y - 2, MINIMAP_SIZE + 4, MINIMAP_SIZE + 4};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 220);
        SDL_RenderFillRect(renderer, &minimapBg);

        SDL_Rect minimap = {MINIMAP_X, MINIMAP_Y, MINIMAP_SIZE, MINIMAP_SIZE};
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderFillRect(renderer, &minimap);

        // Calculate scale
        float scaleX = (float)MINIMAP_SIZE / (Maze::WIDTH * Maze::TILE_SIZE);
        float scaleY = (float)MINIMAP_SIZE / (Maze::HEIGHT * Maze::TILE_SIZE);

        // Render maze walls
        // Outside testing mode: only show nearby walls (fog of war)
        bool isTestingMode = (state.difficulty == Difficulty::TESTING);
        float playerTileX = state.player->getX() / Maze::TILE_SIZE;
        float playerTileY = state.player->getY() / Maze::TILE_SIZE;
        float visibilityRadius = isTestingMode ? 9999.0f : 8.0f;  // Show 8 tiles around player in normal mode

        for (int y = 0; y < Maze::HEIGHT; y++) {
            for (int x = 0; x < Maze::WIDTH; x++) {
                // Check distance to player (for fog of war outside testing mode)
                float dx = x - playerTileX;
                float dy = y - playerTileY;
                float distance = std::sqrt(dx*dx + dy*dy);
                bool isVisible = (distance <= visibilityRadius);

                if (!isTestingMode && !isVisible) {
                    continue;  // Skip tiles that are too far in normal mode
                }

                if (state.maze->isSafeRoom(x, y)) {
                    // BLUE SAFE ROOM - render it blue!
                    SDL_Rect safeRoomRect = {
                        MINIMAP_X + (int)(x * Maze::TILE_SIZE * scaleX),
                        MINIMAP_Y + (int)(y * Maze::TILE_SIZE * scaleY),
                        std::max(2, (int)(Maze::TILE_SIZE * scaleX)),
                        std::max(2, (int)(Maze::TILE_SIZE * scaleY))
                    };
                    // Bright blue with glow effect
                    SDL_SetRenderDrawColor(renderer, 50, 150, 255, 255);
                    SDL_RenderFillRect(renderer, &safeRoomRect);
                } else if (state.maze->isWall(x, y)) {
                    SDL_Rect wallRect = {
                        MINIMAP_X + (int)(x * Maze::TILE_SIZE * scaleX),
                        MINIMAP_Y + (int)(y * Maze::TILE_SIZE * scaleY),
                        std::max(2, (int)(Maze::TILE_SIZE * scaleX)),
                        std::max(2, (int)(Maze::TILE_SIZE * scaleY))
                    };
                    SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255);
                    SDL_RenderFillRect(renderer, &wallRect);
                } else if (state.maze->isExit(x, y) && isTestingMode) {
                    // Only show exit in testing mode
                    SDL_Rect exitRect = {
                        MINIMAP_X + (int)(x * Maze::TILE_SIZE * scaleX),
                        MINIMAP_Y + (int)(y * Maze::TILE_SIZE * scaleY),
                        std::max(2, (int)(Maze::TILE_SIZE * scaleX)),
                        std::max(2, (int)(Maze::TILE_SIZE * scaleY))
                    };
                    SDL_SetRenderDrawColor(renderer, 0, 200, 100, 255);
                    SDL_RenderFillRect(renderer, &exitRect);
                }
            }
        }

        // Only show entities in TESTING mode - normal mode shows only walls/player
        if (isTestingMode) {
            // Render keys
            for (const auto& key : state.keys) {
                if (!key->isCollected()) {
                    int mapX = MINIMAP_X + (int)(key->getX() * scaleX);
                    int mapY = MINIMAP_Y + (int)(key->getY() * scaleY);
                    SDL_Rect keyRect = {mapX - 2, mapY - 2, 4, 4};
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                    SDL_RenderFillRect(renderer, &keyRect);
                }
            }

            // Render weapon pickups
            for (const auto& weapon : state.weaponPickups) {
                if (!weapon->isCollected()) {
                    int mapX = MINIMAP_X + (int)(weapon->getX() * scaleX);
                    int mapY = MINIMAP_Y + (int)(weapon->getY() * scaleY);
                    SDL_Rect weaponRect = {mapX - 2, mapY - 2, 4, 4};
                    if (weapon->getIsAmmo()) {
                        SDL_SetRenderDrawColor(renderer, 255, 180, 50, 255);  // Orange for ammo
                    } else {
                        SDL_SetRenderDrawColor(renderer, 100, 180, 255, 255);  // Blue for weapons
                    }
                    SDL_RenderFillRect(renderer, &weaponRect);
                }
            }

            // Render health boosts
            for (const auto& health : state.healthBoosts) {
                if (!health->isCollected()) {
                    int mapX = MINIMAP_X + (int)(health->getX() * scaleX);
                    int mapY = MINIMAP_Y + (int)(health->getY() * scaleY);
                    SDL_Rect healthRect = {mapX - 2, mapY - 2, 4, 4};
                    SDL_SetRenderDrawColor(renderer, 50, 255, 50, 255);  // Green for health
                    SDL_RenderFillRect(renderer, &healthRect);
                }
            }

            // Render zombies
            for (const auto& zombie : state.zombies) {
                if (!zombie->isDead()) {
                    int mapX = MINIMAP_X + (int)(zombie->getX() * scaleX);
                    int mapY = MINIMAP_Y + (int)(zombie->getY() * scaleY);
                    SDL_Rect zombieRect = {mapX - 2, mapY - 2, 4, 4};
                    SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
                    SDL_RenderFillRect(renderer, &zombieRect);
                }
            }

            // Render hunters (dark purple entities)
            for (const auto& hunter : state.hunters) {
                if (!hunter->isDead()) {
                    int mapX = MINIMAP_X + (int)(hunter->getX() * scaleX);
                    int mapY = MINIMAP_Y + (int)(hunter->getY() * scaleY);
                    SDL_Rect hunterRect = {mapX - 2, mapY - 2, 5, 5};  // Slightly bigger than zombies
                    SDL_SetRenderDrawColor(renderer, 150, 50, 200, 255);  // Dark purple
                    SDL_RenderFillRect(renderer, &hunterRect);
                    // Add border to make them stand out
                    SDL_SetRenderDrawColor(renderer, 200, 100, 255, 255);  // Lighter purple border
                    SDL_RenderDrawRect(renderer, &hunterRect);
                }
            }
        }

        // Render spawn location indicator (purple square) when spawn at crosshair is enabled
        if (state.spawnAtCrosshair && state.showTestingPanel) {
            // Calculate spawn position using same logic as weapon/zombie spawning
            float angle = state.player->getAngle();
            float pitch = state.player->getPitch();
            float baseRange = 150.0f;  // Closer spawn range
            float pitchFactor = 1.0f + pitch;
            pitchFactor = std::max(0.3f, std::min(3.0f, pitchFactor));
            float adjustedRange = baseRange * pitchFactor;
            float spawnX = state.player->getX() + std::cos(angle) * adjustedRange;
            float spawnY = state.player->getY() + std::sin(angle) * adjustedRange;

            // Draw purple spawn indicator on minimap
            int spawnMapX = MINIMAP_X + (int)(spawnX * scaleX);
            int spawnMapY = MINIMAP_Y + (int)(spawnY * scaleY);

            // Draw pulsing purple square
            float pulseAmount = 0.7f + 0.3f * std::sin(SDL_GetTicks() * 0.005f);
            int squareSize = static_cast<int>(6 * pulseAmount);
            SDL_Rect spawnRect = {spawnMapX - squareSize/2, spawnMapY - squareSize/2, squareSize, squareSize};
            SDL_SetRenderDrawColor(renderer, 200, 100, 255, 255);  // Purple
            SDL_RenderFillRect(renderer, &spawnRect);

            // Draw border for visibility
            SDL_SetRenderDrawColor(renderer, 255, 150, 255, 255);  // Lighter purple
            SDL_RenderDrawRect(renderer, &spawnRect);
        }

        // Render player
        int playerMapX = MINIMAP_X + (int)(state.player->getX() * scaleX);
        int playerMapY = MINIMAP_Y + (int)(state.player->getY() * scaleY);
        SDL_Rect playerRect = {playerMapX - 3, playerMapY - 3, 6, 6};
        SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
        SDL_RenderFillRect(renderer, &playerRect);

        // Draw player facing indicator (direction line)
        float angle = state.player->getAngle();
        int lineLength = 10;
        int endX = playerMapX + (int)(std::cos(angle) * lineLength);
        int endY = playerMapY + (int)(std::sin(angle) * lineLength);
        SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);
        SDL_RenderDrawLine(renderer, playerMapX, playerMapY, endX, endY);

        // Draw a small triangle at the end to show direction
        int arrowSize = 3;
        float perpAngle = angle + M_PI / 2.0f;
        int arrow1X = endX + (int)(std::cos(angle - 2.5f) * arrowSize);
        int arrow1Y = endY + (int)(std::sin(angle - 2.5f) * arrowSize);
        int arrow2X = endX + (int)(std::cos(angle + 2.5f) * arrowSize);
        int arrow2Y = endY + (int)(std::sin(angle + 2.5f) * arrowSize);
        SDL_RenderDrawLine(renderer, endX, endY, arrow1X, arrow1Y);
        SDL_RenderDrawLine(renderer, endX, endY, arrow2X, arrow2Y);

        // Cardinal directions (N, S, E, W) - always visible
        // North (top)
        renderText(renderer, "N", MINIMAP_X + MINIMAP_SIZE/2 - 3, MINIMAP_Y - 12, 1);
        // South (bottom)
        renderText(renderer, "S", MINIMAP_X + MINIMAP_SIZE/2 - 3, MINIMAP_Y + MINIMAP_SIZE + 4, 1);
        // West (left)
        renderText(renderer, "W", MINIMAP_X - 10, MINIMAP_Y + MINIMAP_SIZE/2 - 4, 1);
        // East (right)
        renderText(renderer, "E", MINIMAP_X + MINIMAP_SIZE + 4, MINIMAP_Y + MINIMAP_SIZE/2 - 4, 1);

        // Border
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer, &minimap);
    }

    void renderTestingPanel(SDL_Renderer* renderer, PlayState& state) {
        const int PANEL_WIDTH = 300;
        const int PANEL_HEIGHT = 560;  // Increased for spawn mode toggle and hunter button
        const int PANEL_X = Game::SCREEN_WIDTH - PANEL_WIDTH - 20;
        const int PANEL_Y = 20;

        // Semi-transparent dark background
        SDL_Rect panelBg = {PANEL_X - 5, PANEL_Y - 5, PANEL_WIDTH + 10, PANEL_HEIGHT + 10};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_RenderFillRect(renderer, &panelBg);

        // Border
        SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
        SDL_RenderDrawRect(renderer, &panelBg);

        int yOffset = PANEL_Y + 10;
        int lineHeight = 30;

        // Title
        SDL_Rect titleBg = {PANEL_X, yOffset, PANEL_WIDTH, 25};
        SDL_SetRenderDrawColor(renderer, 50, 150, 50, 255);
        SDL_RenderFillRect(renderer, &titleBg);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        renderText(renderer, "TESTING PANEL", PANEL_X + 70, yOffset + 8, 2);
        yOffset += 35;

        // God Mode Toggle
        SDL_Rect godModeBox = {PANEL_X + 10, yOffset, 20, 20};
        if (state.godMode) {
            SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
            SDL_RenderFillRect(renderer, &godModeBox);
            // X mark when checked
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawLine(renderer, PANEL_X + 12, yOffset + 10, PANEL_X + 28, yOffset + 10);
            SDL_RenderDrawLine(renderer, PANEL_X + 20, yOffset + 5, PANEL_X + 20, yOffset + 15);
        }
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer, &godModeBox);

        // God mode label
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        renderText(renderer, "GOD MODE", PANEL_X + 40, yOffset + 5, 2);
        yOffset += lineHeight;

        // Spawn Location Mode Toggle
        SDL_Rect spawnModeBox = {PANEL_X + 10, yOffset, 20, 20};
        if (state.spawnAtCrosshair) {
            SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
            SDL_RenderFillRect(renderer, &spawnModeBox);
            // X mark when checked
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawLine(renderer, PANEL_X + 12, yOffset + 10, PANEL_X + 28, yOffset + 10);
            SDL_RenderDrawLine(renderer, PANEL_X + 20, yOffset + 5, PANEL_X + 20, yOffset + 15);
        }
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer, &spawnModeBox);

        // Spawn mode label
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        renderText(renderer, "SPAWN AT CROSSHAIR", PANEL_X + 40, yOffset + 5, 2);
        yOffset += lineHeight;

        // Weapon Spawning Section
        yOffset += 10;
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        renderText(renderer, "SPAWN WEAPON", PANEL_X + 10, yOffset, 2);
        yOffset += 20;

        // Weapon buttons with text labels
        const char* weaponNames[] = {"SHOTGUN", "PISTOL", "AR", "GRENADE", "SMG", "SNIPER", "FLAME"};
        for (int i = 0; i < 7; i++) {
            SDL_Rect weaponBtn = {PANEL_X + 10, yOffset, PANEL_WIDTH - 20, 22};
            if (state.selectedWeaponSpawn == i) {
                SDL_SetRenderDrawColor(renderer, 100, 150, 255, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 60, 60, 80, 255);
            }
            SDL_RenderFillRect(renderer, &weaponBtn);
            SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
            SDL_RenderDrawRect(renderer, &weaponBtn);

            // Render weapon name
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            renderText(renderer, weaponNames[i], PANEL_X + 15, yOffset + 6, 2);
            yOffset += 25;
        }

        yOffset += 10;
        // Zombie Spawning Section
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        renderText(renderer, "SPAWN ENTITIES", PANEL_X + 10, yOffset, 2);
        yOffset += 20;

        // Spawn zombie button
        SDL_Rect spawnZombieBtn = {PANEL_X + 10, yOffset, PANEL_WIDTH - 20, 22};
        SDL_SetRenderDrawColor(renderer, 150, 50, 50, 255);
        SDL_RenderFillRect(renderer, &spawnZombieBtn);
        SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
        SDL_RenderDrawRect(renderer, &spawnZombieBtn);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        renderText(renderer, "SPAWN ZOMBIE", PANEL_X + 106, yOffset + 6, 2);
        yOffset += 25;

        // Spawn hunter button
        SDL_Rect spawnHunterBtn = {PANEL_X + 10, yOffset, PANEL_WIDTH - 20, 22};
        SDL_SetRenderDrawColor(renderer, 40, 20, 60, 255);  // Dark purple
        SDL_RenderFillRect(renderer, &spawnHunterBtn);
        SDL_SetRenderDrawColor(renderer, 100, 50, 150, 255);
        SDL_RenderDrawRect(renderer, &spawnHunterBtn);
        SDL_SetRenderDrawColor(renderer, 200, 150, 255, 255);
        renderText(renderer, "SPAWN HUNTER", PANEL_X + 100, yOffset + 6, 2);
        yOffset += 30;

        // Trigger Blood Moon button
        SDL_Rect bloodMoonBtn = {PANEL_X + 10, yOffset, PANEL_WIDTH - 20, 22};
        SDL_SetRenderDrawColor(renderer, 120, 0, 0, 255);
        SDL_RenderFillRect(renderer, &bloodMoonBtn);
        SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
        SDL_RenderDrawRect(renderer, &bloodMoonBtn);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        renderText(renderer, "BLOOD MOON", PANEL_X + 90, yOffset + 6, 2);
        yOffset += 30;

        // Trigger Blue Alert button
        SDL_Rect blueAlertBtn = {PANEL_X + 10, yOffset, PANEL_WIDTH - 20, 22};
        SDL_SetRenderDrawColor(renderer, 0, 80, 150, 255);
        SDL_RenderFillRect(renderer, &blueAlertBtn);
        SDL_SetRenderDrawColor(renderer, 100, 180, 255, 255);
        SDL_RenderDrawRect(renderer, &blueAlertBtn);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        renderText(renderer, "BLUE ALERT", PANEL_X + 96, yOffset + 6, 2);

        // Instructions at bottom
        yOffset = PANEL_Y + PANEL_HEIGHT - 45;
        SDL_Rect instrBg = {PANEL_X + 5, yOffset, PANEL_WIDTH - 10, 40};
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderFillRect(renderer, &instrBg);
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        renderText(renderer, "CLICK BUTTONS TO USE", PANEL_X + 40, yOffset + 8, 1);
        renderText(renderer, "F1 TO CLOSE PANEL", PANEL_X + 52, yOffset + 22, 1);
    }

    void initializeGame(PlayState& state, Difficulty difficulty, MazeType mazeType = MazeType::STANDARD, bool isLevelProgression = false) {
        // Store maze type for infinite mode
        state.mazeType = mazeType;

        // If this is not a level progression, reset to level 1
        if (!isLevelProgression) {
            state.currentLevel = 1;
        }

        // Apply difficulty settings
        applyDifficulty(state, difficulty);

        // Scale difficulty for infinite mode levels > 7
        if (mazeType == MazeType::INFINITE && state.currentLevel > 7) {
            // Add 3 more zombies per level after 7
            int bonusZombies = (state.currentLevel - 7) * 3;
            state.initialZombieCount += bonusZombies;
            state.maxZombieCount += bonusZombies;
        }

        // Create new maze (randomly generated with selected type)
        state.maze = std::make_unique<Maze>(mazeType);

        // Reset player at start position
        Vec2 startPos = state.maze->getPlayerStart();
        state.player = std::make_unique<Player>(startPos.x, startPos.y);

        // SOLDIER MODE: Special initialization
        if (mazeType == MazeType::SOLDIER) {
            // Give player assault rifle with infinite ammo in slot 0
            state.player->pickupWeapon(WeaponType::ASSAULT_RIFLE);
            // Give infinite ammo (just a very large amount)
            state.player->pickupAmmo(WeaponType::ASSAULT_RIFLE, 999999);

            // Initialize wave system
            state.currentWave = 1;
            state.waveActive = false;
            state.waveDelayTimer = state.waveDelay;  // Start first wave after delay

            // Don't spawn any initial zombies (they spawn in waves)
            state.zombies.clear();
            state.totalZombiesSpawned = 0;
            state.spawnTimer = 0.0f;

            // No keys needed in Soldier mode
            state.keys.clear();
        }
        // TESTING MODE: Give player all weapons after creation
        else if (difficulty == Difficulty::TESTING) {
            // Give player ALL weapons - cycle through both slots
            state.player->pickupWeapon(WeaponType::SHOTGUN);
            state.player->switchWeapon();
            state.player->pickupWeapon(WeaponType::ASSAULT_RIFLE);
            state.player->switchWeapon();
            state.player->pickupWeapon(WeaponType::SMG);
            state.player->switchWeapon();
            state.player->pickupWeapon(WeaponType::SNIPER);
            state.player->switchWeapon();
            state.player->pickupWeapon(WeaponType::GRENADE_LAUNCHER);
            state.player->switchWeapon();
            state.player->pickupWeapon(WeaponType::FLAMETHROWER);

            // Standard zombie/key initialization for testing mode
            state.zombies.clear();
            auto zombiePositions = state.maze->getRandomZombiePositions(state.initialZombieCount, startPos);
            for (const auto& pos : zombiePositions) {
                ZombieType type = getRandomZombieType();
                state.zombies.push_back(std::make_unique<Zombie>(pos.x, pos.y, state.zombieMaxHealth, type));
            }
            state.totalZombiesSpawned = state.initialZombieCount;
            state.spawnTimer = 0.0f;

            state.keys.clear();
            int requiredKeys = state.maze->getRequiredKeyCount(state.currentLevel);
            auto keyPositions = state.maze->getRandomKeyPositions(requiredKeys);
            for (const auto& pos : keyPositions) {
                state.keys.push_back(std::make_unique<Key>(pos.x, pos.y));
            }
        } else {
            // NORMAL MODES: Standard initialization
            // Clear and create zombies at random positions (far from player spawn)
            state.zombies.clear();
            auto zombiePositions = state.maze->getRandomZombiePositions(state.initialZombieCount, startPos);
            for (const auto& pos : zombiePositions) {
                ZombieType type = getRandomZombieType();
                state.zombies.push_back(std::make_unique<Zombie>(pos.x, pos.y, state.zombieMaxHealth, type));
            }
            state.totalZombiesSpawned = state.initialZombieCount;
            state.spawnTimer = 0.0f;

            // Clear and create keys at random positions
            state.keys.clear();
            int requiredKeys = state.maze->getRequiredKeyCount(state.currentLevel);
            auto keyPositions = state.maze->getRandomKeyPositions(requiredKeys);
            for (const auto& pos : keyPositions) {
                state.keys.push_back(std::make_unique<Key>(pos.x, pos.y));
            }
        }

        // Clear and create weapon pickups (skip in Soldier mode)
        state.weaponPickups.clear();
        if (mazeType != MazeType::SOLDIER) {
            int weaponCount = 3;
            if (mazeType == MazeType::INFINITE && state.currentLevel > 7) {
                // Reduce weapons as difficulty increases: level 8=2, level 9+=1
                weaponCount = std::max(1, 10 - state.currentLevel);
            }
            auto weaponPositions = state.maze->getRandomKeyPositions(weaponCount);
            // Spawn variety of weapons (cycle through all types)
            WeaponType weaponTypes[] = {WeaponType::ASSAULT_RIFLE, WeaponType::GRENADE_LAUNCHER,
                                         WeaponType::SMG, WeaponType::SNIPER, WeaponType::FLAMETHROWER};
            for (size_t i = 0; i < weaponPositions.size() && i < static_cast<size_t>(weaponCount); i++) {
                WeaponType weaponType = weaponTypes[i % 5];  // Cycle through all weapon types
                state.weaponPickups.push_back(std::make_unique<WeaponPickup>(weaponPositions[i].x, weaponPositions[i].y, weaponType));
            }
        }

        // Clear and create health boosts (skip in Soldier mode - rely on natural regen)
        state.healthBoosts.clear();
        if (mazeType != MazeType::SOLDIER) {
            int healthCount = 3;
            if (mazeType == MazeType::INFINITE && state.currentLevel > 7) {
                // Reduce health boosts: level 8=2, level 10+=1
                healthCount = std::max(1, 11 - state.currentLevel);
            }
            auto healthPositions = state.maze->getRandomKeyPositions(healthCount);
            for (size_t i = 0; i < healthPositions.size() && i < static_cast<size_t>(healthCount); i++) {
                state.healthBoosts.push_back(std::make_unique<HealthBoost>(healthPositions[i].x, healthPositions[i].y));
            }
        }

        // Clear bullets
        state.bullets.clear();

        // Reset game state
        state.deathTime = 0;

        // Reset current life score (keep totalScore)
        state.score = 0;
        state.zombiesKilled = 0;
        state.gameStartTime = SDL_GetTicks();
    }
}

void Game::run() {
    // Init SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }

    // Init SDL_mixer
    if (Mix_OpenAudio(22050, AUDIO_U8, 1, 512) < 0) {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    // Initialize sound effects
    initSounds();

    // Load high scores
    loadHighScores();

    auto window = SDL_CreateWindow("Zombie Maze Shooter",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   SCREEN_WIDTH,
                                   SCREEN_HEIGHT,
                                   SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);

    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    // Raise window to foreground (especially important on macOS)
    SDL_RaiseWindow(window);

    auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    // Initialize game state
    MenuState menu;
    PlayState playState;

    // Game loop variables
    bool running = true;
    Uint32 lastTime = SDL_GetTicks();
    bool mousePressed = false;

    // Game loop
    while (running) {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        if (deltaTime > 0.016f) deltaTime = 0.016f;  // Cap at ~60 FPS

        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (menu.currentState == GameState::MENU) {
                    // Main menu navigation
                    if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w) {
                        menu.menuSelection = (menu.menuSelection - 1 + 5) % 5;
                    } else if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s) {
                        menu.menuSelection = (menu.menuSelection + 1) % 5;
                    } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                        if (menu.menuSelection == 0) {
                            // Start game
                            initializeGame(playState, menu.difficulty, menu.mazeType);
                            menu.currentState = GameState::PLAYING;
                            SDL_SetRelativeMouseMode(SDL_TRUE);  // Lock mouse
                        } else if (menu.menuSelection == 1) {
                            // Go to maze type selection
                            menu.currentState = GameState::MAZE_TYPE_SELECT;
                        } else if (menu.menuSelection == 2) {
                            // Go to difficulty selection
                            menu.currentState = GameState::DIFFICULTY_SELECT;
                        } else if (menu.menuSelection == 3) {
                            // Go to controls screen
                            menu.currentState = GameState::CONTROLS;
                        } else if (menu.menuSelection == 4) {
                            // Quit
                            running = false;
                        }
                    } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    }
                } else if (menu.currentState == GameState::MAZE_TYPE_SELECT) {
                    // Maze type selection navigation
                    if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w) {
                        menu.mazeTypeSelection = (menu.mazeTypeSelection - 1 + 4) % 4;
                    } else if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s) {
                        menu.mazeTypeSelection = (menu.mazeTypeSelection + 1) % 4;
                    } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                        // Set maze type and return to main menu
                        menu.mazeType = static_cast<MazeType>(menu.mazeTypeSelection);
                        menu.currentState = GameState::MENU;
                    } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                        // Back to main menu
                        menu.currentState = GameState::MENU;
                    }
                } else if (menu.currentState == GameState::DIFFICULTY_SELECT) {
                    // Difficulty selection navigation
                    if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w) {
                        menu.difficultySelection = (menu.difficultySelection - 1 + 4) % 4;
                    } else if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s) {
                        menu.difficultySelection = (menu.difficultySelection + 1) % 4;
                    } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                        // Check if TESTING mode selected and not unlocked
                        if (menu.difficultySelection == 3 && !menu.devModeUnlocked) {
                            // Go to code entry screen
                            menu.currentState = GameState::CODE_ENTRY;
                            menu.codeEntry = "";
                            menu.codeError = false;
                        } else {
                            // Set difficulty
                            Difficulty oldDifficulty = menu.difficulty;
                            menu.difficulty = static_cast<Difficulty>(menu.difficultySelection);

                            // If difficulty changed during gameplay, restart with new difficulty
                            if (playState.player != nullptr && oldDifficulty != menu.difficulty) {
                                initializeGame(playState, menu.difficulty, playState.mazeType);
                                menu.currentState = GameState::PLAYING;
                                SDL_SetRelativeMouseMode(SDL_TRUE);  // Lock mouse
                            } else {
                                // Otherwise just return to menu
                                menu.currentState = GameState::MENU;
                            }
                        }
                    } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                        // Back to previous state
                        if (playState.player != nullptr) {
                            menu.currentState = GameState::PAUSED;
                        } else {
                            menu.currentState = GameState::MENU;
                        }
                    }
                } else if (menu.currentState == GameState::CODE_ENTRY) {
                    // Code entry screen
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        menu.currentState = GameState::DIFFICULTY_SELECT;
                        menu.codeEntry = "";
                        menu.codeError = false;
                    } else if (event.key.keysym.sym == SDLK_RETURN) {
                        // Check if code is correct
                        if (menu.codeEntry == "012000163135") {
                            menu.devModeUnlocked = true;
                            menu.difficulty = Difficulty::TESTING;
                            menu.difficultySelection = 3;
                            menu.currentState = GameState::MENU;
                            menu.codeEntry = "";
                            menu.codeError = false;
                        } else {
                            menu.codeError = true;
                        }
                    } else if (event.key.keysym.sym == SDLK_BACKSPACE && !menu.codeEntry.empty()) {
                        menu.codeEntry.pop_back();
                        menu.codeError = false;
                    } else if (event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9) {
                        // Add digit to code
                        menu.codeEntry += static_cast<char>('0' + (event.key.keysym.sym - SDLK_0));
                        menu.codeError = false;
                    }
                } else if (menu.currentState == GameState::CONTROLS) {
                    // Controls screen - only ESC to go back
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        menu.currentState = GameState::MENU;
                    }
                } else if (menu.currentState == GameState::PLAYING) {
                    // In-game controls
                    if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_p) {
                        menu.currentState = GameState::PAUSED;
                        menu.pauseSelection = 0;
                        SDL_SetRelativeMouseMode(SDL_FALSE);  // Unlock mouse
                    } else if (event.key.keysym.sym == SDLK_q) {
                        // Switch weapon (ranged only)
                        if (!playState.player->isUsingMelee()) {
                            playState.player->switchWeapon();
                            WeaponStats stats = getWeaponStats(playState.player->getCurrentWeapon());
                            std::cout << "Switched to: " << stats.name << std::endl;
                        }
                    } else if (event.key.keysym.sym == SDLK_v) {
                        // Toggle between melee and ranged weapons
                        playState.player->setUsingMelee(!playState.player->isUsingMelee());
                        WeaponStats stats = getWeaponStats(playState.player->getCurrentWeapon());
                        std::cout << "Switched to: " << stats.name << (stats.isMelee ? " (MELEE)" : " (RANGED)") << std::endl;
                    } else if (event.key.keysym.sym == SDLK_h) {
                        // Toggle HUD elements (all at once)
                        bool newState = !playState.showScore;  // Toggle based on score state
                        playState.showScore = newState;
                        playState.showMinimap = newState;
                        playState.showArrow = newState;
                        std::cout << "HUD: " << (newState ? "ON" : "OFF") << std::endl;
                    } else if (event.key.keysym.sym == SDLK_m) {
                        // Toggle minimap only
                        playState.showMinimap = !playState.showMinimap;
                        std::cout << "Minimap: " << (playState.showMinimap ? "ON" : "OFF") << std::endl;
                    } else if (event.key.keysym.sym == SDLK_F1) {
                        // Toggle testing panel (only in TESTING mode)
                        if (playState.difficulty == Difficulty::TESTING) {
                            playState.showTestingPanel = !playState.showTestingPanel;
                            std::cout << "Testing Panel: " << (playState.showTestingPanel ? "ON" : "OFF") << std::endl;

                            // Unlock mouse when panel is open, lock when closed
                            if (playState.showTestingPanel) {
                                SDL_SetRelativeMouseMode(SDL_FALSE);  // Unlock mouse for clicking
                            } else {
                                SDL_SetRelativeMouseMode(SDL_TRUE);  // Lock mouse for gameplay
                            }
                        }
                    }
                } else if (menu.currentState == GameState::PAUSED) {
                    // Pause menu navigation
                    if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w) {
                        menu.pauseSelection = (menu.pauseSelection - 1 + 5) % 5;
                    } else if (event.key.keysym.sym == SDLK_DOWN || event.key.keysym.sym == SDLK_s) {
                        menu.pauseSelection = (menu.pauseSelection + 1) % 5;
                    } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                        if (menu.pauseSelection == 0) {
                            // Resume
                            menu.currentState = GameState::PLAYING;
                            SDL_SetRelativeMouseMode(SDL_TRUE);  // Lock mouse
                        } else if (menu.pauseSelection == 1) {
                            // Restart with current difficulty
                            initializeGame(playState, menu.difficulty);
                            menu.currentState = GameState::PLAYING;
                            SDL_SetRelativeMouseMode(SDL_TRUE);  // Lock mouse
                        } else if (menu.pauseSelection == 2) {
                            // Change difficulty
                            menu.currentState = GameState::DIFFICULTY_SELECT;
                            SDL_SetRelativeMouseMode(SDL_FALSE);  // Unlock mouse for menu
                        } else if (menu.pauseSelection == 3) {
                            // Main menu
                            menu.currentState = GameState::MENU;
                            SDL_SetRelativeMouseMode(SDL_FALSE);  // Unlock mouse
                        } else if (menu.pauseSelection == 4) {
                            // Quit
                            running = false;
                        }
                    } else if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_p) {
                        // Resume with ESC or P
                        menu.currentState = GameState::PLAYING;
                        SDL_SetRelativeMouseMode(SDL_TRUE);  // Lock mouse
                    }
                } else if (menu.currentState == GameState::GAME_WON || menu.currentState == GameState::GAME_LOST) {
                    // Win/lose screen
                    if (event.key.keysym.sym == SDLK_r) {
                        initializeGame(playState, menu.difficulty);
                        menu.currentState = GameState::PLAYING;
                        SDL_SetRelativeMouseMode(SDL_TRUE);  // Lock mouse
                    } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                        menu.currentState = GameState::MENU;
                        SDL_SetRelativeMouseMode(SDL_FALSE);  // Unlock mouse
                    }
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                if (menu.currentState == GameState::PLAYING) {
                    // Check if testing panel is open and handle clicks
                    if (playState.showTestingPanel && playState.difficulty == Difficulty::TESTING) {
                        int mouseX = event.button.x;
                        int mouseY = event.button.y;

                        const int PANEL_WIDTH = 300;
                        const int PANEL_X = Game::SCREEN_WIDTH - PANEL_WIDTH - 20;
                        const int PANEL_Y = 20;

                        // Calculate exact positions matching render function
                        int yOffset = PANEL_Y + 10;  // Start position
                        yOffset += 35;  // After title

                        // Check god mode checkbox click
                        SDL_Rect godModeBox = {PANEL_X + 10, yOffset, 20, 20};
                        if (mouseX >= godModeBox.x && mouseX <= godModeBox.x + godModeBox.w &&
                            mouseY >= godModeBox.y && mouseY <= godModeBox.y + godModeBox.h) {
                            playState.godMode = !playState.godMode;
                            std::cout << "God Mode: " << (playState.godMode ? "ON" : "OFF") << std::endl;
                        }

                        yOffset += 30;  // After god mode

                        // Check spawn mode toggle click
                        SDL_Rect spawnModeBox = {PANEL_X + 10, yOffset, 20, 20};
                        if (mouseX >= spawnModeBox.x && mouseX <= spawnModeBox.x + spawnModeBox.w &&
                            mouseY >= spawnModeBox.y && mouseY <= spawnModeBox.y + spawnModeBox.h) {
                            playState.spawnAtCrosshair = !playState.spawnAtCrosshair;
                            std::cout << "Spawn Location: " << (playState.spawnAtCrosshair ? "CROSSHAIR" : "PLAYER") << std::endl;
                        }

                        yOffset += 30;  // After spawn mode
                        yOffset += 10;  // Weapon section spacing
                        yOffset += 20;  // After weapon title

                        // Check weapon buttons (7 buttons)
                        for (int i = 0; i < 7; i++) {
                            SDL_Rect weaponBtn = {PANEL_X + 10, yOffset, PANEL_WIDTH - 20, 22};
                            if (mouseX >= weaponBtn.x && mouseX <= weaponBtn.x + weaponBtn.w &&
                                mouseY >= weaponBtn.y && mouseY <= weaponBtn.y + weaponBtn.h) {
                                // Spawn weapon at player position or crosshair
                                WeaponType weaponType = static_cast<WeaponType>(i);
                                float spawnX, spawnY;

                                if (playState.spawnAtCrosshair) {
                                    // Calculate crosshair position using aiming logic
                                    float angle = playState.player->getAngle();
                                    float pitch = playState.player->getPitch();
                                    float baseRange = 150.0f;  // Closer spawn range
                                    float pitchFactor = 1.0f + pitch;
                                    pitchFactor = std::max(0.3f, std::min(3.0f, pitchFactor));
                                    float adjustedRange = baseRange * pitchFactor;
                                    spawnX = playState.player->getX() + std::cos(angle) * adjustedRange;
                                    spawnY = playState.player->getY() + std::sin(angle) * adjustedRange;
                                } else {
                                    // Spawn in front of player facing direction
                                    float angle = playState.player->getAngle();
                                    float spawnDist = 100.0f;  // Distance in front of player
                                    spawnX = playState.player->getX() + std::cos(angle) * spawnDist;
                                    spawnY = playState.player->getY() + std::sin(angle) * spawnDist;
                                }

                                playState.weaponPickups.push_back(std::make_unique<WeaponPickup>(spawnX, spawnY, weaponType, false));
                                std::cout << "Spawned weapon type " << i << " at " << (playState.spawnAtCrosshair ? "crosshair" : "player") << std::endl;
                            }
                            yOffset += 25;
                        }

                        yOffset += 10;  // Zombie section spacing
                        yOffset += 20;  // After zombie title

                        // Check spawn zombie button
                        SDL_Rect spawnZombieBtn = {PANEL_X + 10, yOffset, PANEL_WIDTH - 20, 22};
                        if (mouseX >= spawnZombieBtn.x && mouseX <= spawnZombieBtn.x + spawnZombieBtn.w &&
                            mouseY >= spawnZombieBtn.y && mouseY <= spawnZombieBtn.y + spawnZombieBtn.h) {
                            float spawnX, spawnY;

                            if (playState.spawnAtCrosshair) {
                                // Calculate crosshair position
                                float angle = playState.player->getAngle();
                                float pitch = playState.player->getPitch();
                                float baseRange = 150.0f;  // Closer spawn range
                                float pitchFactor = 1.0f + pitch;
                                pitchFactor = std::max(0.3f, std::min(3.0f, pitchFactor));
                                float adjustedRange = baseRange * pitchFactor;
                                spawnX = playState.player->getX() + std::cos(angle) * adjustedRange;
                                spawnY = playState.player->getY() + std::sin(angle) * adjustedRange;
                            } else {
                                // Spawn in front of player facing direction
                                float angle = playState.player->getAngle();
                                float spawnDist = 100.0f;  // Distance in front of player
                                spawnX = playState.player->getX() + std::cos(angle) * spawnDist;
                                spawnY = playState.player->getY() + std::sin(angle) * spawnDist;
                            }

                            playState.zombies.push_back(std::make_unique<Zombie>(spawnX, spawnY, playState.zombieMaxHealth));
                            std::cout << "Spawned zombie at " << (playState.spawnAtCrosshair ? "crosshair" : "player") << std::endl;
                        }

                        yOffset += 25;  // After spawn zombie button

                        // Check spawn hunter button
                        SDL_Rect spawnHunterBtn = {PANEL_X + 10, yOffset, PANEL_WIDTH - 20, 22};
                        if (mouseX >= spawnHunterBtn.x && mouseX <= spawnHunterBtn.x + spawnHunterBtn.w &&
                            mouseY >= spawnHunterBtn.y && mouseY <= spawnHunterBtn.y + spawnHunterBtn.h) {
                            float spawnX, spawnY;

                            if (playState.spawnAtCrosshair) {
                                // Calculate crosshair position
                                float angle = playState.player->getAngle();
                                float pitch = playState.player->getPitch();
                                float baseRange = 150.0f;  // Closer spawn range
                                float pitchFactor = 1.0f + pitch;
                                pitchFactor = std::max(0.3f, std::min(3.0f, pitchFactor));
                                float adjustedRange = baseRange * pitchFactor;
                                spawnX = playState.player->getX() + std::cos(angle) * adjustedRange;
                                spawnY = playState.player->getY() + std::sin(angle) * adjustedRange;
                            } else {
                                // Spawn in front of player facing direction
                                float angle = playState.player->getAngle();
                                float spawnDist = 100.0f;  // Distance in front of player
                                spawnX = playState.player->getX() + std::cos(angle) * spawnDist;
                                spawnY = playState.player->getY() + std::sin(angle) * spawnDist;
                            }

                            // Spawn hunter (dark, fast zombie-like entity with high health)
                            playState.hunters.push_back(std::make_unique<Zombie>(spawnX, spawnY, 999));
                            std::cout << "Spawned hunter at " << (playState.spawnAtCrosshair ? "crosshair" : "player") << std::endl;
                        }

                        yOffset += 30;  // After spawn hunter button

                        // Check blood moon button
                        SDL_Rect bloodMoonBtn = {PANEL_X + 10, yOffset, PANEL_WIDTH - 20, 22};
                        if (mouseX >= bloodMoonBtn.x && mouseX <= bloodMoonBtn.x + bloodMoonBtn.w &&
                            mouseY >= bloodMoonBtn.y && mouseY <= bloodMoonBtn.y + bloodMoonBtn.h) {
                            // Trigger blood moon event
                            if (!playState.bloodMoonActive) {
                                playState.bloodMoonActive = true;
                                playState.bloodMoonTimer = 0.0f;
                                std::cout << "Blood Moon activated!" << std::endl;
                            }
                        }

                        yOffset += 30;  // After blood moon button

                        // Check blue alert button
                        SDL_Rect blueAlertBtn = {PANEL_X + 10, yOffset, PANEL_WIDTH - 20, 22};
                        if (mouseX >= blueAlertBtn.x && mouseX <= blueAlertBtn.x + blueAlertBtn.w &&
                            mouseY >= blueAlertBtn.y && mouseY <= blueAlertBtn.y + blueAlertBtn.h) {
                            // Trigger blue alert event
                            if (!playState.blueAlertActive) {
                                playState.blueAlertActive = true;
                                playState.blueAlertTimer = 0.0f;
                                playState.safeRoomLocked = false;  // Unlock the room

                                // Store safe room coordinates
                                Vec2 safePos = playState.maze->getSafeRoomPos();
                                playState.blueRoomX = (int)(safePos.x / Maze::TILE_SIZE);
                                playState.blueRoomY = (int)(safePos.y / Maze::TILE_SIZE);

                                // Kill all zombies
                                int zombiesKilled = 0;
                                for (auto& zombie : playState.zombies) {
                                    if (!zombie->isDead()) {
                                        while (!zombie->isDead()) {
                                            zombie->takeDamage();
                                        }
                                        zombiesKilled++;
                                    }
                                }

                                std::cout << "Blue Alert activated! " << zombiesKilled << " zombies eliminated!" << std::endl;
                            }
                        }
                    } else {
                        mousePressed = true;
                    }
                }
            } else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
                mousePressed = false;
            } else if (event.type == SDL_MOUSEMOTION) {
                if (menu.currentState == GameState::PLAYING && !playState.showTestingPanel) {
                    // Only rotate camera if testing panel is not open
                    float sensitivity = 0.003f;

                    // Horizontal rotation (left/right)
                    float angleChange = event.motion.xrel * sensitivity;
                    float newAngle = playState.player->getAngle() + angleChange;
                    playState.player->setAngle(newAngle);

                    // Vertical look (up/down)
                    float pitchChange = -event.motion.yrel * sensitivity;  // Negative for natural look
                    float newPitch = playState.player->getPitch() + pitchChange;
                    // Clamp pitch to prevent looking too far up/down
                    const float MAX_PITCH = M_PI / 3.0f;  // 60 degrees up/down
                    newPitch = std::max(-MAX_PITCH, std::min(MAX_PITCH, newPitch));
                    playState.player->setPitch(newPitch);
                }
            }
        }

        // Auto-respawn after 2 seconds of death
        if (menu.currentState == GameState::GAME_LOST && playState.deathTime > 0) {
            if (currentTime - playState.deathTime > 2000) {
                std::cout << "Auto-respawning with new maze..." << std::endl;
                initializeGame(playState, menu.difficulty);
                menu.currentState = GameState::PLAYING;
                SDL_SetRelativeMouseMode(SDL_TRUE);  // Lock mouse
            }
        }

        if (menu.currentState == GameState::PLAYING) {
            // Handle shooting/melee - use weapon's fire rate, not a hardcoded limit
            if (mousePressed) {
                float time = static_cast<float>(currentTime) / 1000.0f;
                WeaponStats currentWeapon = getWeaponStats(playState.player->getCurrentWeapon());

                if (currentWeapon.isMelee) {
                    // MELEE ATTACK - check for zombies in range
                    // Check if enough time has passed since last attack
                    static float lastMeleeTime = 0.0f;
                    if (time - lastMeleeTime >= currentWeapon.fireRate) {
                        lastMeleeTime = time;

                        float playerX = playState.player->getX();
                        float playerY = playState.player->getY();
                        float angle = playState.player->getAngle();

                        // Check zombies in front of player within melee range
                        for (auto& zombie : playState.zombies) {
                            if (!zombie->isDead()) {
                                float dx = zombie->getX() - playerX;
                                float dy = zombie->getY() - playerY;
                                float distance = std::sqrt(dx * dx + dy * dy);

                                // Check if zombie is within melee range
                                if (distance < currentWeapon.meleeRange) {
                                    // Check if zombie is in front of player (within 60 degree cone)
                                    float angleToZombie = std::atan2(dy, dx);
                                    float angleDiff = angleToZombie - angle;
                                    // Normalize angle diff
                                    while (angleDiff > M_PI) angleDiff -= 2.0f * M_PI;
                                    while (angleDiff < -M_PI) angleDiff += 2.0f * M_PI;

                                    if (std::abs(angleDiff) < M_PI / 6.0f) {  // 60 degree cone
                                        zombie->takeDamage(currentWeapon.damage);
                                        // Play melee sound (using shoot sound for now)
                                        Mix_PlayChannel(-1, shootSound, 0);
                                        playState.screenShake = 0.15f;
                                        break;  // Only hit one zombie per swing
                                    }
                                }
                            }
                        }
                    }
                } else {
                    // RANGED ATTACK - shoot bullets
                    float angle = playState.player->getAngle();
                    float pitch = playState.player->getPitch();

                    // Adjust bullet range based on pitch
                    // Looking up = shoot further, looking down = shoot closer
                    // This simulates aiming at where the crosshair intersects the ground
                    float baseRange = 1000.0f;
                    float pitchFactor = 1.0f + pitch;  // pitch is -PI/3 to PI/3, so factor is ~0.0 to 2.0
                    pitchFactor = std::max(0.3f, std::min(3.0f, pitchFactor));  // Clamp to reasonable range
                    float adjustedRange = baseRange * pitchFactor;

                    float targetX = playState.player->getX() + std::cos(angle) * adjustedRange;
                    float targetY = playState.player->getY() + std::sin(angle) * adjustedRange;

                    bool shotFired = playState.player->shoot(targetX, targetY, playState.bullets, time);

                    if (shotFired) {
                        Mix_PlayChannel(-1, shootSound, 0);  // Play shoot sound
                        // Add screen shake
                        playState.screenShake = 0.2f;  // Shake intensity
                    }
                }
            }

            // Update player
            const Uint8* keyState = SDL_GetKeyboardState(nullptr);
            playState.player->handleInput(keyState);
            playState.player->update(deltaTime, *playState.maze);

            // Testing mode: infinite ammo for all weapons
            if (playState.difficulty == Difficulty::TESTING) {
                for (int i = 0; i < 2; i++) {
                    WeaponStats stats = getWeaponStats(playState.player->getWeaponInSlot(i));
                    if (stats.maxAmmo > 0) {
                        playState.player->pickupAmmo(playState.player->getWeaponInSlot(i), stats.maxAmmo);
                    }
                }
            }

            // Soldier mode: infinite ammo for assault rifle
            if (playState.mazeType == MazeType::SOLDIER) {
                playState.player->pickupAmmo(WeaponType::ASSAULT_RIFLE, 999999);
            }

            // SOLDIER MODE: Wave-based spawning system
            if (playState.mazeType == MazeType::SOLDIER) {
                // Count alive zombies
                int aliveZombies = 0;
                for (const auto& zombie : playState.zombies) {
                    if (!zombie->isDead()) {
                        aliveZombies++;
                    }
                }

                // If wave is complete (all zombies dead), start delay for next wave
                if (playState.waveActive && aliveZombies == 0) {
                    playState.waveActive = false;
                    playState.waveDelayTimer = playState.waveDelay;
                    playState.currentWave++;
                    std::cout << "Wave " << (playState.currentWave - 1) << " complete! Next wave in " << playState.waveDelay << " seconds..." << std::endl;
                }

                // Update wave delay timer
                if (!playState.waveActive) {
                    playState.waveDelayTimer -= deltaTime;
                    if (playState.waveDelayTimer <= 0.0f) {
                        // Start new wave!
                        playState.waveActive = true;
                        playState.zombies.clear();  // Clear any dead zombies

                        // Calculate zombies for this wave (increases each wave)
                        int baseZombies = 5;  // Start with 5 zombies per spawn point
                        int zombiesPerSpawn = baseZombies + (playState.currentWave - 1) * 2;  // +2 per wave

                        // Spawn zombies in the 4 corner maze areas (not in walls!)
                        int arenaLeft = Maze::WIDTH / 4;
                        int arenaRight = 3 * Maze::WIDTH / 4;
                        int arenaTop = Maze::HEIGHT / 4;
                        int arenaBottom = 3 * Maze::HEIGHT / 4;

                        // Define spawn areas in each corner maze (middle of each corner maze)
                        std::vector<Vec2> spawnAreas;

                        // Top-left corner maze center
                        int tlX = (2 + arenaLeft - 1) / 2;
                        int tlY = (2 + arenaTop - 1) / 2;
                        spawnAreas.push_back({tlX * Maze::TILE_SIZE + Maze::TILE_SIZE / 2.0f,
                                             tlY * Maze::TILE_SIZE + Maze::TILE_SIZE / 2.0f});

                        // Top-right corner maze center
                        int trX = (arenaRight + 2 + Maze::WIDTH - 2) / 2;
                        int trY = (2 + arenaTop - 1) / 2;
                        spawnAreas.push_back({trX * Maze::TILE_SIZE + Maze::TILE_SIZE / 2.0f,
                                             trY * Maze::TILE_SIZE + Maze::TILE_SIZE / 2.0f});

                        // Bottom-left corner maze center
                        int blX = (2 + arenaLeft - 1) / 2;
                        int blY = (arenaBottom + 2 + Maze::HEIGHT - 2) / 2;
                        spawnAreas.push_back({blX * Maze::TILE_SIZE + Maze::TILE_SIZE / 2.0f,
                                             blY * Maze::TILE_SIZE + Maze::TILE_SIZE / 2.0f});

                        // Bottom-right corner maze center
                        int brX = (arenaRight + 2 + Maze::WIDTH - 2) / 2;
                        int brY = (arenaBottom + 2 + Maze::HEIGHT - 2) / 2;
                        spawnAreas.push_back({brX * Maze::TILE_SIZE + Maze::TILE_SIZE / 2.0f,
                                             brY * Maze::TILE_SIZE + Maze::TILE_SIZE / 2.0f});

                        // Spawn zombies at each spawn area with wider spread
                        for (const auto& spawnArea : spawnAreas) {
                            for (int i = 0; i < zombiesPerSpawn; i++) {
                                // Larger random offset within the spawn area (to spread out zombies)
                                float offsetX = (rand() % 120 - 60);
                                float offsetY = (rand() % 120 - 60);

                                // Make sure spawn position is not in a wall
                                float spawnX = spawnArea.x + offsetX;
                                float spawnY = spawnArea.y + offsetY;
                                int tileX = static_cast<int>(spawnX / Maze::TILE_SIZE);
                                int tileY = static_cast<int>(spawnY / Maze::TILE_SIZE);

                                // If spawning in wall, try center of spawn area instead
                                if (playState.maze->isWall(tileX, tileY)) {
                                    spawnX = spawnArea.x;
                                    spawnY = spawnArea.y;
                                }

                                ZombieType type = getRandomZombieType();
                                playState.zombies.push_back(
                                    std::make_unique<Zombie>(spawnX, spawnY, playState.zombieMaxHealth, type)
                                );
                            }
                        }

                        int totalSpawned = spawnAreas.size() * zombiesPerSpawn;
                        std::cout << "WAVE " << playState.currentWave << " STARTING! " << totalSpawned << " zombies incoming!" << std::endl;
                    }
                }
            }

            // Update screen shake
            if (playState.screenShake > 0.0f) {
                playState.screenShake -= deltaTime * 5.0f;  // Decay shake
                if (playState.screenShake < 0.0f) playState.screenShake = 0.0f;

                // Generate random shake offset
                playState.shakeOffsetX = ((float)rand() / RAND_MAX - 0.5f) * playState.screenShake * 10.0f;
                playState.shakeOffsetY = ((float)rand() / RAND_MAX - 0.5f) * playState.screenShake * 10.0f;
            } else {
                playState.shakeOffsetX = 0.0f;
                playState.shakeOffsetY = 0.0f;
            }

            // === BLOOD MOON EVENT SYSTEM ===
            playState.bloodMoonTimer += deltaTime;

            // Check if Blood Moon should start
            if (!playState.bloodMoonActive && playState.bloodMoonTimer >= playState.bloodMoonInterval) {
                playState.bloodMoonActive = true;
                playState.bloodMoonTimer = 0.0f;
                std::cout << "\n=== BLOOD MOON RISING! ===\n" << std::endl;
                std::cout << "The zombies grow restless..." << std::endl;
            }

            // Check if Blood Moon should end
            if (playState.bloodMoonActive && playState.bloodMoonTimer >= playState.bloodMoonDuration) {
                playState.bloodMoonActive = false;
                playState.bloodMoonTimer = 0.0f;

                // Kill half of all zombies when Blood Moon ends
                int zombiesBeforeCull = 0;
                int zombiesKilled = 0;

                for (auto& zombie : playState.zombies) {
                    if (!zombie->isDead()) {
                        zombiesBeforeCull++;
                        // 50% chance to kill each zombie
                        if (rand() % 2 == 0) {
                            zombie->takeDamage(999);  // Instant kill
                            zombiesKilled++;
                        }
                    }
                }

                std::cout << "\n=== BLOOD MOON FADES ===\n" << std::endl;
                std::cout << zombiesKilled << " / " << zombiesBeforeCull << " zombies succumb to exhaustion!" << std::endl;

                if (zombiesKilled > 0) {
                    Mix_PlayChannel(-1, zombieDeathSound, 0);
                }
            }

            // === BLUE ALERT EVENT SYSTEM (Evacuation) ===
            // Separate timer from blood moon
            static float blueEventTimer = 0.0f;

            if (!playState.blueAlertActive) {
                blueEventTimer += deltaTime;

                // Trigger blue alert every 3 minutes
                if (blueEventTimer >= playState.blueAlertInterval) {
                    playState.blueAlertActive = true;
                    playState.blueAlertTimer = 0.0f;
                    blueEventTimer = 0.0f;  // Reset for next cycle
                    playState.safeRoomLocked = false;  // Unlock the room for evacuation

                    // Store safe room coordinates
                    Vec2 safePos = playState.maze->getSafeRoomPos();
                    playState.blueRoomX = (int)(safePos.x / Maze::TILE_SIZE);
                    playState.blueRoomY = (int)(safePos.y / Maze::TILE_SIZE);

                    // KILL ALL ZOMBIES when blue alert starts
                    int zombiesKilled = 0;
                    for (auto& zombie : playState.zombies) {
                        if (!zombie->isDead()) {
                            while (!zombie->isDead()) {
                                zombie->takeDamage();
                            }
                            zombiesKilled++;
                        }
                    }

                    std::cout << "\n=== BLUE ALERT! EVACUATE TO SAFE ROOM! ===\n" << std::endl;
                    std::cout << "All " << zombiesKilled << " zombies eliminated by evacuation protocol!" << std::endl;
                    std::cout << "You have 45 seconds to reach the blue room!" << std::endl;
                }
            }

            // During blue alert
            if (playState.blueAlertActive) {
                playState.blueAlertTimer += deltaTime;

                // Check if player is in safe room
                int playerTileX = (int)(playState.player->getX() / Maze::TILE_SIZE);
                int playerTileY = (int)(playState.player->getY() / Maze::TILE_SIZE);
                playState.inSafeRoom = playState.maze->isSafeRoom(playerTileX, playerTileY);

                // Timer expired - check if player made it
                if (playState.blueAlertTimer >= playState.blueAlertDuration) {
                    // LOCK THE SAFE ROOM after timer expires
                    playState.safeRoomLocked = true;
                    std::cout << "\n=== SAFE ROOM SEALED! ===\n" << std::endl;

                    if (!playState.inSafeRoom && playState.difficulty != Difficulty::TESTING && !playState.godMode) {
                        // START HUNTER PHASE instead of instant death!
                        playState.hunterPhaseActive = true;
                        playState.hunterPhaseTimer = 0.0f;

                        // Spawn 3-5 fast hunters around the player
                        int numHunters = 3 + (rand() % 3);  // 3-5 hunters
                        for (int i = 0; i < numHunters; i++) {
                            float angle = (float)i * (M_PI * 2.0f / numHunters);
                            float spawnDist = 200.0f + (rand() % 100);  // 200-300 units away
                            float spawnX = playState.player->getX() + cos(angle) * spawnDist;
                            float spawnY = playState.player->getY() + sin(angle) * spawnDist;

                            // Make sure not spawning in wall
                            int tileX = (int)(spawnX / Maze::TILE_SIZE);
                            int tileY = (int)(spawnY / Maze::TILE_SIZE);
                            if (!playState.maze->isWall(tileX, tileY)) {
                                playState.hunters.push_back(std::make_unique<Zombie>(spawnX, spawnY, 999));
                            }
                        }

                        std::cout << "=== FAILED TO EVACUATE! ===\n" << std::endl;
                        std::cout << "=== HUNTER PHASE ACTIVATED! ===\n" << std::endl;
                        std::cout << numHunters << " dark hunters have been unleashed!" << std::endl;
                        std::cout << "Survive for 60 seconds!" << std::endl;
                    } else if (playState.inSafeRoom) {
                        std::cout << "=== EVACUATION SUCCESSFUL! ===\n" << std::endl;
                        std::cout << "You survived the blue alert! The room is now sealed." << std::endl;
                    }

                    // End the alert
                    playState.blueAlertActive = false;
                    playState.blueAlertTimer = 0.0f;
                }
            }

            // HUNTER PHASE - 60 seconds of terror!
            if (playState.hunterPhaseActive) {
                playState.hunterPhaseTimer += deltaTime;

                // Check if hunter phase is over
                if (playState.hunterPhaseTimer >= playState.hunterPhaseDuration) {
                    // Kill all hunters
                    for (auto& hunter : playState.hunters) {
                        if (!hunter->isDead()) {
                            while (!hunter->isDead()) {
                                hunter->takeDamage();
                            }
                        }
                    }
                    playState.hunters.clear();

                    playState.hunterPhaseActive = false;
                    playState.hunterPhaseTimer = 0.0f;

                    std::cout << "\n=== HUNTER PHASE ENDED! ===\n" << std::endl;
                    std::cout << "You survived! The hunters have retreated." << std::endl;
                }
            }

            // Zombie spawning system - spawn reinforcements when zombies are low (SKIP for Soldier mode)
            if (playState.mazeType != MazeType::SOLDIER) {
                playState.spawnTimer += deltaTime;
                if (playState.spawnTimer >= SPAWN_CHECK_INTERVAL) {
                    playState.spawnTimer = 0.0f;

                    // Count alive zombies
                int aliveZombies = 0;
                for (const auto& zombie : playState.zombies) {
                    if (!zombie->isDead()) {
                        aliveZombies++;
                    }
                }

                // Check if all zombies are dead - spawn new wave
                if (aliveZombies == 0) {
                    std::cout << "All zombies eliminated! Spawning new wave..." << std::endl;

                    // Clear dead zombies from the list
                    playState.zombies.clear();

                    // Spawn a fresh wave (reset spawn counter)
                    playState.totalZombiesSpawned = 0;
                    int waveSize = playState.initialZombieCount;

                    // Get spawn positions
                    Vec2 playerPos = {playState.player->getX(), playState.player->getY()};
                    auto zombiePositions = playState.maze->getRandomZombiePositions(waveSize, playerPos);

                    for (const auto& pos : zombiePositions) {
                        ZombieType type = getRandomZombieType();
                        playState.zombies.push_back(std::make_unique<Zombie>(pos.x, pos.y, playState.zombieMaxHealth, type));
                    }
                    playState.totalZombiesSpawned = waveSize;

                    std::cout << "New wave spawned! (" << waveSize << " zombies)" << std::endl;
                }
                // Spawn new zombies if below minimum and haven't hit max
                else if (aliveZombies < playState.minZombieCount && playState.totalZombiesSpawned < playState.maxZombieCount) {
                    int baseSpawn = 3;  // Base: Spawn 3 at a time

                    // During Blood Moon, spawn 3x more!
                    if (playState.bloodMoonActive) {
                        baseSpawn = static_cast<int>(baseSpawn * playState.bloodMoonSpawnMultiplier);
                    }

                    int toSpawn = std::min(baseSpawn, playState.maxZombieCount - playState.totalZombiesSpawned);

                    // Collect existing zombie positions
                    std::vector<Vec2> existingPositions;
                    for (const auto& zombie : playState.zombies) {
                        if (!zombie->isDead()) {
                            existingPositions.push_back({zombie->getX(), zombie->getY()});
                        }
                    }

                    // Spawn new zombies in areas with fewer zombies
                    for (int i = 0; i < toSpawn; i++) {
                        Vec2 spawnPos = playState.maze->getSpawnPositionAwayFromZombies(
                            existingPositions,
                            {playState.player->getX(), playState.player->getY()}
                        );
                        ZombieType type = getRandomZombieType();
                        playState.zombies.push_back(std::make_unique<Zombie>(spawnPos.x, spawnPos.y, playState.zombieMaxHealth, type));
                        existingPositions.push_back(spawnPos);  // Add to list for next spawn
                        playState.totalZombiesSpawned++;
                    }

                    std::cout << "Zombie reinforcements spawned! (" << toSpawn << " zombies)" << std::endl;
                }
            }
            }  // End of non-Soldier spawning

            // Update zombies
            for (auto& zombie : playState.zombies) {
                zombie->update(deltaTime, playState.player->getX(), playState.player->getY(), *playState.maze, &playState.zombies);

                // FREQUENTLY play CREEPY zombie groans/moans (much more common now!)
                if (!zombie->isDead() && (rand() % 150) == 0) {  // ~0.67% chance per frame = MUCH more frequent!
                    // Calculate distance to player for volume adjustment
                    float dx = zombie->getX() - playState.player->getX();
                    float dy = zombie->getY() - playState.player->getY();
                    float distance = std::sqrt(dx * dx + dy * dy);

                    // Play if zombie is within hearing range (800 units = further!)
                    if (distance < 800.0f) {
                        // Choose random sound and adjust volume based on distance
                        // Closer zombies are MUCH louder
                        float volumeFactor = (1.0f - distance / 800.0f);
                        int volume = static_cast<int>(MIX_MAX_VOLUME * volumeFactor * 1.2f);  // Louder!
                        if (volume > MIX_MAX_VOLUME) volume = MIX_MAX_VOLUME;

                        Mix_Chunk* sound = (rand() % 2 == 0) ? zombieGroanSound : zombieMoanSound;
                        if (sound) {
                            Mix_VolumeChunk(sound, volume);
                            Mix_PlayChannel(-1, sound, 0);
                        }
                    }
                }

                // Check if zombie caught player
                if (zombie->checkCollision(playState.player->getX(), playState.player->getY(), playState.player->getRadius())) {
                    // In testing mode or with god mode, player is immortal
                    if (playState.difficulty != Difficulty::TESTING && !playState.godMode) {
                        if (playState.player->takeDamage()) {
                            // Player took damage
                            std::cout << "Hit! Health: " << playState.player->getHealth() << "/" << playState.player->getMaxHealth() << std::endl;

                            // Check if player died
                            if (playState.player->isDead()) {
                                menu.currentState = GameState::GAME_LOST;
                                playState.deathTime = currentTime;

                            // Calculate survival time bonus
                            int timeSurvived = (currentTime - playState.gameStartTime) / 1000;
                            playState.score += timeSurvived;
                            playState.totalScore += playState.score;

                            Mix_PlayChannel(-1, playerDeathSound, 0);  // Play player death sound
                            std::cout << "You died! Score: " << playState.score << " | Total: " << playState.totalScore << std::endl;

                            // Save high score if applicable
                            if (isHighScore(playState.totalScore)) {
                                addHighScore(playState.totalScore, playState.currentLevel,
                                           mazeTypeToString(playState.mazeType),
                                           difficultyToString(menu.difficulty));
                                std::cout << "NEW HIGH SCORE!" << std::endl;
                            }

                            std::cout << "Respawning in 2 seconds..." << std::endl;
                        }
                    }
                    }
                }
            }

            // Update hunters (fast, dark entities)
            for (auto& hunter : playState.hunters) {
                // Store previous position in case hunter tries to enter safe room
                float prevX = hunter->getX();
                float prevY = hunter->getY();

                // Hunters move faster than zombies - pass the hunter vector instead of zombie vector
                hunter->update(deltaTime, playState.player->getX(), playState.player->getY(), *playState.maze, &playState.hunters);

                // PREVENT HUNTERS FROM ENTERING BLUE SAFE ROOM
                int hunterTileX = (int)(hunter->getX() / Maze::TILE_SIZE);
                int hunterTileY = (int)(hunter->getY() / Maze::TILE_SIZE);
                if (playState.maze->isSafeRoom(hunterTileX, hunterTileY)) {
                    // Hunter tried to enter safe room - push them back!
                    hunter->setPosition(prevX, prevY);
                }

                // Hunters make scary breathing sounds (no groans)
                if (!hunter->isDead() && (rand() % 200) == 0) {  // Less frequent than zombies
                    float dx = hunter->getX() - playState.player->getX();
                    float dy = hunter->getY() - playState.player->getY();
                    float distance = std::sqrt(dx * dx + dy * dy);

                    if (distance < 600.0f) {
                        // Play scary sound (using zombie sound for now, but quieter/different)
                        float volumeFactor = (1.0f - distance / 600.0f);
                        int volume = static_cast<int>(MIX_MAX_VOLUME * volumeFactor * 0.5f);  // Quieter
                        if (volume > MIX_MAX_VOLUME) volume = MIX_MAX_VOLUME;

                        // Use groan sound pitched differently for hunters
                        if (zombieGroanSound) {
                            Mix_VolumeChunk(zombieGroanSound, volume);
                            Mix_PlayChannel(-1, zombieGroanSound, 0);
                        }
                    }
                }

                // Check if hunter caught player
                if (hunter->checkCollision(playState.player->getX(), playState.player->getY(), playState.player->getRadius())) {
                    // Hunters damage player in same way as zombies
                    if (playState.difficulty != Difficulty::TESTING && !playState.godMode) {
                        if (playState.player->takeDamage()) {
                            std::cout << "HUNTER HIT! Health: " << playState.player->getHealth() << "/" << playState.player->getMaxHealth() << std::endl;

                            if (playState.player->isDead()) {
                                menu.currentState = GameState::GAME_LOST;
                                playState.deathTime = currentTime;

                                int timeSurvived = (currentTime - playState.gameStartTime) / 1000;
                                playState.score += timeSurvived;
                                playState.totalScore += playState.score;

                                Mix_PlayChannel(-1, playerDeathSound, 0);
                                std::cout << "Killed by hunter! Score: " << playState.score << " | Total: " << playState.totalScore << std::endl;

                                if (isHighScore(playState.totalScore)) {
                                    addHighScore(playState.totalScore, playState.currentLevel,
                                               mazeTypeToString(playState.mazeType),
                                               difficultyToString(menu.difficulty));
                                    std::cout << "NEW HIGH SCORE!" << std::endl;
                                }

                                std::cout << "Respawning in 2 seconds..." << std::endl;
                            }
                        }
                    }
                }
            }

            // Proximity beep system - beep faster as closest zombie gets nearer
            playState.proximityBeepTimer += deltaTime;

            // Find closest alive zombie
            float closestDistance = 99999.0f;
            for (const auto& zombie : playState.zombies) {
                if (!zombie->isDead()) {
                    float dx = zombie->getX() - playState.player->getX();
                    float dy = zombie->getY() - playState.player->getY();
                    float distance = std::sqrt(dx * dx + dy * dy);
                    if (distance < closestDistance) {
                        closestDistance = distance;
                    }
                }
            }

            // Calculate beep interval based on distance (closer = faster beeps)
            // Very close (< 100): 0.2s, Close (< 200): 0.5s, Medium (< 400): 1.0s, Far (< 600): 2.0s, Very far: 3.0s
            if (closestDistance < 100.0f) {
                playState.proximityBeepInterval = 0.2f;
            } else if (closestDistance < 200.0f) {
                playState.proximityBeepInterval = 0.5f;
            } else if (closestDistance < 400.0f) {
                playState.proximityBeepInterval = 1.0f;
            } else if (closestDistance < 600.0f) {
                playState.proximityBeepInterval = 2.0f;
            } else {
                playState.proximityBeepInterval = 3.0f;
            }

            // Play beep if timer exceeds interval
            if (playState.proximityBeepTimer >= playState.proximityBeepInterval && closestDistance < 600.0f) {
                playState.proximityBeepTimer = 0.0f;
                if (proximityBeepSound) {
                    // Adjust volume based on distance (closer = louder)
                    int volume = static_cast<int>(MIX_MAX_VOLUME * 0.3f * (1.0f - closestDistance / 600.0f));
                    Mix_VolumeChunk(proximityBeepSound, volume);
                    Mix_PlayChannel(-1, proximityBeepSound, 0);
                }
            }

            // Update bullets
            for (auto& bullet : playState.bullets) {
                // Store old position for collision checking along the bullet's path
                float oldX = bullet->getX();
                float oldY = bullet->getY();

                bullet->update(deltaTime, *playState.maze);

                // Check bullet-zombie collision (check along the bullet's path to avoid missing fast-moving bullets)
                if (bullet->isActive()) {
                    float newX = bullet->getX();
                    float newY = bullet->getY();

                    // Calculate distance traveled this frame
                    float dx = newX - oldX;
                    float dy = newY - oldY;
                    float distTraveled = std::sqrt(dx * dx + dy * dy);

                    // Check more points for faster bullets to ensure no misses
                    int numChecks = std::max(10, static_cast<int>(distTraveled / 5.0f));
                    bool hitSomething = false;
                    float explosionX = 0.0f;
                    float explosionY = 0.0f;

                    for (int i = 0; i <= numChecks && !hitSomething; i++) {
                        float t = static_cast<float>(i) / numChecks;
                        float checkX = oldX + (newX - oldX) * t;
                        float checkY = oldY + (newY - oldY) * t;

                        // For explosive bullets, also check if hit wall for explosion
                        if (bullet->isExplosive()) {
                            int tileX = static_cast<int>(checkX / Maze::TILE_SIZE);
                            int tileY = static_cast<int>(checkY / Maze::TILE_SIZE);
                            if (playState.maze->isWall(tileX, tileY)) {
                                hitSomething = true;
                                explosionX = checkX;
                                explosionY = checkY;
                                bullet->deactivate();
                                break;
                            }
                        }

                        for (auto& zombie : playState.zombies) {
                            if (!zombie->isDead() &&
                                zombie->checkCollision(checkX, checkY, bullet->getRadius())) {
                                hitSomething = true;
                                explosionX = checkX;
                                explosionY = checkY;

                                if (!bullet->isExplosive()) {
                                    // Regular bullet: single target damage
                                    zombie->takeDamage(bullet->getDamage());

                                    // Award points and play sound only on death
                                    if (zombie->isDead()) {
                                        playState.zombiesKilled++;
                                        playState.score += 100;  // 100 points per zombie kill
                                        Mix_PlayChannel(-1, zombieDeathSound, 0);  // Play zombie death sound
                                    }
                                }

                                bullet->deactivate();
                                break;
                            }
                        }

                        // Check hunters
                        for (auto& hunter : playState.hunters) {
                            if (!hunter->isDead() &&
                                hunter->checkCollision(checkX, checkY, bullet->getRadius())) {
                                hitSomething = true;
                                explosionX = checkX;
                                explosionY = checkY;

                                if (!bullet->isExplosive()) {
                                    // Regular bullet: single target damage
                                    hunter->takeDamage(bullet->getDamage());

                                    // Award MORE points for killing hunters (they're harder)
                                    if (hunter->isDead()) {
                                        playState.score += 500;  // 500 points per hunter kill
                                        Mix_PlayChannel(-1, zombieDeathSound, 0);  // Play death sound
                                        std::cout << "Hunter eliminated! +500 points" << std::endl;
                                    }
                                }

                                bullet->deactivate();
                                break;
                            }
                        }
                    }

                    // EXPLOSIVE DAMAGE: If this was an explosive bullet and it hit something
                    if (hitSomething && bullet->isExplosive()) {
                        float explosionRadius = bullet->getExplosionRadius();
                        int zombiesKilledInExplosion = 0;

                        // Damage ALL zombies within explosion radius
                        for (auto& zombie : playState.zombies) {
                            if (!zombie->isDead()) {
                                float dx = zombie->getX() - explosionX;
                                float dy = zombie->getY() - explosionY;
                                float distance = std::sqrt(dx * dx + dy * dy);

                                if (distance <= explosionRadius) {
                                    // Damage falls off with distance (full damage at center, 50% at edge)
                                    float damageMult = 1.0f - (distance / explosionRadius) * 0.5f;
                                    int damage = static_cast<int>(bullet->getDamage() * damageMult);
                                    zombie->takeDamage(damage);

                                    // Award points for kills
                                    if (zombie->isDead()) {
                                        zombiesKilledInExplosion++;
                                        playState.zombiesKilled++;
                                        playState.score += 100;
                                    }
                                }
                            }
                        }

                        // Damage ALL hunters within explosion radius
                        int huntersKilledInExplosion = 0;
                        for (auto& hunter : playState.hunters) {
                            if (!hunter->isDead()) {
                                float dx = hunter->getX() - explosionX;
                                float dy = hunter->getY() - explosionY;
                                float distance = std::sqrt(dx * dx + dy * dy);

                                if (distance <= explosionRadius) {
                                    float damageMult = 1.0f - (distance / explosionRadius) * 0.5f;
                                    int damage = static_cast<int>(bullet->getDamage() * damageMult);
                                    hunter->takeDamage(damage);

                                    if (hunter->isDead()) {
                                        huntersKilledInExplosion++;
                                        playState.score += 500;  // 500 points per hunter
                                    }
                                }
                            }
                        }

                        // Play explosion sound if zombies or hunters were killed
                        if (zombiesKilledInExplosion > 0 || huntersKilledInExplosion > 0) {
                            Mix_PlayChannel(-1, zombieDeathSound, 0);
                            if (zombiesKilledInExplosion > 0 && huntersKilledInExplosion > 0) {
                                std::cout << "EXPLOSION! Killed " << zombiesKilledInExplosion << " zombies and " << huntersKilledInExplosion << " hunters!" << std::endl;
                            } else if (zombiesKilledInExplosion > 0) {
                                std::cout << "EXPLOSION! Killed " << zombiesKilledInExplosion << " zombies!" << std::endl;
                            } else {
                                std::cout << "EXPLOSION! Killed " << huntersKilledInExplosion << " hunters!" << std::endl;
                            }
                        }

                        // Add screen shake for explosion
                        playState.screenShake = 0.5f;
                    }
                }
            }

            // Remove inactive bullets
            playState.bullets.erase(
                std::remove_if(playState.bullets.begin(), playState.bullets.end(),
                    [](const auto& b) { return !b->isActive(); }),
                playState.bullets.end()
            );

            // Check key collection
            for (auto& key : playState.keys) {
                if (key->checkCollision(playState.player->getX(), playState.player->getY(), playState.player->getRadius())) {
                    if (!key->isCollected()) {
                        key->collect();
                        playState.player->addKey();
                        playState.score += 250;  // 250 points per key
                        Mix_PlayChannel(-1, keySound, 0);  // Play key collection sound
                        std::cout << "Key collected! (" << playState.player->getKeys() << "/" << playState.maze->getRequiredKeyCount() << ") +250 points!" << std::endl;
                    }
                }
            }

            // Check weapon pickup (collect new weapons to spawn after loop to avoid iterator invalidation)
            std::vector<std::unique_ptr<WeaponPickup>> newWeaponsToSpawn;
            for (auto& weapon : playState.weaponPickups) {
                if (weapon->checkCollision(playState.player->getX(), playState.player->getY(), playState.player->getRadius())) {
                    if (!weapon->isCollected()) {
                        WeaponType pickedWeapon = weapon->getType();
                        WeaponStats stats = getWeaponStats(pickedWeapon);

                        if (weapon->getIsAmmo()) {
                            // This is an ammo pickup
                            playState.player->pickupAmmo(pickedWeapon, stats.ammoPerPickup);
                            weapon->collect();
                            std::cout << "Picked up ammo for: " << stats.name << " (+" << stats.ammoPerPickup << ")" << std::endl;
                        } else {
                            // This is a weapon pickup
                            playState.player->pickupWeapon(pickedWeapon);
                            weapon->collect();
                            std::cout << "Picked up: " << stats.name << " (" << stats.maxAmmo << " rounds)" << std::endl;
                        }

                        // Prepare a new random weapon pickup (spawn after loop to avoid iterator invalidation)
                        auto newPos = playState.maze->getRandomKeyPositions(1);
                        if (!newPos.empty()) {
                            // Randomly choose a weapon type (exclude pistol which everyone starts with)
                            WeaponType newWeaponType = static_cast<WeaponType>(rand() % 7);
                            if (newWeaponType == WeaponType::PISTOL) {
                                newWeaponType = WeaponType::SHOTGUN;  // Replace pistol with shotgun
                            }
                            newWeaponsToSpawn.push_back(
                                std::make_unique<WeaponPickup>(newPos[0].x, newPos[0].y, newWeaponType, false)
                            );
                        }
                    }
                }
            }
            // Now safely add new weapons after iteration is complete
            for (auto& newWeapon : newWeaponsToSpawn) {
                playState.weaponPickups.push_back(std::move(newWeapon));
            }

            // Check health boost collection (always maintain 3 in world)
            for (auto& healthBoost : playState.healthBoosts) {
                if (healthBoost->checkCollision(playState.player->getX(), playState.player->getY(), playState.player->getRadius())) {
                    if (!healthBoost->isCollected()) {
                        bool healed = false;
                        // Heal player if not at max health
                        if (playState.player->getHealth() < playState.player->getMaxHealth()) {
                            playState.player->heal(1);
                            healed = true;
                        }

                        // ALWAYS refill ammo for both weapon slots
                        for (int i = 0; i < 2; i++) {
                            WeaponType weapon = playState.player->getWeaponInSlot(i);
                            WeaponStats stats = getWeaponStats(weapon);
                            if (stats.maxAmmo > 0) {
                                playState.player->pickupAmmo(weapon, stats.maxAmmo);
                            }
                        }

                        healthBoost->collect();

                        // Respawn at new random location
                        auto newPos = playState.maze->getRandomKeyPositions(1);
                        if (!newPos.empty()) {
                            healthBoost->respawn(newPos[0].x, newPos[0].y);
                        }

                        playState.score += 50;  // 50 points for health boost
                        Mix_PlayChannel(-1, keySound, 0);  // Play collection sound
                        if (healed) {
                            std::cout << "Health boost collected! +1 HP (now " << playState.player->getHealth() << "/" << playState.player->getMaxHealth() << ") + Full Ammo +50 points!" << std::endl;
                        } else {
                            std::cout << "Health boost collected! Full Ammo +50 points!" << std::endl;
                        }
                    }
                }
            }

            // Check if current weapon is out of ammo - convert all matching pickups to ammo
            if (playState.player->isOutOfAmmo()) {
                WeaponType currentWeapon = playState.player->getCurrentWeapon();
                bool converted = false;
                for (auto& weapon : playState.weaponPickups) {
                    if (!weapon->isCollected() && weapon->getType() == currentWeapon && !weapon->getIsAmmo()) {
                        weapon->convertToAmmo();
                        converted = true;
                    }
                }
                if (converted) {
                    WeaponStats stats = getWeaponStats(currentWeapon);
                    std::cout << "Out of ammo! All " << stats.name << " pickups converted to ammo!" << std::endl;
                }
            }

            // Check if player reached exit with all keys
            int exitX = static_cast<int>(playState.player->getX() / Maze::TILE_SIZE);
            int exitY = static_cast<int>(playState.player->getY() / Maze::TILE_SIZE);
            int requiredKeys = playState.maze->getRequiredKeyCount(playState.currentLevel);
            if (playState.maze->isExit(exitX, exitY) && playState.player->getKeys() >= requiredKeys) {
                // Bonus points for completing level
                playState.score += 500;

                // Time bonus (1 point per second)
                int timeSurvived = (currentTime - playState.gameStartTime) / 1000;
                playState.score += timeSurvived;

                playState.totalScore += playState.score;

                // Check if infinite mode - if so, regenerate and continue
                if (playState.mazeType == MazeType::INFINITE) {
                    playState.currentLevel++;
                    std::cout << "Level " << (playState.currentLevel - 1) << " Complete! " << std::endl;
                    std::cout << "Score: " << playState.score << " | Total: " << playState.totalScore << std::endl;
                    std::cout << "Starting Level " << playState.currentLevel << "..." << std::endl;

                    // Save player health and weapons before regenerating
                    int playerHealth = playState.player->getHealth();
                    WeaponType weapon0 = playState.player->getWeaponInSlot(0);
                    WeaponType weapon1 = playState.player->getWeaponInSlot(1);
                    int ammo0 = playState.player->getAmmoInSlot(0);
                    int ammo1 = playState.player->getAmmoInSlot(1);
                    int weaponSlot = playState.player->getCurrentWeaponSlot();

                    // Regenerate the maze (passing true for isLevelProgression)
                    initializeGame(playState, menu.difficulty, playState.mazeType, true);

                    // Restore player state
                    for (int i = 0; i < playerHealth; i++) {
                        playState.player->heal(1);
                    }
                    // Restore weapons (this is a bit hacky but works)
                    if (weapon0 != WeaponType::SHOTGUN || weapon1 != WeaponType::SHOTGUN) {
                        playState.player->pickupWeapon(weapon0);
                        playState.player->switchWeapon();
                        playState.player->pickupWeapon(weapon1);
                        if (weaponSlot == 0) {
                            playState.player->switchWeapon();
                        }
                    }
                    // Note: ammo restoration would require more complex logic, skipping for now
                } else {
                    // Regular game modes - show win screen
                    menu.currentState = GameState::GAME_WON;
                    std::cout << "You win! Score: " << playState.score << " | Total: " << playState.totalScore << std::endl;

                    // Save high score if applicable
                    if (isHighScore(playState.totalScore)) {
                        addHighScore(playState.totalScore, playState.currentLevel,
                                   mazeTypeToString(playState.mazeType),
                                   difficultyToString(menu.difficulty));
                        std::cout << "NEW HIGH SCORE!" << std::endl;
                    }

                    std::cout << "Press R to play again or ESC to quit." << std::endl;
                }
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        if (menu.currentState == GameState::MENU) {
            renderMainMenu(renderer, menu);
        } else if (menu.currentState == GameState::MAZE_TYPE_SELECT) {
            renderMazeTypeSelect(renderer, menu);
        } else if (menu.currentState == GameState::DIFFICULTY_SELECT) {
            renderDifficultySelect(renderer, menu);
        } else if (menu.currentState == GameState::CODE_ENTRY) {
            renderCodeEntry(renderer, menu);
        } else if (menu.currentState == GameState::CONTROLS) {
            renderControlsScreen(renderer);
        } else if (menu.currentState == GameState::PLAYING || menu.currentState == GameState::PAUSED ||
                   menu.currentState == GameState::GAME_WON || menu.currentState == GameState::GAME_LOST) {
            // Render first-person view
            renderFirstPersonView(renderer, playState);

            // Guard against null player before rendering UI
            if (!playState.player) {
                SDL_RenderPresent(renderer);
                continue;
            }

            // Render UI - key count
            SDL_Rect keyUI = {10, 10, 20, 20};
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderFillRect(renderer, &keyUI);

            // Simple text representation of key count (just draw rectangles)
            for (int i = 0; i < playState.player->getKeys(); i++) {
                SDL_Rect miniKey = {35 + i * 15, 15, 10, 10};
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                SDL_RenderFillRect(renderer, &miniKey);
            }

            // Render UI - health hearts
            for (int i = 0; i < playState.player->getMaxHealth(); i++) {
                int heartX = 10 + i * 25;
                int heartY = 40;

                if (i < playState.player->getHealth()) {
                    // Full heart (red)
                    SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
                    SDL_Rect heart = {heartX, heartY, 20, 20};
                    SDL_RenderFillRect(renderer, &heart);

                    // Heart highlight
                    SDL_Rect heartHighlight = {heartX + 3, heartY + 3, 8, 8};
                    SDL_SetRenderDrawColor(renderer, 255, 150, 150, 255);
                    SDL_RenderFillRect(renderer, &heartHighlight);
                } else {
                    // Empty heart (dark)
                    SDL_SetRenderDrawColor(renderer, 80, 30, 30, 255);
                    SDL_Rect heart = {heartX, heartY, 20, 20};
                    SDL_RenderFillRect(renderer, &heart);
                }

                // Heart border
                SDL_Rect heartBorder = {heartX, heartY, 20, 20};
                SDL_SetRenderDrawColor(renderer, 150, 30, 30, 255);
                SDL_RenderDrawRect(renderer, &heartBorder);
            }

            // Flash effect when invulnerable
            if (playState.player->isInvulnerable()) {
                SDL_Rect flashRect = {5, 35, 135, 30};
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100);
                SDL_RenderFillRect(renderer, &flashRect);
            }

            // Score display (top-right corner) - toggle with 'H' key
            if (playState.showScore) {
                // SOLDIER MODE: Show wave counter instead of score
                if (playState.mazeType == MazeType::SOLDIER) {
                    SDL_Rect waveBg = {SCREEN_WIDTH - 200, 10, 190, 110};
                    SDL_SetRenderDrawColor(renderer, 60, 40, 40, 200);  // Red-tinted background
                    SDL_RenderFillRect(renderer, &waveBg);
                    SDL_SetRenderDrawColor(renderer, 150, 100, 100, 255);
                    SDL_RenderDrawRect(renderer, &waveBg);

                    // Wave number
                    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
                    renderText(renderer, "WAVE", SCREEN_WIDTH - 190, 18, 3);

                    char waveStr[32];
                    snprintf(waveStr, sizeof(waveStr), "%d", playState.currentWave);
                    SDL_SetRenderDrawColor(renderer, 255, 200, 200, 255);
                    renderText(renderer, waveStr, SCREEN_WIDTH - 100, 18, 3);

                    // Zombies alive
                    int aliveCount = 0;
                    for (const auto& zombie : playState.zombies) {
                        if (!zombie->isDead()) aliveCount++;
                    }
                    SDL_SetRenderDrawColor(renderer, 255, 150, 100, 255);
                    renderText(renderer, "ZOMBIES", SCREEN_WIDTH - 190, 58, 2);

                    snprintf(waveStr, sizeof(waveStr), "%d", aliveCount);
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    renderText(renderer, waveStr, SCREEN_WIDTH - 100, 58, 2);

                    // Wave status
                    if (!playState.waveActive && playState.waveDelayTimer > 0.0f) {
                        SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
                        renderText(renderer, "NEXT WAVE", SCREEN_WIDTH - 190, 88, 2);
                        snprintf(waveStr, sizeof(waveStr), "%.1fs", playState.waveDelayTimer);
                        renderText(renderer, waveStr, SCREEN_WIDTH - 100, 88, 2);
                    }
                } else {
                    // NORMAL MODE: Show score
                    SDL_Rect scoreBg = {SCREEN_WIDTH - 200, 10, 190, 80};
                    SDL_SetRenderDrawColor(renderer, 40, 40, 60, 200);
                    SDL_RenderFillRect(renderer, &scoreBg);
                    SDL_SetRenderDrawColor(renderer, 100, 100, 150, 255);
                    SDL_RenderDrawRect(renderer, &scoreBg);

                    // Score label and value
                    SDL_SetRenderDrawColor(renderer, 150, 200, 255, 255);
                    renderText(renderer, "SCORE", SCREEN_WIDTH - 190, 18, 2);

                    // Convert score to string and render
                    char scoreStr[32];
                    snprintf(scoreStr, sizeof(scoreStr), "%d", playState.score);
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    renderText(renderer, scoreStr, SCREEN_WIDTH - 100, 18, 2);

                    // Total score label and value
                    SDL_SetRenderDrawColor(renderer, 255, 200, 150, 255);
                    renderText(renderer, "TOTAL", SCREEN_WIDTH - 190, 48, 2);

                    snprintf(scoreStr, sizeof(scoreStr), "%d", playState.totalScore);
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    renderText(renderer, scoreStr, SCREEN_WIDTH - 100, 48, 2);
                }

                // Ammo display
                WeaponStats currentWeaponStats = getWeaponStats(playState.player->getCurrentWeapon());
                int currentAmmo = playState.player->getCurrentAmmo();
                if (currentAmmo >= 0) {  // Don't show ammo for infinite weapons
                    SDL_SetRenderDrawColor(renderer, 255, 220, 100, 255);
                    renderText(renderer, "AMMO", SCREEN_WIDTH - 190, 68, 1);
                    char ammoStr[32];
                    snprintf(ammoStr, sizeof(ammoStr), "%d", currentAmmo);
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    renderText(renderer, ammoStr, SCREEN_WIDTH - 140, 68, 2);
                }
            }

            // Dual weapon display UI at bottom center
            int bottomY = Game::SCREEN_HEIGHT - 80;
            int centerX = Game::SCREEN_WIDTH / 2;
            int boxWidth = 180;
            int boxHeight = 70;
            int boxSpacing = 10;

            // Draw both weapon slots
            for (int slot = 0; slot < 2; slot++) {
                int boxX = centerX - boxWidth - boxSpacing/2;
                if (slot == 1) {
                    boxX = centerX + boxSpacing/2;
                }

                WeaponType weaponType = playState.player->getWeaponInSlot(slot);
                WeaponStats weaponStats = getWeaponStats(weaponType);
                int ammo = playState.player->getAmmoInSlot(slot);
                bool isActive = (playState.player->getCurrentWeaponSlot() == slot);

                // Box background - highlight active weapon
                SDL_Rect weaponBox = {boxX, bottomY, boxWidth, boxHeight};
                if (isActive) {
                    SDL_SetRenderDrawColor(renderer, 80, 80, 120, 220);  // Brighter for active
                } else {
                    SDL_SetRenderDrawColor(renderer, 40, 40, 60, 180);   // Darker for inactive
                }
                SDL_RenderFillRect(renderer, &weaponBox);

                // Border - thicker for active weapon
                if (isActive) {
                    SDL_SetRenderDrawColor(renderer, 150, 200, 255, 255);
                    SDL_RenderDrawRect(renderer, &weaponBox);
                    SDL_Rect innerBorder = {boxX - 2, bottomY - 2, boxWidth + 4, boxHeight + 4};
                    SDL_RenderDrawRect(renderer, &innerBorder);
                } else {
                    SDL_SetRenderDrawColor(renderer, 100, 100, 150, 255);
                    SDL_RenderDrawRect(renderer, &weaponBox);
                }

                // Slot number
                char slotText[8];
                snprintf(slotText, sizeof(slotText), "[%d]", slot + 1);
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                renderText(renderer, slotText, boxX + 5, bottomY + 5, 2);

                // Weapon name
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                renderText(renderer, weaponStats.name, boxX + 5, bottomY + 25, 1);

                // Ammo display
                if (ammo >= 0) {  // Has limited ammo
                    char ammoText[32];
                    snprintf(ammoText, sizeof(ammoText), "AMMO: %d", ammo);
                    if (ammo == 0) {
                        SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);  // Red when empty
                    } else if (ammo < weaponStats.maxAmmo / 4) {
                        SDL_SetRenderDrawColor(renderer, 255, 200, 100, 255);  // Orange when low
                    } else {
                        SDL_SetRenderDrawColor(renderer, 150, 255, 150, 255);  // Green when good
                    }
                    renderText(renderer, ammoText, boxX + 5, bottomY + 50, 1);
                } else {
                    // Infinite ammo indicator
                    SDL_SetRenderDrawColor(renderer, 100, 255, 255, 255);
                    renderText(renderer, "INFINITE", boxX + 5, bottomY + 50, 1);
                }
            }

            // Display win/lose messages (simple visual feedback)
            if (menu.currentState == GameState::GAME_WON) {
                SDL_Rect winRect = {SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 50, 200, 100};
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 200);
                SDL_RenderFillRect(renderer, &winRect);

                // Display "R to restart" hint
                SDL_Rect hintRect = {SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 + 60, 160, 30};
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 150);
                SDL_RenderFillRect(renderer, &hintRect);
            }

            if (menu.currentState == GameState::GAME_LOST) {
                SDL_Rect loseRect = {SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 50, 200, 100};
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 200);
                SDL_RenderFillRect(renderer, &loseRect);

                // Show countdown or respawn indicator
                if (playState.deathTime > 0) {
                    int secondsLeft = 2 - ((currentTime - playState.deathTime) / 1000);
                    // Show a small indicator for respawn countdown
                    for (int i = 0; i < secondsLeft; i++) {
                        SDL_Rect dotRect = {SCREEN_WIDTH/2 - 15 + i * 15, SCREEN_HEIGHT/2 + 60, 10, 10};
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
                        SDL_RenderFillRect(renderer, &dotRect);
                    }
                }
            }

            // Render minimap - toggle with 'M' or 'H' key
            if (playState.showMinimap) {
                renderMinimap(renderer, playState);
            }

            // Zombie direction arrow REMOVED - too easy, player must rely on sound and sight!

            // Render scary vignette effect (darkens edges of screen)
            const int vignetteSize = 150;
            const int screenW = SCREEN_WIDTH;
            const int screenH = SCREEN_HEIGHT;

            // Top vignette
            for (int i = 0; i < vignetteSize; i++) {
                int alpha = static_cast<int>(120.0f * (1.0f - (float)i / vignetteSize));
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
                SDL_RenderDrawLine(renderer, 0, i, screenW, i);
            }

            // Bottom vignette
            for (int i = 0; i < vignetteSize; i++) {
                int alpha = static_cast<int>(120.0f * (1.0f - (float)i / vignetteSize));
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
                SDL_RenderDrawLine(renderer, 0, screenH - i - 1, screenW, screenH - i - 1);
            }

            // Left vignette
            for (int i = 0; i < vignetteSize; i++) {
                int alpha = static_cast<int>(100.0f * (1.0f - (float)i / vignetteSize));
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
                SDL_RenderDrawLine(renderer, i, 0, i, screenH);
            }

            // Right vignette
            for (int i = 0; i < vignetteSize; i++) {
                int alpha = static_cast<int>(100.0f * (1.0f - (float)i / vignetteSize));
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, alpha);
                SDL_RenderDrawLine(renderer, screenW - i - 1, 0, screenW - i - 1, screenH);
            }

            // === BLOOD MOON RED OVERLAY ===
            if (menu.currentState == GameState::PLAYING && playState.bloodMoonActive) {
                // Pulsing red overlay effect
                float pulseIntensity = 0.5f + 0.3f * std::sin(playState.bloodMoonTimer * 3.0f);  // Pulse between 0.2 and 0.8
                int redAlpha = static_cast<int>(80 * pulseIntensity);  // Alpha between 16 and 64

                SDL_Rect fullScreen = {0, 0, screenW, screenH};
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 180, 0, 0, redAlpha);  // Dark red tint
                SDL_RenderFillRect(renderer, &fullScreen);

                // Blood Moon warning text
                std::string warningText = "BLOOD MOON ACTIVE";
                int remainingTime = static_cast<int>(playState.bloodMoonDuration - playState.bloodMoonTimer);
                std::string timeText = std::to_string(remainingTime) + "s";

                // Draw warning at top of screen
                SDL_Rect warningBg = {screenW/2 - 120, 10, 240, 30};
                SDL_SetRenderDrawColor(renderer, 100, 0, 0, 200);
                SDL_RenderFillRect(renderer, &warningBg);
                SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
                SDL_RenderDrawRect(renderer, &warningBg);
            }

            // === BLUE ALERT OVERLAY ===
            if (menu.currentState == GameState::PLAYING && playState.blueAlertActive) {
                // Pulsing blue overlay effect
                float pulseIntensity = 0.5f + 0.3f * std::sin(playState.blueAlertTimer * 5.0f);
                int blueAlpha = static_cast<int>(60 * pulseIntensity);

                SDL_Rect fullScreen = {0, 0, screenW, screenH};
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 100, 200, blueAlpha);
                SDL_RenderFillRect(renderer, &fullScreen);

                // Warning text
                int remainingTime = static_cast<int>(playState.blueAlertDuration - playState.blueAlertTimer);

                // Draw warning at top of screen
                SDL_Rect warningBg = {screenW/2 - 150, 50, 300, 80};
                SDL_SetRenderDrawColor(renderer, 0, 50, 150, 220);
                SDL_RenderFillRect(renderer, &warningBg);
                SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
                SDL_RenderDrawRect(renderer, &warningBg);

                // Render warning text
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                renderText(renderer, "BLUE ALERT", screenW/2 - 60, 58, 3);
                renderText(renderer, "EVACUATE TO SAFE ROOM", screenW/2 - 126, 85, 2);

                // Timer
                std::string timeStr = std::to_string(remainingTime) + "s";
                renderText(renderer, timeStr.c_str(), screenW/2 - 12, 108, 2);

                // Arrow pointing to safe room (if not in it)
                if (!playState.inSafeRoom) {
                    Vec2 safePos = playState.maze->getSafeRoomPos();
                    float dx = safePos.x - playState.player->getX();
                    float dy = safePos.y - playState.player->getY();
                    float angleToRoom = std::atan2(dy, dx);

                    // Draw arrow at center top of screen
                    int arrowCenterX = screenW / 2;
                    int arrowCenterY = 150;
                    int arrowLength = 40;
                    int arrowEndX = arrowCenterX + (int)(std::cos(angleToRoom) * arrowLength);
                    int arrowEndY = arrowCenterY + (int)(std::sin(angleToRoom) * arrowLength);

                    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
                    SDL_RenderDrawLine(renderer, arrowCenterX, arrowCenterY, arrowEndX, arrowEndY);

                    // Arrow head
                    int arrow1X = arrowEndX + (int)(std::cos(angleToRoom - 2.5f) * 10);
                    int arrow1Y = arrowEndY + (int)(std::sin(angleToRoom - 2.5f) * 10);
                    int arrow2X = arrowEndX + (int)(std::cos(angleToRoom + 2.5f) * 10);
                    int arrow2Y = arrowEndY + (int)(std::sin(angleToRoom + 2.5f) * 10);
                    SDL_RenderDrawLine(renderer, arrowEndX, arrowEndY, arrow1X, arrow1Y);
                    SDL_RenderDrawLine(renderer, arrowEndX, arrowEndY, arrow2X, arrow2Y);
                } else {
                    // Show "SAFE" indicator
                    SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
                    renderText(renderer, "SAFE", screenW/2 - 24, 150, 3);
                }
            }

            // === HUNTER PHASE OVERLAY ===
            if (menu.currentState == GameState::PLAYING && playState.hunterPhaseActive) {
                // Pulsing dark red overlay effect (more intense than blood moon)
                float pulseIntensity = 0.6f + 0.4f * std::sin(playState.hunterPhaseTimer * 6.0f);
                int darkAlpha = static_cast<int>(80 * pulseIntensity);

                SDL_Rect fullScreen = {0, 0, screenW, screenH};
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 100, 0, 0, darkAlpha);  // Dark red
                SDL_RenderFillRect(renderer, &fullScreen);

                // Warning text
                int remainingTime = static_cast<int>(playState.hunterPhaseDuration - playState.hunterPhaseTimer);

                // Draw warning at top of screen with dark/scary theme
                SDL_Rect warningBg = {screenW/2 - 180, 50, 360, 100};
                SDL_SetRenderDrawColor(renderer, 60, 0, 0, 240);  // Very dark red, almost black
                SDL_RenderFillRect(renderer, &warningBg);
                SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);  // Red border
                SDL_RenderDrawRect(renderer, &warningBg);

                // Render scary warning text
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                renderText(renderer, "!!! HUNTER PHASE !!!", screenW/2 - 132, 58, 3);
                renderText(renderer, "SURVIVE", screenW/2 - 45, 85, 2);

                // Countdown timer (large and scary)
                std::string timeStr = std::to_string(remainingTime) + "s";
                int textWidth = timeStr.length() * 6;  // Approximate width
                renderText(renderer, timeStr.c_str(), screenW/2 - textWidth, 110, 3);

                // Render active hunter count
                int aliveHunters = 0;
                for (const auto& hunter : playState.hunters) {
                    if (!hunter->isDead()) {
                        aliveHunters++;
                    }
                }
                std::string hunterStr = std::to_string(aliveHunters) + " HUNTERS";
                int hunterTextWidth = hunterStr.length() * 4;
                SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);  // Light red
                renderText(renderer, hunterStr.c_str(), screenW/2 - hunterTextWidth, 135, 2);
            }

            // Render testing panel (F1 to toggle, only in TESTING mode)
            if (playState.showTestingPanel && playState.difficulty == Difficulty::TESTING) {
                renderTestingPanel(renderer, playState);
            }

            // Render pause menu overlay
            if (menu.currentState == GameState::PAUSED) {
                renderPauseMenu(renderer, menu);
            }
        }

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    cleanupSounds();
    Mix_CloseAudio();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
