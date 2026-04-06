#include <SDL3/SDL_main.h>

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../include/Constants.h"
#include "../include/NES.h"
#include "../include/Renderer/Renderer.h" // includes SDH.h

std::vector<uint8_t> readROM(char *filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " +
                                 std::string(filename));
    }

    // read rom file into vector
    std::vector<uint8_t> romDump((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());

    return romDump;
}

void initialise_SDL(SDL_Window *&sdlWindow, SDL_Renderer *&sdlRenderer,
                    SDL_Texture *&sdlTexture) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("SDL_Init Error: ") +
                                 SDL_GetError());
    }

    if (!SDL_CreateWindowAndRenderer("NES Emulator - tobygrice.com",
                                     RENDER_WIDTH,  // width, in pixels
                                     RENDER_HEIGHT, // height, in pixels
                                     0,              // flags
                                     &sdlWindow, &sdlRenderer)) {
        SDL_Quit();
        throw std::runtime_error(
            std::string("SDL_CreateWindowAndRenderer Error: ") +
            SDL_GetError());
    }
    if (!SDL_SetRenderVSync(sdlRenderer, 1)) {
        std::cerr << "Warning: could not enable VSync: " << SDL_GetError()
                  << std::endl;
    }

    // Create a streaming texture for updating pixel data
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_RGB24,
                                   SDL_TEXTUREACCESS_STREAMING, RENDER_WIDTH,
                                   RENDER_HEIGHT);

    if (!sdlTexture) {
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(sdlWindow);
        SDL_Quit();
        throw std::runtime_error(std::string("SDL_CreateTexture Error: ") +
                                 SDL_GetError());
    }

    if (!SDL_SetTextureScaleMode(sdlTexture, SDL_SCALEMODE_NEAREST)) {
        SDL_DestroyTexture(sdlTexture);
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(sdlWindow);
        SDL_Quit();
        throw std::runtime_error(
            std::string("SDL_SetTextureScaleMode Error: ") + SDL_GetError());
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        throw std::invalid_argument("Usage: nesemu <rom.nes> [--trace]");
    }

    bool enableTrace = false;
    if (argc == 3) {
        if (std::string(argv[2]) == "--trace") {
            enableTrace = true;
        } else {
            throw std::invalid_argument("Unknown option: " +
                                        std::string(argv[2]));
        }
    }

    SDL_Window *sdlWindow = nullptr;
    SDL_Renderer *sdlRenderer = nullptr;
    SDL_Texture *sdlTexture = nullptr;
    initialise_SDL(sdlWindow, sdlRenderer, sdlTexture);

    std::vector<uint8_t> romDump = readROM(argv[1]); // read ROM from file
    Renderer renderer(sdlWindow, sdlRenderer, sdlTexture);
    NES nes(std::move(renderer), romDump); // instantiate a virtual NES console
    if (!enableTrace) {
        nes.log.mute();
    }
    nes.start();

    return 0;
}
