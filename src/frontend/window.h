#pragma once

#include "common/types.h"

#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace nds::frontend {

// SDL2 window sized for two DS screens (256x192 each) stacked vertically.
class Window {
public:
    static constexpr int kScreenWidth   = 256;
    static constexpr int kScreenHeight  = 192;
    static constexpr int kWindowWidth   = kScreenWidth;
    static constexpr int kWindowHeight  = kScreenHeight * 2;

    Window() = default;
    ~Window();
    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    bool init(const std::string& title);

    // Returns false when the user has closed the window or pressed escape.
    bool pump_events();

    // Present a solid-color frame. Until the 2D engines land this stays
    // black; later this will be replaced by a buffer upload.
    void present_blank();

private:
    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};

}  // namespace nds::frontend
