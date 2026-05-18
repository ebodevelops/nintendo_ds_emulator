add_executable(nds_emu
    common/log.cpp
    cart/header.cpp
    cart/cartridge.cpp
    memory/memory_map.cpp
    cpu/arm_core.cpp
    cpu/arm7.cpp
    cpu/arm9.cpp
    core/nds.cpp
    frontend/config.cpp
    frontend/window.cpp
    frontend/main.cpp
)

target_include_directories(nds_emu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(nds_emu PRIVATE nds::sdl2)

if(MSVC)
    target_compile_options(nds_emu PRIVATE /W4 /permissive- /Zc:__cplusplus)
    target_compile_definitions(nds_emu PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
    target_compile_options(nds_emu PRIVATE -Wall -Wextra -Wpedantic)
endif()
