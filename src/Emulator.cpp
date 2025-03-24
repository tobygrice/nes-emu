#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

// #include "../include/Bus.h"
// #include "../include/CPU/CPU.h"
// #include "../include/Logger.h"
#include "../include/NES.h"
#include "../include/Renderer/Renderer.h"
#include "../include/Clock.h"

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

  std::vector<uint8_t> romDump = readROM(argv[1]);

  NES nes = NES(romDump);  // instantiate a virtual NES console
  // NES nes = NES(); // empty nes
  Renderer nes_renderer = Renderer();

  // nes.cpu.setPC(0xC000);  // overwrite reset vector for incomplete emulators
  // nes.cpu.executeProgram();

  // SDL initialisation:
  SDL_Window *window = nullptr;
  SDL_Renderer *sdl_renderer = nullptr;
  SDL_Texture *texture = nullptr;

  SDL_Init(SDL_INIT_VIDEO);  // Initialize SDL3

  // Create an application window with the following settings:
  window = SDL_CreateWindow("grice.software - NES EMU",
                            256,               // width, in pixels
                            240,               // height, in pixels
                            SDL_WINDOW_OPENGL  // flags
  );
  sdl_renderer = SDL_CreateRenderer(window, NULL);
  SDL_SetRenderScale(sdl_renderer, 3.0f, 3.0f);
  texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_RGB24,
                              SDL_TEXTUREACCESS_STREAMING, 256, 240);

  
  Clock clock = Clock(&nes, NESRegion::NTSC);
  // clock.start();
  
  nes.cpu.triggerRES();
  for (int i=0; i<7; i++) {
    nes.cpu.tick();
  }

  // Close and destroy the window
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(sdl_renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}