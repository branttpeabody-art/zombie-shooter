# Zombie Maze Shooter

A top-down zombie shooter game with **3D-style graphics**, **humanoid characters**, **smart AI**, and a **score system**!

## Features

- **🎨 3D-Style Graphics** - Walls, entities, and objects rendered with depth, shadows, and highlights!
- **👤 Human-like Player** - Detailed character with head, body, arms, legs, and a visible gun!
- **🧟 Zombie Enemies** - Menacing zombies with outstretched arms, glowing eyes, and decayed appearance!
- **🧠 Smart AI** - A* pathfinding allows zombies to navigate around obstacles and hunt you down!
- **🎲 Randomly generated maze** - Every death creates a new unique maze layout!
- **🏆 Score System** - Track your kills, keys, and survival time!
- **📍 Random spawn** - Player spawns in a random safe location each game
- **🚪 One-Way Doors** - Strategic corridor doors that only allow passage in one direction!
- **🔫 Visible Weapon** - Player character visibly holds a gun
- **⚡ Larger Mazes** - Bigger 26x20 maze with more zombies (12 total!)
- **🎮 Enhanced Challenge** - Navigate one-way corridors while avoiding more zombies
- Grid-based maze with walls and corridors
- Top-down shooter mechanics with smooth controls
- Intelligent zombie AI that paths around walls to reach you
- Key collection system with visual feedback
- Exit door that requires all keys to escape
- Mouse-based shooting with bullet physics
- Collision detection for bullets, zombies, and walls
- **Auto-respawn system** - Automatically respawn after death with a new maze
- Manual respawn option with R key
- Persistent total score across lives

## Objective

1. Collect all **3 yellow glowing keys** scattered throughout the maze (+250 points each)
2. Shoot zombies with your mouse to clear the path (+100 points each)
3. Navigate to the **green exit door** in the bottom-right corner
4. Survive as long as possible for time bonus (+1 point per second)
5. Reach the exit with all keys for a 500 point bonus!
6. Don't let zombies catch you or you'll respawn in a new maze!

## Controls

- **WASD** or **Arrow Keys** - Move in four directions
- **Left Mouse Button** - Shoot (aims at mouse cursor)
- **R** - Respawn immediately (after death) or restart (after winning)
- **ESC** - Quit game

## Gameplay

You control a **human survivor** with detailed character design (head, body, arms, legs, and a visible gun) navigating through a large procedurally generated maze filled with **12 horrifying zombie enemies** featuring outstretched arms, glowing eyes, and decayed flesh! The zombies use **A* pathfinding AI** to intelligently hunt you down, navigating around walls and obstacles to reach you. Click the mouse to shoot white bullets in the direction of your cursor. Kill zombies to clear your path, collect all the glowing yellow keys, and reach the green exit door to win!

**One-Way Doors**: Navigate strategically placed one-way doors in corridors! These orange doors with directional arrows only allow passage in one direction. Plan your route carefully - going through the wrong door might force you to take a longer path!

**Character Design:**
- **Player**: Human figure with blue clothing, skin-tone face, detailed limbs
- **Zombies**: Greenish decayed skin, hunched posture, outstretched arms, glowing eyes with red pupils, gaping mouths

**3D Visual Effects:**
- Walls have depth with lighting and shadows
- All entities cast realistic shadows on the ground
- Highlights and gradients create a pseudo-3D appearance
- Glowing effects on keys and exit door

**AI System:**
- Zombies use A* pathfinding to find optimal routes to the player
- Navigate around obstacles and through corridors intelligently
- Path recalculates every 0.5 seconds to adapt to player movement
- Can move diagonally for smoother, more realistic movement

**Score System:**
- **Zombie Kill**: +100 points
- **Key Collection**: +250 points
- **Level Complete**: +500 points
- **Time Bonus**: +1 point per second survived
- **Total Score**: Accumulates across all lives!

When you die, your current life score is added to your total score, and the game automatically respawns you after 2 seconds with a **completely new randomly generated maze** and a **random spawn location**, giving you a fresh challenge every time! You can also press **R** to respawn immediately.

