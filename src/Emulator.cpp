#include <SDL3/SDL_main.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "../include/NES.h"
#include "../include/Renderer/Renderer.h"

namespace {
std::vector<uint8_t> readROM(const char *filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + std::string(filename));
    }
    return {(std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()};
}
} // namespace

int main(int argc, char *argv[]) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            std::cout << "Usage: nesemu <rom.nes> [--trace]\n";
            return EXIT_SUCCESS;
        }
        if (argc < 2 || argc > 3) {
            throw std::invalid_argument("Usage: nesemu <rom.nes> [--trace]");
        }
        const bool enableTrace = argc == 3 && std::string(argv[2]) == "--trace";
        if (argc == 3 && !enableTrace) {
            throw std::invalid_argument("Unknown option: " + std::string(argv[2]));
        }

        const auto romDump = readROM(argv[1]);
        NES nes(Renderer::create(), romDump);
        if (!enableTrace) {
            nes.log.mute();
        }
        nes.start();
        return EXIT_SUCCESS;
    } catch (const std::exception &error) {
        std::cerr << "nesemu: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
