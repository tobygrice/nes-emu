#include "../../include/Renderer/Renderer.h"

void Renderer::render(Frame* frame) {
  std::cout << "renderer called\n";
  if (!frame) {
    throw std::runtime_error("Renderer given nullptr to frame.");
  }

  if (SDL_UpdateTexture(sdlTexture, nullptr, frame->pixelData.data(),
                        WIDTH * 3) != 0) {
    throw std::runtime_error(std::string("SDL_UpdateTexture Error: ") +
                             SDL_GetError());
  }

  if (SDL_RenderClear(sdlRenderer) != 0) {
    throw std::runtime_error(std::string("SDL_RenderClear Error: ") +
                             SDL_GetError());
  }

  if (SDL_RenderTexture(sdlRenderer, sdlTexture, nullptr, nullptr) != 0) {
    throw std::runtime_error(std::string("SDL_RenderTexture Error: ") +
                             SDL_GetError());
  }

  SDL_RenderPresent(sdlRenderer);
}