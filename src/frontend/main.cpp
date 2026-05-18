#include "common/log.h"
#include "core/nds.h"
#include "frontend/config.h"
#include "frontend/window.h"

#include <SDL.h>

#include <string>

// SDL2 redefines main on Windows; both with-args and no-args forms are fine.
int main(int argc, char* argv[]) {
    nds::frontend::Config cfg;
    if (!nds::frontend::parse_args(argc, argv, cfg)) {
        return 1;
    }
    if (cfg.verbose) nds::log::set_level(nds::log::Level::Debug);

    NDS_LOG_INFO("main", "nds_emu starting");

    nds::core::Nds nds;
    if (!nds.load_rom(cfg.rom_path)) {
        return 2;
    }

    nds::frontend::Window window;
    const std::string title = "nds_emu - " + nds.cartridge().header().title_string();
    if (!window.init(title)) {
        return 3;
    }

    while (window.pump_events()) {
        nds.run_frame();
        window.present_blank();
        SDL_Delay(16);  // ~60 Hz placeholder until the scheduler drives timing.
    }

    NDS_LOG_INFO("main", "Shutting down");
    return 0;
}
