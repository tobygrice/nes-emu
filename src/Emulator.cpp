#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "../include/CPU/CPU.h"
#include "../include/Logger.h"
#include "../include/MMU.h"

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

  // CPU is passed reference to MMU. MMU holds PPU.
  Logger logger = Logger();
  MMU mmu = MMU(romDump);        // instantiate memory management unit (bus)
  CPU cpu = CPU(&mmu, &logger);  // instantiate CPU and provide bus

  cpu.in_RESET();  // call CPU reset interrupt to emulate cartridge insertion
  // cpu.setPC(0xC000);  // overwrite reset vector for incomplete emulators
  // cpu.executeProgram();

  // SDL initialisation:
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  bool done = false;

  SDL_Init(SDL_INIT_VIDEO);  // Initialize SDL3

  // Create an application window with the following settings:
  window = SDL_CreateWindow("grice.software - NES EMU",
                            256,               // width, in pixels
                            240,               // height, in pixels
                            SDL_WINDOW_OPENGL  // flags
  );
  renderer = SDL_CreateRenderer(window, NULL);

  if (window == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n",
                 SDL_GetError());
    return 1;
  }

  while (!done) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        done = true;
      }
    }

    // Do game logic, present a frame, etc.
    cpu.executeInstruction(); // will automatically tick PPU/APU

    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
  }

  // Close and destroy the window
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}