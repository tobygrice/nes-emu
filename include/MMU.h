#ifndef MMU_H
#define MMU_H

#include <array>
#include <cstdint>

#include "BusInterface.h"
#include "Cartridge.h"
#include "PPU/PPU.h"

// Memory Management Unit (Bus)
class MMU : public BusInterface {
 private:
  // https://fceux.com/web/help/NESRAMMappingFindingValues.html
  std::array<uint8_t, 0x0800> cpu_ram;  // $0000 – $07FF: CPU RAM
                                        // $0800 – $1FFF: mirrors of CPU RAM
                                        // $2000 - $2007: PPU registers
                                        // $2008 – $3FFF: mirrors of PPU regs
  std::array<uint8_t, 0x0020> apu_io;   // $4000 – $401F: APU & I/O registers
  std::array<uint8_t, 0x1FE0> exp_rom;  // $4020 – $5FFF: cart expansion ROM
  std::array<uint8_t, 0x2000> s_ram;    // $6000 – $7FFF: save RAM
  Cartridge cart;                       // $8000 - $FFFF: cartridge ROM
  PPU ppu;

  uint64_t cycles = 0;  // global cycle counter

 public:
  MMU(const std::vector<uint8_t>& romDump)
      : cpu_ram{},
        apu_io{},
        exp_rom{},
        s_ram{},
        cart(romDump),
        ppu(&cart),
        cycles(0) {
    apu_io.fill(0xFF);  // init FF
  }

  inline void tick(uint8_t c) override {
    cycles += c;
    ppu.tick(c * 3);
  }

  inline bool ppuNMI() override { return ppu.getNMI(); }

  inline uint64_t getCycleCount() const override { return cycles; }
  inline void resetCycles() override { cycles = 0; }

  inline uint8_t read(uint16_t addr) override {
    // cycles++;
    // CPU RAM mirror: 0x0000 - 0x1FFF
    if (addr <= 0x1FFF) {
      addr &= 0b0000011111111111;  // mirror down addr
      return cpu_ram[addr];
    }
    // 0x2002: PPU status
    else if (addr == 0x2002) {
      return ppu.read_status();
    }
    // 0x2004: PPU OAM data
    else if (addr == 0x2004) {
      return ppu.read_oam_data();
    }
    // 0x2007: PPU data port (read)
    else if (addr == 0x2007) {
      return ppu.read_data();
    }
    // PPU registers mirror: 0x2008 to 0x3FFF
    else if (addr >= 0x2008 && addr <= 0x3FFF) {
      // mirror down to 0x2000-0x2007 and recurse
      return read(addr & 0x2007);
    }
    // error point / TO-DO: missing exp_rom, s_ram and apu_io
    // cartridge PRG_ROM space: 0x8000 to 0xFFFF
    else if (addr >= 0x8000 && addr <= 0xFFFF) {
      return cart.read_prg_rom(addr);
    } else {
      throw std::runtime_error("Attempt to read from unsupported address: " +
                               std::to_string(addr));
      return 0;
    }
  }

  inline void write(uint16_t addr, uint8_t value) override {
    // cycles++;
    // CPU RAM mirror: 0x0000 - 0x1FFF
    if (addr <= 0x1FFF) {
      addr &= 0b0000011111111111;  // mirror down addr
      cpu_ram[addr] = value;
    } else if (addr == 0x2000) {
      ppu.write_to_ctrl(value);
    } else if (addr == 0x2001) {
      ppu.write_to_mask(value);
    } else if (addr == 0x2003) {
      ppu.write_to_oam_addr(value);
    } else if (addr == 0x2004) {
      ppu.write_to_oam_data(value);
    } else if (addr == 0x2005) {
      ppu.write_to_scroll(value);
    } else if (addr == 0x2006) {
      ppu.write_to_ppu_addr(value);
    } else if (addr == 0x2007) {
      ppu.write_to_data(value);
    }
    // PPU registers mirror: 0x2008 to 0x3FFF
    else if (addr >= 0x2008 && addr <= 0x3FFF) {
      // mirror down to 0x2000-0x2007 and recurse
      write(addr & 0x2007, value);
    }
    // error point / TO-DO: missing exp_rom, s_ram and apu_io
    else {
      throw std::runtime_error("Attempt to write to unsupported address: " +
                               std::to_string(addr));
    }
  }
};

#endif