# Zombie Shooter - Implementation Status

## ✅ COMPLETED (Ready to Use)

### 1. Testing Panel (F1 Key)
- **Location**: `zombie/game.cpp` lines 2530-2633, 2984-2996, 3045-3110
- **Features**:
  - Press F1 in TESTING mode to open/close panel
  - Mouse automatically unlocks when panel opens
  - God Mode checkbox - click to toggle invulnerability
  - 7 Weapon spawn buttons - click to spawn weapons near player
  - Spawn Zombie button - click to spawn zombie
  - Blood Moon button - click to trigger red event
- **Fixed**: Button coordinates now match rendering positions exactly

### 2. Blue Safe Room
- **Location**: `zombie/maze.h` (SafeRoom tile type), `zombie/maze.cpp` lines 86-110, 474-499
- **Features**:
  - 3x3 glowing blue room generated in every maze
  - Located away from start and exit
  - Bright blue color with glow effect
  - Position stored in `maze->getSafeRoomPos()`
- **Status**: Room exists in maze, visible on map

### 3. Blue Event Data Structures
- **Location**: `zombie/game.cpp` lines 346-352
- **Variables Added**:
  - `blueAlertTimer` - tracks time since last alert
  - `blueAlertActive` - whether alert is currently active
  - `blueAlertDuration` - 45 seconds to reach safe room
  - `blueAlertInterval` - triggers every 3 minutes
  - `blueRoomX`, `blueRoomY` - safe room coordinates
  - `inSafeRoom` - tracks if player is in safe room

## ⚠️ TODO (Not Yet Implemented)

### HIGH PRIORITY (Core Issues)

#### 1. Blue Event Logic (CRITICAL)
**What's needed**:
- Trigger blue alert event every 3 minutes
- Start countdown timer (45 seconds)
- Check if player is in safe room during alert
- Kill player if not in safe room when timer expires
- End alert when timer expires or player reaches safety

**Where to add**: In `zombie/game.cpp` around line 3355 (near blood moon update logic)
```cpp
// Blue Alert event timing
if (!playState.blueAlertActive) {
    playState.blueAlertTimer += deltaTime;
    if (playState.blueAlertTimer >= playState.blueAlertInterval) {
        playState.blueAlertActive = true;
        playState.blueAlertTimer = 0.0f;
        // Store safe room coords from maze
        Vec2 safePos = playState.maze->getSafeRoomPos();
        playState.blueRoomX = (int)(safePos.x / Maze::TILE_SIZE);
        playState.blueRoomY = (int)(safePos.y / Maze::TILE_SIZE);
        std::cout << "BLUE ALERT! Evacuate to safe room!" << std::endl;
    }
}

// During blue alert
if (playState.blueAlertActive) {
    playState.blueAlertTimer += deltaTime;

    // Check if player is in safe room
    int playerTileX = (int)(playState.player->getX() / Maze::TILE_SIZE);
    int playerTileY = (int)(playState.player->getY() / Maze::TILE_SIZE);
    playState.inSafeRoom = playState.maze->isSafeRoom(playerTileX, playerTileY);

    // Timer expired
    if (playState.blueAlertTimer >= playState.blueAlertDuration) {
        if (!playState.inSafeRoom && playState.difficulty != Difficulty::TESTING && !playState.godMode) {
            // Kill player
            while (!playState.player->isDead()) {
                playState.player->takeDamage();
            }
            menu.currentState = GameState::GAME_LOST;
            playState.deathTime = currentTime;
            std::cout << "Failed to reach safe room!" << std::endl;
        }
        playState.blueAlertActive = false;
        playState.blueAlertTimer = 0.0f;
    }
}
```

#### 2. Blue Event UI (CRITICAL)
**What's needed**:
- Show "BLUE ALERT - EVACUATE!" warning at top of screen
- Display countdown timer
- Show arrow pointing to safe room direction
- Add blue screen overlay (similar to red for blood moon)

