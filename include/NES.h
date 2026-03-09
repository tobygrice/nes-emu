#ifndef NES_H
#define NES_H

#include <utility>
#include <vector>

#include "Bus.h"
#include "CPU/CPU.h"
#include "Cartridge.h"
#include "Clock.h"
#include "Logger.h"
#include "PPU/PPU.h"
#include "Renderer/Renderer.h"

/**
 * Virtual implementation of an NES console. Instantiates and correctly links
 * all hardware components.
 */
class NES {
  public:
    Logger log;
    Cartridge cart;
    PPU ppu;
    Bus bus;
    CPU cpu;
    Renderer renderer;
    Clock clock;
    // APU apu;

    NES(const NES &) = delete;
    NES &operator=(const NES &) = delete;
    NES(NES &&) = delete;
    NES &operator=(NES &&) = delete;

    /**
     * Instantiates all components and loads romDump into cartridge.
     * @param renderer Renderer ownership is transferred to NES.
     * @param romDump iNES 1.0 format NES ROM dump.
     */
    NES(Renderer renderer, const std::vector<uint8_t> &romDump)
        : log(), cart(), ppu(cart), bus(ppu, cart), cpu(bus, log),
          renderer(std::move(renderer)), clock(*this) {
        insertCartridge(romDump);
    }

    /**
     * Loads romDump into cartridge.
     * @param romDump iNES 1.0 format NES ROM dump.
     */
    void insertCartridge(const std::vector<uint8_t> &romDump) {
        cart.load(romDump);
        clock.setRegion(cart.getRegion());

        // reset interrupt called on cartridge insertion
        // tick CPU past reset interrupt without PPU
        cpu.triggerRES();
        for (int i = 0; i < 7; i++)
            cpu.tick();
    }

    void start() { clock.start(); }
};

#endif // NES_H
