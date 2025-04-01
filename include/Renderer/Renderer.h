#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

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
  Renderer(SDL_Window* w, SDL_Renderer* r, SDL_Texture* t)
      : sdlWindow(w), sdlRenderer(r), sdlTexture(t) {}

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