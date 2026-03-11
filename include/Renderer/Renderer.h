#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "Frame.h"

class Renderer {
  private:
    struct WindowDeleter {
        void operator()(SDL_Window *window) const {
            if (window != nullptr) {
                SDL_DestroyWindow(window);
            }
        }
    };

    struct RendererDeleter {
        void operator()(SDL_Renderer *renderer) const {
            if (renderer != nullptr) {
                SDL_DestroyRenderer(renderer);
            }
        }
    };

    struct TextureDeleter {
        void operator()(SDL_Texture *texture) const {
            if (texture != nullptr) {
                SDL_DestroyTexture(texture);
            }
        }
    };

    std::unique_ptr<SDL_Window, WindowDeleter> sdlWindow;
    std::unique_ptr<SDL_Renderer, RendererDeleter> sdlRenderer;
    std::unique_ptr<SDL_Texture, TextureDeleter> sdlTexture;
    std::vector<uint8_t> upscaledPixelData =
        std::vector<uint8_t>(RENDER_WIDTH * RENDER_HEIGHT * 3, 0);
    uint64_t fpsWindowStartMs = 0;
    uint32_t framesInCurrentWindow = 0;
    float currentFps = 0.0f;

  public:
    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;
    Renderer(Renderer &&) noexcept = default;
    Renderer &operator=(Renderer &&) = delete;

    // Constructor: initializes SDL, creates a window, renderer, and texture.
    Renderer(SDL_Window *w, SDL_Renderer *r, SDL_Texture *t)
        : sdlWindow(w), sdlRenderer(r), sdlTexture(t) {}

    // Destructor: cleans up SDL resources.
    ~Renderer() {
        const bool ownsSDL = (sdlWindow != nullptr || sdlRenderer != nullptr ||
                              sdlTexture != nullptr);
        sdlTexture.reset();
        sdlRenderer.reset();
        sdlWindow.reset();
        if (ownsSDL) {
            SDL_Quit();
        }
    }

    // Renders a Frame object onto the SDL window
    void render(const Frame &frame);
};

#endif // RENDERER_H
