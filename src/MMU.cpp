#include "../include/MMU.h"

/**
 * Constructor to initialise memory with zeros.
 */
MMU::MMU() {
  cycles = 0;
  cpu_ram.fill(0);
  ppu_reg.fill(0);
  apu_io.fill(0);
  exp_rom.fill(0);
  s_ram.fill(0);
  cart.fill(0);
}

uint64_t MMU::getCycleCount() { return cycles; }

inline uint8_t MMU::read(uint16_t addr) {
  cycles++;
  switch (addr & 0xE000) {
    case 0x0000:
      return cpu_ram[addr & 0x07FF];  // mirror RAM
    case 0x2000:
      return ppu_reg[(addr - 0x2000) & 0x0007];  // mirror PPU reg
    case 0x4000:
      if (addr < 0x4020) return apu_io[addr - 0x4000];  // APU and I/O
      return exp_rom[addr - 0x4020];  // cartridge expansion ROM
    case 0x6000:
      return s_ram[addr - 0x6000];  // save RAM
    default:
      return cart[addr - 0x8000];  // cartridge rom
  }
}

inline void MMU::write(uint16_t addr, uint8_t value) {
  cycles++;
  switch (addr & 0xE000) {
    case 0x0000:
      cpu_ram[addr & 0x07FF] = value;
      break;
    case 0x2000:
      ppu_reg[(addr - 0x2000) & 0x0007] = value;
      break;
    case 0x4000:
      if (addr < 0x4020) apu_io[addr - 0x4000] = value;  // APU and I/O
      // Ignore writes to cartridge expansion ROM (read-only)
      break;
    case 0x6000:
      s_ram[addr - 0x6000] = value;
      break;
    default:
      break;  // Ignore writes to ROM
  }
}
