#include "../../include/Renderer/Renderer.h"

void Renderer::render(const Frame &frame) {
    constexpr float OVERSCAN_LEFT = 8.0f;
    constexpr float OVERSCAN_TOP = 8.0f;
    const SDL_FRect sourceRect{OVERSCAN_LEFT, OVERSCAN_TOP,
                               static_cast<float>(WIDTH) - OVERSCAN_LEFT,
                               static_cast<float>(HEIGHT) - OVERSCAN_TOP};
    const SDL_FRect destinationRect{0.0f, 0.0f, static_cast<float>(WIDTH),
                                    static_cast<float>(HEIGHT)};

    if (!SDL_UpdateTexture(sdlTexture.get(), nullptr, frame.pixelData.data(),
                           WIDTH * 3)) {
        throw std::runtime_error(std::string("SDL_UpdateTexture Error: ") +
                                 SDL_GetError());
    }

    if (!SDL_RenderClear(sdlRenderer.get())) {
        throw std::runtime_error(std::string("SDL_RenderClear Error: ") +
                                 SDL_GetError());
    }

    // crop overscan area and scale remaining image
    if (!SDL_RenderTexture(sdlRenderer.get(), sdlTexture.get(), &sourceRect,
                           &destinationRect)) {
        throw std::runtime_error(std::string("SDL_RenderTexture Error: ") +
                                 SDL_GetError());
    }

    SDL_RenderPresent(sdlRenderer.get());
}
