#ifndef NES_H
#define NES_H

#include "Bus.h"
#include "CPU/CPU.h"
#include "Cartridge.h"
#include "Logger.h"
#include "PPU/PPU.h"

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
  // APU apu;

  /**
   * Instantiates all components with an empty cartridge.
   */
  NES() : log(), cart(), ppu(&cart), bus(&ppu, &cart), cpu(&bus, &log) {}

  /**
   * Instantiates all components and loads romDump into cartridge.
   * @param romDump iNES 1.0 format NES ROM dump.
   */
  NES(const std::vector<uint8_t>& romDump) : NES() { insertCartridge(romDump); }

  /**
   * Loads romDump into cartridge.
   * @param romDump iNES 1.0 format NES ROM dump.
   */
  void insertCartridge(const std::vector<uint8_t>& romDump) {
    cart.load(romDump);
    cpu.triggerRES(); // reset interrupt called on cartridge insertion
    for (int i = 0; i < 7; i++) {
      cpu.tick();
    }
  }
};

#endif  // NES_H