## Building

```bash
# Build the zombie shooter
bazel build //:zombie_shooter
```

## Running

```bash
# Run the game
./bazel-bin/zombie_shooter
```

## Game Mechanics

- **Player**: Human survivor with detailed anatomy (head, torso, arms, legs, visible gun), blue clothing, spawns randomly in maze, moves at moderate speed
- **Zombies**: Decayed humanoid enemies (12 total) with greenish skin, outstretched arms, glowing eyes, use A* pathfinding AI to intelligently pursue player around obstacles, spawn in random locations each game
- **Bullets**: White projectiles, destroy zombies on contact
- **Keys**: Yellow glowing pickups (3 required), spawn in random locations each game
- **Exit**: Green glowing tile with bright inner light, only accessible with all 3 keys
- **One-Way Doors**: Orange/brown corridor doors with directional arrows (3-6 per maze), allow passage in one direction only, placed strategically in corridors to prevent trapping keys or exit, zombies respect door directions in pathfinding
- **Walls**: Gray 3D blocks with highlights and shadows, block movement for all entities
- **Floor**: Dark grid pattern for depth perception
- **Respawn**: Auto-respawn after 2 seconds on death, or press R to respawn immediately
- **Maze Generation**: Uses recursive backtracking to create unique 26x20 maze layouts every game
- **Random Spawn**: Player spawns in a random empty tile far from the exit

## Score Breakdown

**Points Earned:**
- Zombie kill: **100 points**
- Key collected: **250 points**
- Level complete: **500 points**
- Survival time: **1 point per second**

**Example Score Calculation:**
- Kill 5 zombies: 500 points
- Collect 3 keys: 750 points
- Survive 60 seconds: 60 points
- Complete level: 500 points
- **Total**: 1810 points

## Tips

- Keep moving! 12 zombies will surround you if you stay still
- Use walls to create distance from zombies and control their approach
- **Watch for one-way doors!** Orange doors with arrows show which direction you can pass through
- **Plan your route carefully** - going through a one-way door might require finding an alternate path back
- One-way doors are always in corridors, so they won't trap you in dead ends
- Collect keys strategically - plan your route through the maze considering door directions
- The exit won't open until you have all 3 keys
- Don't worry about dying - you'll get a new maze to try!
- Learn the maze generation patterns to find keys and exit faster
- Press R if you want to skip the death countdown and start fresh immediately
- Try to kill all 12 zombies for maximum score (1200 points!) before exiting
- Survive longer for more time bonus points
- Your total score persists across all lives - aim for a high total!
- The larger maze means more room to maneuver but also more zombies to avoid

## Technical Details

- **Graphics**: Pseudo-3D rendering with shadows, highlights, and depth effects; detailed humanoid character designs with visible weapon
- **AI**: A* pathfinding algorithm with 8-directional movement, 0.5s recalculation interval for dynamic player tracking, respects one-way door constraints
- **Maze Algorithm**: Recursive backtracking for guaranteed solvable mazes, 26x20 tile grid (780x600 pixels)
- **One-Way Door System**: Doors placed only in corridors with exactly 2 walls on opposite sides, ensures game winnability by preventing trapped rooms
- **Random Generation**: New maze, key positions, zombie spawns (12 total), door placements (3-6), and player spawn on each death/respawn
- **Respawn System**: 2-second countdown with visual indicator, or instant with R key
- **Score System**: Current life score + persistent total score with time, kill, and completion bonuses
- **Character Rendering**: Multi-part sprite system for player (head, body, arms, legs, gun) and zombies (detailed anatomy with decay effects)
- **Collision Detection**: Tile-based collision for navigation with directional door checking, circle-based collision for entities
- **C++20**: Modern C++ with smart pointers, lambdas, structured bindings, priority queues, and RAII patterns
- **SDL2**: Hardware-accelerated rendering with VSync
- **Build System**: Bazel for reproducible builds
