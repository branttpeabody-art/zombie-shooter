#!/bin/bash
# Build script for compiling the zombie shooter game to WebAssembly using Emscripten

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
    echo "Error: emcc not found. Please install Emscripten SDK."
    echo "Visit: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

echo "Building Zombie Shooter for Web..."

# Compile all source files
emcc \
    zombie/main.cpp \
    zombie/game.cpp \
    zombie/player.cpp \
    zombie/zombie.cpp \
    zombie/bullet.cpp \
    zombie/maze.cpp \
    zombie/key.cpp \
    zombie/weapon.cpp \
    zombie/healthboost.cpp \
    -o zombie_shooter.html \
    -s USE_SDL=2 \
    -s USE_SDL_MIXER=2 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s TOTAL_MEMORY=67108864 \
    -s EXPORTED_RUNTIME_METHODS='["ccall"]' \
    -s EXPORTED_FUNCTIONS='["_main"]' \
    --shell-file zombie_shell.html \
    -O2 \
    -std=c++17

echo "Build complete! Open zombie_shooter.html in a web browser."
