cmake_minimum_required(VERSION 3.20)
project(nds_emu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Prefer a system SDL2 install when available; otherwise pull it from GitHub.
find_package(SDL2 CONFIG QUIET)
if(NOT SDL2_FOUND)
    find_package(SDL2 MODULE QUIET)
endif()

if(NOT SDL2_FOUND)
    message(STATUS "SDL2 not found on system; fetching via FetchContent...")
    include(FetchContent)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON  CACHE BOOL "" FORCE)
    set(SDL_TEST   OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-2.30.7
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(SDL2)
endif()

# Normalize the SDL2 target name so src/CMakeLists.txt can always link `nds::sdl2`.
if(TARGET SDL2::SDL2-static)
    add_library(nds_sdl2 INTERFACE)
    target_link_libraries(nds_sdl2 INTERFACE SDL2::SDL2-static)
    if(TARGET SDL2::SDL2main)
        target_link_libraries(nds_sdl2 INTERFACE SDL2::SDL2main)
    endif()
elseif(TARGET SDL2::SDL2)
    add_library(nds_sdl2 INTERFACE)
    target_link_libraries(nds_sdl2 INTERFACE SDL2::SDL2)
    if(TARGET SDL2::SDL2main)
        target_link_libraries(nds_sdl2 INTERFACE SDL2::SDL2main)
    endif()
else()
    # Old FindSDL2 modules expose SDL2_INCLUDE_DIRS / SDL2_LIBRARIES.
    add_library(nds_sdl2 INTERFACE)
    target_include_directories(nds_sdl2 INTERFACE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(nds_sdl2 INTERFACE ${SDL2_LIBRARIES})
endif()
add_library(nds::sdl2 ALIAS nds_sdl2)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
