add_executable(test_arm7 test_arm7.cpp)
target_include_directories(test_arm7 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_arm7 PRIVATE nds_emu_core)
add_test(NAME arm7 COMMAND test_arm7)
