#include "frontend/config.h"

#include <cstdio>
#include <cstring>

namespace nds::frontend {

void print_usage(const char* prog) {
    std::fprintf(stderr,
        "Usage: %s [options] <rom.nds>\n"
        "\n"
        "Options:\n"
        "  -v, --verbose   Enable debug logging\n"
        "  -h, --help      Show this message\n",
        prog ? prog : "nds_emu");
}

bool parse_args(int argc, char** argv, Config& out) {
    if (argc < 2) {
        print_usage(argc > 0 ? argv[0] : nullptr);
        return false;
    }
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        if (std::strcmp(a, "-h") == 0 || std::strcmp(a, "--help") == 0) {
            print_usage(argv[0]);
            return false;
        } else if (std::strcmp(a, "-v") == 0 || std::strcmp(a, "--verbose") == 0) {
            out.verbose = true;
        } else if (a[0] == '-') {
            std::fprintf(stderr, "Unknown option: %s\n", a);
            print_usage(argv[0]);
            return false;
        } else {
            if (!out.rom_path.empty()) {
                std::fprintf(stderr, "Multiple ROM paths supplied\n");
                return false;
            }
            out.rom_path = a;
        }
    }
    if (out.rom_path.empty()) {
        print_usage(argv[0]);
        return false;
    }
    return true;
}

}  // namespace nds::frontend
