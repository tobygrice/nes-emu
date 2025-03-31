#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "../include/NES.h"
#include "../include/Renderer/Renderer.h"

std::vector<uint8_t> readROM(char *filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open file: " + std::string(filename));
  }

  // read rom file into vector
  std::vector<uint8_t> romDump((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());

  return romDump;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    throw std::invalid_argument(
        "Expected single argument specifiying name of .nes file");
  }

  // SDL initialisation:
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  SDL_Texture *texture = nullptr;

  SDL_Init(SDL_INIT_VIDEO);  // initialise SDL3

  int scale = 3;
  SDL_CreateWindowAndRenderer("grice.software - NES EMU",
                              256 * scale,        // width, in pixels
                              240 * scale,        // height, in pixels
                              SDL_WINDOW_OPENGL,  // flags
                              &window, &renderer);
  SDL_SetRenderScale(renderer, scale, scale);

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                              SDL_TEXTUREACCESS_STREAMING, 256, 240);

  std::vector<uint8_t> romDump = readROM(argv[1]);  // read ROM from file
  NES nes = NES();  // instantiate a virtual NES console
  // load cartridge (triggers reset interrupt on CPU)
  nes.insertCartridge(romDump);
  nes.start();

  // Close and destroy the window
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}