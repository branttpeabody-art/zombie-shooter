cc_binary(
    name = "platformer",
    srcs = [
        "src/main.cpp",
        "src/game.cpp",
        "src/game.h",
        "src/player.cpp",
        "src/player.h",
        "src/platform.cpp",
        "src/platform.h",
    ],
    deps = [
        "@sdl2//:SDL2",
    ],
    copts = ["-Iinclude"],
)

cc_binary(
    name = "zombie_shooter",
    srcs = [
        "zombie/main.cpp",
        "zombie/game.cpp",
        "zombie/game.h",
        "zombie/player.cpp",
        "zombie/player.h",
        "zombie/zombie.cpp",
        "zombie/zombie.h",
        "zombie/bullet.cpp",
        "zombie/bullet.h",
        "zombie/maze.cpp",
        "zombie/maze.h",
        "zombie/key.cpp",
        "zombie/key.h",
        "zombie/weapon.cpp",
        "zombie/weapon.h",
        "zombie/healthboost.cpp",
        "zombie/healthboost.h",
    ],
    deps = [
        "@sdl2//:SDL2",
        "@sdl2_mixer//:SDL2_mixer",
    ],
)
