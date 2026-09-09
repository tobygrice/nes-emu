#include "../../include/Renderer/Renderer.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
std::runtime_error sdlError(const char *operation) {
    return std::runtime_error(std::string(operation) + ": " + SDL_GetError());
}
} // namespace

Renderer Renderer::create() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw sdlError("SDL_Init");
    }

    // Adopt each resource immediately so partial initialization also cleans up.
    Renderer result(nullptr, nullptr, nullptr);
    try {
        result.sdlWindow.reset(SDL_CreateWindow(
            "NES Emulator - tobygrice.com", RENDER_WIDTH, RENDER_HEIGHT, 0));
        if (!result.sdlWindow) {
            throw sdlError("SDL_CreateWindow");
        }
        result.sdlRenderer.reset(SDL_CreateRenderer(result.sdlWindow.get(), nullptr));
        if (!result.sdlRenderer) {
            throw sdlError("SDL_CreateRenderer");
        }
        if (!SDL_SetRenderVSync(result.sdlRenderer.get(), 1)) {
            std::cerr << "Warning: could not enable VSync: " << SDL_GetError()
                      << '\n';
        }
        if (!SDL_SetRenderLogicalPresentation(result.sdlRenderer.get(),
                SCREEN_WIDTH, SCREEN_HEIGHT, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)) {
            throw sdlError("SDL_SetRenderLogicalPresentation");
        }

        result.sdlTexture.reset(SDL_CreateTexture(result.sdlRenderer.get(),
            SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING,
            SCREEN_WIDTH, SCREEN_HEIGHT));
        if (!result.sdlTexture) {
            throw sdlError("SDL_CreateTexture");
        }
        if (!SDL_SetTextureScaleMode(result.sdlTexture.get(), SDL_SCALEMODE_NEAREST)) {
            throw sdlError("SDL_SetTextureScaleMode");
        }
    } catch (...) {
        // If window creation failed, result has no resource to signal SDL ownership.
        if (!result.sdlWindow) {
            SDL_Quit();
        }
        throw;
    }
    return result;
}

void Renderer::render(const Frame &frame) {
    if (!SDL_UpdateTexture(sdlTexture.get(), nullptr, frame.pixelData.data(),
                           SCREEN_WIDTH * 3)) {
        throw sdlError("SDL_UpdateTexture");
    }
    if (!SDL_RenderClear(sdlRenderer.get())) {
        throw sdlError("SDL_RenderClear");
    }
    if (!SDL_RenderTexture(sdlRenderer.get(), sdlTexture.get(), nullptr, nullptr)) {
        throw sdlError("SDL_RenderTexture");
    }
    if (!SDL_RenderPresent(sdlRenderer.get())) {
        throw sdlError("SDL_RenderPresent");
    }
}
