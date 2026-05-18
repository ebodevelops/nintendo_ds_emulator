#include "frontend/window.h"

#include "common/log.h"

#include <SDL.h>

namespace nds::frontend {

Window::~Window() {
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_)   SDL_DestroyWindow(window_);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

bool Window::init(const std::string& title) {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        NDS_LOG_ERROR("sdl", "SDL_InitSubSystem(VIDEO) failed: %s", SDL_GetError());
        return false;
    }
    window_ = SDL_CreateWindow(title.c_str(),
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               kWindowWidth, kWindowHeight,
                               SDL_WINDOW_SHOWN);
    if (!window_) {
        NDS_LOG_ERROR("sdl", "SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        // Fall back to software renderer (headless CI, RDP sessions, etc.).
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer_) {
        NDS_LOG_ERROR("sdl", "SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }
    SDL_RenderSetLogicalSize(renderer_, kWindowWidth, kWindowHeight);
    return true;
}

bool Window::pump_events() {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT) return false;
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) return false;
    }
    return true;
}

void Window::present_blank() {
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
}

}  // namespace nds::frontend
