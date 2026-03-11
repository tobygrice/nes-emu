#include "../../include/Renderer/Renderer.h"

#include <cstring>

namespace {

void upscaleFramePixels(const Frame &frame, std::vector<uint8_t> &output) {
    constexpr std::size_t sourceStride =
        static_cast<std::size_t>(SCREEN_WIDTH) * 3;
    constexpr std::size_t outputStride =
        static_cast<std::size_t>(RENDER_WIDTH) * 3;

    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        const uint8_t *sourceRow =
            frame.pixelData.data() + (static_cast<std::size_t>(y) * sourceStride);
        uint8_t *firstOutputRow = output.data() +
                                  (static_cast<std::size_t>(y) *
                                   SCREEN_SCALING * outputStride);

        std::size_t outputOffset = 0;
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            const std::size_t sourceOffset = static_cast<std::size_t>(x) * 3;
            for (int scaleX = 0; scaleX < SCREEN_SCALING; ++scaleX) {
                firstOutputRow[outputOffset++] = sourceRow[sourceOffset];
                firstOutputRow[outputOffset++] = sourceRow[sourceOffset + 1];
                firstOutputRow[outputOffset++] = sourceRow[sourceOffset + 2];
            }
        }

        for (int scaleY = 1; scaleY < SCREEN_SCALING; ++scaleY) {
            std::memcpy(firstOutputRow +
                            (static_cast<std::size_t>(scaleY) * outputStride),
                        firstOutputRow, outputStride);
        }
    }
}

} // namespace

void Renderer::render(const Frame &frame) {
    upscaleFramePixels(frame, upscaledPixelData);

    if (!SDL_UpdateTexture(sdlTexture.get(), nullptr, upscaledPixelData.data(),
                           RENDER_WIDTH * 3)) {
        throw std::runtime_error(std::string("SDL_UpdateTexture Error: ") +
                                 SDL_GetError());
    }

    if (!SDL_RenderClear(sdlRenderer.get())) {
        throw std::runtime_error(std::string("SDL_RenderClear Error: ") +
                                 SDL_GetError());
    }

    int outputWidth = 0;
    int outputHeight = 0;
    if (!SDL_GetCurrentRenderOutputSize(sdlRenderer.get(), &outputWidth,
                                        &outputHeight)) {
        throw std::runtime_error(
            std::string("SDL_GetCurrentRenderOutputSize Error: ") +
            SDL_GetError());
    }

    const SDL_FRect destination = {0.0f, 0.0f,
                                   static_cast<float>(outputWidth),
                                   static_cast<float>(outputHeight)};
    if (!SDL_RenderTexture(sdlRenderer.get(), sdlTexture.get(), nullptr,
                           &destination)) {
        throw std::runtime_error(std::string("SDL_RenderTexture Error: ") +
                                 SDL_GetError());
    }

    SDL_RenderPresent(sdlRenderer.get());
}
