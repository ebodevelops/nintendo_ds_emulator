#pragma once

#include <string>

namespace nds::frontend {

struct Config {
    std::string rom_path;
    bool        verbose = false;
};

// Parse argv. Returns true if a config was produced; false if the user asked
// for --help or arguments were invalid (in which case usage is printed to
// stderr).
bool parse_args(int argc, char** argv, Config& out);

void print_usage(const char* prog);

}  // namespace nds::frontend
