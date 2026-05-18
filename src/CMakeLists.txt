add_library(nds_emu_core STATIC
    common/log.cpp
    cart/header.cpp
    cart/cartridge.cpp
    memory/memory_map.cpp
    cpu/arm_core.cpp
    cpu/arm7.cpp
    cpu/arm7_arm.cpp
    cpu/arm7_thumb.cpp
    cpu/arm9.cpp
    core/nds.cpp
)
target_include_directories(nds_emu_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(nds_emu
    frontend/config.cpp
    frontend/window.cpp
    frontend/main.cpp
)
target_link_libraries(nds_emu PRIVATE nds_emu_core nds::sdl2)

if(MSVC)
    foreach(t nds_emu_core nds_emu)
        target_compile_options(${t} PRIVATE /W4 /permissive- /Zc:__cplusplus)
        target_compile_definitions(${t} PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
    endforeach()
else()
    foreach(t nds_emu_core nds_emu)
        target_compile_options(${t} PRIVATE -Wall -Wextra -Wpedantic)
    endforeach()
endif()
