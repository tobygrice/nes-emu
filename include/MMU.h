#ifndef MMU_H
#define MMU_H

#include <array>
#include <cstdint>
#include <vector>

// Memory Management Unit (Bus)
class MMU {
 private:
  // https://fceux.com/web/help/NESRAMMappingFindingValues.html
  std::array<uint8_t, 0x0800> cpu_ram;  // $0000 – $07FF: CPU RAM
                                        // $0800 – $1FFF: mirrors of CPU RAM
  std::array<uint8_t, 0x0008> ppu_reg;  // $2000 - $2007: PPU registers
                                        // $2008 – $3FFF: mirrors of PPU regs
  std::array<uint8_t, 0x0020> apu_io;   // $4000 – $401F: APU & I/O registers
  std::array<uint8_t, 0x1FE0> exp_rom;  // $4020 – $5FFF: cart expansion ROM
  std::array<uint8_t, 0x2000> s_ram;    // $6000 – $7FFF: save RAM
  std::array<uint8_t, 0x8000> cart;     // $8000 - $FFFF: cartridge ROM

  uint64_t cycles;  // global cycle counter

 public:
  MMU();
  uint64_t getCycleCount();
  inline uint8_t read(uint16_t addr);
  inline void write(uint16_t addr, uint8_t value);
};

#endif