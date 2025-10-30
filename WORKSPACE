workspace(name = "platformer_game")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# SDL2
new_local_repository(
    name = "sdl2",
    path = "/opt/homebrew/opt/sdl2",
    build_file_content = """
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "SDL2",
    hdrs = glob(["include/SDL2/*.h"]),
    includes = ["include/SDL2"],
    linkopts = ["-L/opt/homebrew/opt/sdl2/lib", "-lSDL2"],
)
""",
)

# SDL2_mixer
new_local_repository(
    name = "sdl2_mixer",
    path = "/opt/homebrew/opt/sdl2_mixer",
    build_file_content = """
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "SDL2_mixer",
    hdrs = glob(["include/SDL2/*.h"]),
    includes = ["include/SDL2"],
    linkopts = ["-L/opt/homebrew/opt/sdl2_mixer/lib", "-lSDL2_mixer"],
)
""",
)
