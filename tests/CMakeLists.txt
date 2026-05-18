# GoogleTest-based unit tests will be added in milestone M1 onwards
# (ARM instruction tests via test ROMs and per-opcode unit tests).
#
# For now this placeholder keeps `ctest` exit code clean.
add_test(NAME placeholder
         COMMAND ${CMAKE_COMMAND} -E echo "tests pending - see roadmap (M1+)")
