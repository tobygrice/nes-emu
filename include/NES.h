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
  Bus bus;
  CPU cpu;
  PPU ppu;
  // APU apu;

  /**
   * Instantiates all components with an empty cartridge.
   */
  NES() : log(), cart(), bus(&cart), cpu(&bus, &log), ppu(&bus, &cart) {}

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
    cpu.in_RESET(); // reset interrupt called on cartridge insertion
    bus.setCycles(7); // consumes 7 CPU cycles
    // bus.tick(7) ? - nintendulator seems not to tick ppu, only cpu here
  }

  void generateFrame() {
    // run CPU until PPU triggers NMI
    while (!bus.ppuNMI()) {
      bus.tickCPU();
    }

    cpu.in_NMI();  // call CPU NMI handler
    bus.tick(8);   // NMI handler takes 8 cycles

    // continue to tick CPU while: 
    //  - CPU is still handling NMI
    //  - nmiInterrupt is still set (frame gen incomplete)
    while (cpu.isHandlingNMI() || ppu.getNMI()) {
      bus.tickCPU();
    }
  }
};

#endif  // NES_H