**Where to add**: In `zombie/game.cpp` around line 4058 (after blood moon overlay)
```cpp
// === BLUE ALERT OVERLAY ===
if (menu.currentState == GameState::PLAYING && playState.blueAlertActive) {
    // Pulsing blue overlay
    float pulseIntensity = 0.5f + 0.3f * std::sin(playState.blueAlertTimer * 5.0f);
    int blueAlpha = static_cast<int>(60 * pulseIntensity);

    SDL_Rect fullScreen = {0, 0, screenW, screenH};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 100, 200, blueAlpha);
    SDL_RenderFillRect(renderer, &fullScreen);

    // Warning text
    int remainingTime = static_cast<int>(playState.blueAlertDuration - playState.blueAlertTimer);
    SDL_Rect warningBg = {screenW/2 - 150, 50, 300, 35};
    SDL_SetRenderDrawColor(renderer, 0, 50, 150, 220);
    SDL_RenderFillRect(renderer, &warningBg);
    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
    SDL_RenderDrawRect(renderer, &warningBg);

    // Render warning text and timer
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    renderText(renderer, "BLUE ALERT  EVACUATE", screenW/2 - 120, 58, 2);

    std::string timeStr = std::to_string(remainingTime) + "s";
    renderText(renderer, timeStr.c_str(), screenW/2 - 12, 75, 2);

    // Arrow pointing to safe room (if not in it)
    if (!playState.inSafeRoom) {
        Vec2 safePos = playState.maze->getSafeRoomPos();
        float dx = safePos.x - playState.player->getX();
        float dy = safePos.y - playState.player->getY();
        float angleToRoom = std::atan2(dy, dx);

        // Draw arrow at top of screen
        int arrowCenterX = screenW / 2;
        int arrowCenterY = 120;
        int arrowLength = 30;
        int arrowEndX = arrowCenterX + (int)(std::cos(angleToRoom) * arrowLength);
        int arrowEndY = arrowCenterY + (int)(std::sin(angleToRoom) * arrowLength);

        SDL_SetRenderDrawColor(renderer, 100, 200, 255, 255);
        SDL_RenderDrawLine(renderer, arrowCenterX, arrowCenterY, arrowEndX, arrowEndY);

        // Arrow head
        float perpAngle = angleToRoom + M_PI / 2.0f;
        int arrow1X = arrowEndX + (int)(std::cos(angleToRoom - 2.5f) * 10);
        int arrow1Y = arrowEndY + (int)(std::sin(angleToRoom - 2.5f) * 10);
        int arrow2X = arrowEndX + (int)(std::cos(angleToRoom + 2.5f) * 10);
        int arrow2Y = arrowEndY + (int)(std::sin(angleToRoom + 2.5f) * 10);
        SDL_RenderDrawLine(renderer, arrowEndX, arrowEndY, arrow1X, arrow1Y);
        SDL_RenderDrawLine(renderer, arrowEndX, arrowEndY, arrow2X, arrow2X);
    } else {
        // Show "SAFE" indicator
        SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
        renderText(renderer, "SAFE", screenW/2 - 24, 100, 3);
    }
}
```

#### 3. Increase Red Event Zombie Spawning (HIGH PRIORITY)
**Issue**: User reports not enough zombies spawn during blood moon
**Current**: `bloodMoonSpawnMultiplier = 3.0f` (line 344)
**Fix**: Change to higher value like 5.0f or 8.0f

**Where to change**: `zombie/game.cpp` line 344
```cpp
float bloodMoonSpawnMultiplier = 8.0f;  // Spawn 8x more zombies during blood moon
```

Also check zombie spawn logic around line 3360-3380 to ensure multiplier is being applied correctly.

#### 4. Debug Panel Button Clicks
**Issue**: User reports buttons don't work
**Possible causes**:
1. Coordinate mismatch (should be fixed now)
2. Mouse not being captured properly
3. Panel not detecting it's in TESTING mode

**Test steps**:
1. Start game in TESTING difficulty
2. Press F1 to open panel
3. Try clicking god mode checkbox - should print "God Mode: ON/OFF" to console
4. Try clicking weapon buttons - should print "Spawned weapon type X" to console

**If still not working**, add debug output in click handler (line 3045):
```cpp
std::cout << "Mouse click at: " << mouseX << ", " << mouseY << std::endl;
std::cout << "Panel X: " << PANEL_X << ", Panel Y: " << PANEL_Y << std::endl;
```

### MEDIUM PRIORITY

#### 5. Zombie Beeping (Only When Fast Route)
**What's needed**:
- Only play beep sound when zombie has a short/direct path to player
- Check path length from zombie to player
- Only beep if path < some threshold (e.g., 10 tiles)

**Where to add**: In zombie update logic where beeping occurs

#### 6. Textured Floor
**What's needed**:
- Add checkerboard pattern to floor rendering
- Maybe brick/stone texture effect

#### 7. Make Walls Taller
**What's needed**:
- Adjust wall height multiplier in first-person rendering

### LOW PRIORITY

#### 8. Hunter Entity with A* Pathfinding
- New enemy type that hunts player intelligently
- Immortal, just chases player
- Uses A* algorithm for smart pathfinding

#### 9. Weapon Pickup Menu
- UI to choose weapon before picking up
- Replace current automatic pickup

#### 10. Multiplayer with Join Codes
- Network code for multiplayer
- Join code system
- Synchronization

## 🔍 DEBUGGING TIPS

### If Testing Panel Doesn't Work:
1. Make sure you're in TESTING difficulty mode
2. Check console output when pressing F1
3. Check console output when clicking buttons
4. Verify mouse is unlocked (cursor visible)

### If Blue Room Doesn't Show:
1. Enable minimap (M key)
2. Look for blue square on minimap
3. Room should be away from start (top-left) and exit (bottom-right)

### If Events Don't Trigger:
1. Check console for event messages
2. Blood Moon should trigger every 2 minutes
3. Blue Alert should trigger every 3 minutes (once implemented)

## 📝 BUILD INSTRUCTIONS

```bash
cd /Users/brantpeabody/work/game
bazel build //:zombie_shooter
./bazel-bin/zombie_shooter
```

## 🎮 CONTROLS

- **WASD / Arrows**: Move
- **Mouse**: Look around
- **Left Click**: Shoot
- **E**: Pick up items
- **Q**: Switch weapon slots
- **1**: Switch to melee weapon
- **M**: Toggle minimap
- **H**: Toggle HUD
- **ESC**: Pause
- **F1**: Testing panel (TESTING mode only)
