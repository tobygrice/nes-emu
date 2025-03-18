#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "../include/Bus.h"
#include "../include/CPU/CPU.h"
#include "../include/Logger.h"
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

  std::vector<uint8_t> romDump = readROM(argv[1]);

  NES nes = NES(romDump);  // instantiate a virtual NES console
  Renderer nes_renderer = Renderer();

  // std::cout << nes.cpu.getPC() << std::endl;
  // exit(0);
  nes.cpu.setPC(0xC000);  // overwrite reset vector for incomplete emulators
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

  bool running = true;
  while (running) {
    // Run the emulation until a frame is rendered
    nes.generateFrame();

    Frame frame = Frame();
    nes_renderer.render(nes.ppu, frame);

    // Update the texture with the new frame from nes.ppu.getFrameData() (or
    // similar)
    SDL_UpdateTexture(texture, nullptr, frame.data.data(), 256 * 3);

    // Render and present the frame
    SDL_RenderClear(sdl_renderer);
    SDL_RenderTexture(sdl_renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(sdl_renderer);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }
  }

  // Close and destroy the window
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(sdl_renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}