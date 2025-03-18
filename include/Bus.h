#ifndef BUS_H
#define BUS_H

#include <array>
#include <cstdint>

#include "BusInterface.h"
#include "CPU/CPU.h"
#include "Cartridge.h"
#include "PPU/PPU.h"

// Memory Management Unit (Bus)
class Bus : public BusInterface {
 private:
  // https://fceux.com/web/help/NESRAMMappingFindingValues.html
  std::array<uint8_t, 0x0800> cpu_ram;  // $0000 – $07FF: CPU RAM
                                        // $0800 – $1FFF: mirrors of CPU RAM
                                        // $2000 - $2007: PPU registers
                                        // $2008 – $3FFF: mirrors of PPU regs
  std::array<uint8_t, 0x0020> apu_io;   // $4000 – $401F: APU & I/O registers
  std::array<uint8_t, 0x1FE0> exp_rom;  // $4020 – $5FFF: cart expansion ROM
  std::array<uint8_t, 0x2000> s_ram;    // $6000 – $7FFF: save RAM
  CPU* cpu;
  PPU* ppu;
  Cartridge* cart;  // $8000 - $FFFF: cartridge ROM

  uint64_t cycles = 0;  // global cycle counter

 public:
  Bus(CPU* cpu, PPU* ppu, Cartridge* cart)
      : cpu_ram{},
        apu_io{},
        exp_rom{},
        s_ram{},
        cpu(cpu),
        ppu(ppu),
        cart(cart),
        cycles(7)  // pre-tick (error point)
  {
    apu_io.fill(0xFF);  // init FF
  }

  inline void tick(uint8_t c) override {
    ppu->tick(c * 3);
    cycles += c;
  }

  inline void tickCPU() {
    uint8_t cpu_cycles = cpu->executeInstruction();
    tick(cpu_cycles);
  }

  inline bool ppuNMI() override { return ppu->getNMI(); }
  inline uint16_t getPPUScanline() override { return ppu->getScanline(); }
  inline uint16_t getPPUCycle() override { return ppu->getCycle(); }

  inline uint64_t getCycleCount() const override { return cycles; }
  inline void resetCycles() override { cycles = 0; }

  inline uint8_t read(uint16_t addr) override {
    // cycles++;
    // CPU RAM mirror: 0x0000 - 0x1FFF
    if (addr <= 0x1FFF) {
      addr &= 0b0000011111111111;  // mirror down addr
      return cpu_ram[addr];
    } else if (addr == 0x2002) {
      return ppu->read_status();
    } else if (addr == 0x2004) {
      return ppu->read_oam_data();
    } else if (addr == 0x2007) {
      return ppu->read_data();
    } else if ((addr >= 0x2000) && (addr <= 0x2006)) {
      // 0x2000, 0x2001, 0x2003, 0x2005, 0x2006
      // PPU READ ONLY - return last value written to 0x2000 -> 0x2007
      return ppu->lastWrittenValue();
    }
    // PPU registers mirror: 0x2008 to 0x3FFF
    else if (addr >= 0x2008 && addr <= 0x3FFF) {
      // mirror down to 0x2000-0x2007 and recurse
      return read(addr & 0x2007);
    } else if (addr >= 0x4000 && addr <= 0x4015) {
      return 0;  // apu->readRegister(addr);
    } else if (addr == 0x4016) {
      return 0;  // joypad 1
    } else if (addr == 0x4017) {
      return 0;  // joypad 2
    } else if (addr >= 0x8000 && addr <= 0xFFFF) {
      return cart->read_prg_rom(addr);
    } else {
      // error point / TO-DO: missing exp_rom, s_ram and apu_io
      // cartridge PRG_ROM space: 0x8000 to 0xFFFF
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
      ppu->write_to_ctrl(value);
    } else if (addr == 0x2001) {
      ppu->write_to_mask(value);
    } else if (addr == 0x2003) {
      ppu->write_to_oam_addr(value);
    } else if (addr == 0x2004) {
      ppu->write_to_oam_data(value);
    } else if (addr == 0x2005) {
      ppu->write_to_scroll(value);
    } else if (addr == 0x2006) {
      ppu->write_to_ppu_addr(value);
    } else if (addr == 0x2007) {
      ppu->write_to_data(value);
    } else if (addr >= 0x2008 && addr <= 0x3FFF) {
      // PPU registers mirror: 0x2008 to 0x3FFF
      // mirror down to 0x2000-0x2007 and recurse
      write(addr & 0x2007, value);
    } else if (addr == 0x4014) {
      // data written to 0x4014 is the high byte of a memery block
      // in CPU RAM.
      std::array<uint8_t, 256> buffer;
      // compute the start address
      uint16_t start_addr = static_cast<uint16_t>(value) << 8;

      for (uint16_t i = 0; i < 256; i++) {
        buffer[i] = read(start_addr + i);
      }
      ppu->write_oam_dma(buffer);

      // TODO: fix cycle counting here (error point)
      // uint16_t add_cycles = (cycles % 2 == 1) ? 514 : 513;
      // tick(add_cycles);  // This would need PPU ticks (add_cycles * 3)
    } else if (addr >= 0x4000 && addr <= 0x4015) {
      // apu write
    } else if (addr == 0x4016) {
      // joypad 1
    } else if (addr == 0x4017) {
      // joypad 2
    } else {
      // error point / TO-DO: missing exp_rom, s_ram and apu_io
      throw std::runtime_error("Attempt to write to unsupported address: " +
                               std::to_string(addr));
    }
  }
};

#endif  // BUS_H