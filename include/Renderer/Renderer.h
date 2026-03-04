#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>

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
    void operator()(SDL_Window* window) const {
      if (window != nullptr) {
        SDL_DestroyWindow(window);
      }
    }
  };

  struct RendererDeleter {
    void operator()(SDL_Renderer* renderer) const {
      if (renderer != nullptr) {
        SDL_DestroyRenderer(renderer);
      }
    }
  };

  struct TextureDeleter {
    void operator()(SDL_Texture* texture) const {
      if (texture != nullptr) {
        SDL_DestroyTexture(texture);
      }
    }
  };

  std::unique_ptr<SDL_Window, WindowDeleter> sdlWindow;
  std::unique_ptr<SDL_Renderer, RendererDeleter> sdlRenderer;
  std::unique_ptr<SDL_Texture, TextureDeleter> sdlTexture;
  const int WIDTH = 256;

 public:
  // Constructor: initializes SDL, creates a window, renderer, and texture.
  Renderer(SDL_Window* w, SDL_Renderer* r, SDL_Texture* t)
      : sdlWindow(w), sdlRenderer(r), sdlTexture(t) {}

  // Destructor: cleans up SDL resources.
  ~Renderer() {
    sdlTexture.reset();
    sdlRenderer.reset();
    sdlWindow.reset();
    SDL_Quit();
  }

  // Render function: converts a Frame's pixel data into an image on the window.
  void render(const Frame& frame);
};

#endif  // RENDERER_H
