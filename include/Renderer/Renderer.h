#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <iostream>

#include "Frame.h"

class Renderer {
 private:
  SDL_Window* sdlWindow;
  SDL_Renderer* sdlRenderer;
  SDL_Texture* sdlTexture;
  const int WIDTH = 256;
  const int HEIGHT = 240;
  const int SCALE = 3;

 public:
  // Constructor: initializes SDL, creates a window, renderer, and texture.
  Renderer() : sdlWindow(nullptr), sdlRenderer(nullptr), sdlTexture(nullptr) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      throw std::runtime_error(std::string("SDL_Init Error: ") +
                               SDL_GetError());
    }

    SDL_CreateWindowAndRenderer("grice.software - NES EMU",
                                WIDTH * SCALE,      // width, in pixels
                                HEIGHT * SCALE,     // height, in pixels
                                SDL_WINDOW_OPENGL,  // flags
                                &sdlWindow, &sdlRenderer);
    SDL_SetRenderScale(sdlRenderer, SCALE, SCALE);

    // Create a streaming texture for updating pixel data
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_RGB24,
                                   SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!sdlTexture) {
      SDL_DestroyRenderer(sdlRenderer);
      SDL_DestroyWindow(sdlWindow);
      SDL_Quit();
      throw std::runtime_error(std::string("SDL_CreateTexture Error: ") +
                               SDL_GetError());
    }

    std::cout << "initialised sdl\n";
  }

  // Destructor: cleans up SDL resources.
  ~Renderer() {
    if (sdlTexture) SDL_DestroyTexture(sdlTexture);
    if (sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
    if (sdlWindow) SDL_DestroyWindow(sdlWindow);
    SDL_Quit();
  }

  // Render function: converts a Frame's pixel data into an image on the window.
  void render(Frame* frame);
};

#endif  // RENDERER_H