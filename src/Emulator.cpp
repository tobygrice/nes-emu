#include <SDL3/SDL_main.h>

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "../include/NES.h"
#include "../include/Renderer/Renderer.h" // includes SDH.h

std::vector<uint8_t> readROM(char* filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open file: " + std::string(filename));
  }

  // read rom file into vector
  std::vector<uint8_t> romDump((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());

  return romDump;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    throw std::invalid_argument(
        "Expected single argument specifiying name of .nes file");
  }

  SDL_Window* sdlWindow = nullptr;
  SDL_Renderer* sdlRenderer = nullptr;
  SDL_Texture* sdlTexture = nullptr;
  const int WIDTH = 256;
  const int HEIGHT = 240;
  const int SCALE = 3;

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    throw std::runtime_error(std::string("SDL_Init Error: ") + SDL_GetError());
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

  Renderer* renderer = new Renderer(sdlWindow, sdlRenderer, sdlTexture);

  std::vector<uint8_t> romDump = readROM(argv[1]);  // read ROM from file
  NES nes = NES(renderer);  // instantiate a virtual NES console

  // load cartridge (triggers reset interrupt on CPU)
  nes.insertCartridge(romDump);
  nes.start();

  delete renderer;
  return 0;
}