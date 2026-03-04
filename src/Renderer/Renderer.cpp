#include "../../include/Renderer/Renderer.h"

void Renderer::render(const Frame& frame) {
  std::cout << "renderer called\n";
  if (!SDL_UpdateTexture(sdlTexture.get(), nullptr, frame.pixelData.data(),
                        WIDTH * 3)) {
    throw std::runtime_error(std::string("SDL_UpdateTexture Error: ") +
                             SDL_GetError());
  }

  if (!SDL_RenderClear(sdlRenderer.get())) {
    throw std::runtime_error(std::string("SDL_RenderClear Error: ") +
                             SDL_GetError());
  }

  if (!SDL_RenderTexture(sdlRenderer.get(), sdlTexture.get(), nullptr,
                         nullptr)) {
    throw std::runtime_error(std::string("SDL_RenderTexture Error: ") +
                             SDL_GetError());
  }

  SDL_RenderPresent(sdlRenderer.get());
}
