#include "../../include/Renderer/Renderer.h"

#include <cstdio>
#include <cstring>

void Renderer::render(const Frame& frame) {
  constexpr float OVERSCAN_LEFT = 8.0f;
  constexpr float OVERSCAN_TOP = 8.0f;
  const SDL_FRect sourceRect{
      OVERSCAN_LEFT,
      OVERSCAN_TOP,
      static_cast<float>(WIDTH) - OVERSCAN_LEFT,
      static_cast<float>(HEIGHT) - OVERSCAN_TOP};
  const SDL_FRect destinationRect{
      0.0f,
      0.0f,
      static_cast<float>(WIDTH),
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

  // Crop overscan area and scale the remaining image to the full output.
  if (!SDL_RenderTexture(sdlRenderer.get(), sdlTexture.get(), &sourceRect,
                         &destinationRect)) {
    throw std::runtime_error(std::string("SDL_RenderTexture Error: ") +
                             SDL_GetError());
  }

  const uint64_t nowMs = SDL_GetTicks();
  if (fpsWindowStartMs == 0) {
    fpsWindowStartMs = nowMs;
  }
  framesInCurrentWindow++;
  const uint64_t elapsedMs = nowMs - fpsWindowStartMs;
  if (elapsedMs >= 500) {
    currentFps = static_cast<float>(framesInCurrentWindow) * 1000.0f /
                 static_cast<float>(elapsedMs);
    framesInCurrentWindow = 0;
    fpsWindowStartMs = nowMs;
  }

  char fpsText[16];
  std::snprintf(fpsText, sizeof(fpsText), "FPS:%3.0f", currentFps);
  const int textWidth = static_cast<int>(
      std::strlen(fpsText) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE);
  const float textX = static_cast<float>(WIDTH - textWidth - 2);

  if (!SDL_SetRenderDrawColor(sdlRenderer.get(), 255, 255, 255, 255)) {
    throw std::runtime_error(std::string("SDL_SetRenderDrawColor Error: ") +
                             SDL_GetError());
  }
  SDL_RenderDebugText(sdlRenderer.get(), textX, 2.0f, fpsText);

  SDL_RenderPresent(sdlRenderer.get());
}
