cmake_minimum_required(VERSION 3.12)

project(gp25_exercises)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(ASSIMP_WARNINGS_AS_ERRORS OFF)

FILE(COPY data DESTINATION ${CMAKE_BINARY_DIR})

set(SDLMIXER_VENDORED OFF)

# 3rd party libraries
add_subdirectory(lib/SDL)
add_subdirectory(lib/SDL_mixer)
add_subdirectory(lib/SDL_ttf)
add_subdirectory(lib/box2d)
add_subdirectory(lib/imgui)
include_directories(lib/stb)
add_subdirectory(lib/assimp)

# repo's executables
add_subdirectory(examples)
add_subdirectory(playground)
add_subdirectory(exercises)
add_subdirectory(exercises_solutions)
