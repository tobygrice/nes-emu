#ifndef MMU_H
#define MMU_H

#include <array>
#include <cstdint>

#include "BusInterface.h"
#include "Cartridge.h"

// Memory Management Unit (Bus)
class MMU : public BusInterface {
 private:
  // https://fceux.com/web/help/NESRAMMappingFindingValues.html
  std::array<uint8_t, 0x0800> cpu_ram;  // $0000 – $07FF: CPU RAM
                                        // $0800 – $1FFF: mirrors of CPU RAM
  std::array<uint8_t, 0x0008> ppu_reg;  // $2000 - $2007: PPU registers
                                        // $2008 – $3FFF: mirrors of PPU regs
  std::array<uint8_t, 0x0020> apu_io;   // $4000 – $401F: APU & I/O registers
  std::array<uint8_t, 0x1FE0> exp_rom;  // $4020 – $5FFF: cart expansion ROM
  std::array<uint8_t, 0x2000> s_ram;    // $6000 – $7FFF: save RAM
  Cartridge cart;     // $8000 - $FFFF: cartridge ROM

  uint64_t cycles = 0;  // global cycle counter

 public:
  MMU(const std::vector<uint8_t>& romDump) {
    cycles = 0;
    cpu_ram.fill(0);
    ppu_reg.fill(0);
    // ppu_reg[2] = 0x80;
    apu_io.fill(0xFF); // init FF
    exp_rom.fill(0);
    s_ram.fill(0);
    cart = Cartridge(romDump);
  }

  inline uint64_t getCycleCount() const override { return cycles; }

  inline uint8_t read(uint16_t addr) override {
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
        return cart.read_prg_rom(addr - 0x8000);  // cartridge rom
    }
  }

  inline uint8_t read_chr_rom(uint16_t addr) {
    cycles++;
    return cart.read_chr_rom(addr);  // cartridge rom
  }

  inline void write(uint16_t addr, uint8_t value) override {
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
        break; // ignore writes to CE-ROM
      case 0x6000:
        s_ram[addr - 0x6000] = value;
        break;
      default:
        break;  // ignore writes to ROM
    }
  }
};

#endif