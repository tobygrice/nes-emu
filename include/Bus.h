#ifndef BUS_H
#define BUS_H

#include <array>
#include <cstdint>

#include "Cartridge.h"
#include "BusInterface.h"
#include "PPU/PPU.h"

// Memory Management Unit (Bus)
class Bus : public BusInterface {
 private:
  // https://fceux.com/web/help/NESRAMMappingFindingValues.html
  std::array<uint8_t, 0x0800> cpu_ram;  // $0000 – $07FF: CPU RAM
                                        // $0800 – $1FFF: mirrors of CPU RAM
                                        // $2000 - $2007: PPU registers
                                        // $2008 – $3FFF: mirrors of PPU regs
  Cartridge& cart;  // $8000 - $FFFF: cartridge ROM
  PPU& ppu;
  uint8_t joypad1Buttons = 0x00;
  uint8_t joypad1Shift = 0x00;
  bool joypadStrobe = false;

  uint64_t cycles = 0;  // global cycle counter
  bool dmaPending = false;
  bool dmaActive = false;
  uint8_t dmaPage = 0;
  uint16_t dmaOffset = 0;
  uint8_t dmaValue = 0;
  uint8_t dmaIdleCycles = 0;
  bool dmaReadPhase = true;

 public:
  static constexpr uint8_t JOYPAD_A = 0x01;
  static constexpr uint8_t JOYPAD_B = 0x02;
  static constexpr uint8_t JOYPAD_SELECT = 0x04;
  static constexpr uint8_t JOYPAD_START = 0x08;
  static constexpr uint8_t JOYPAD_UP = 0x10;
  static constexpr uint8_t JOYPAD_DOWN = 0x20;
  static constexpr uint8_t JOYPAD_LEFT = 0x40;
  static constexpr uint8_t JOYPAD_RIGHT = 0x80;

  Bus(const Bus&) = delete;
  Bus& operator=(const Bus&) = delete;
  Bus(Bus&&) = delete;
  Bus& operator=(Bus&&) = delete;

  Bus(PPU& ppu, Cartridge& cart)
      : cpu_ram{},
        cart(cart),
        ppu(ppu),
        cycles(0)
  {}

  inline bool ppuNMI() { return ppu.getNMI(); }
  inline uint16_t getPPUScanline() override { return ppu.getScanline(); }
  inline uint16_t getPPUCycle() override { return ppu.getCycle(); }

  inline uint64_t getCycleCount() const { return cycles; }
  inline void resetCycles() { cycles = 0; }
  bool isDMAActive() const { return dmaPending || dmaActive; }

  bool tickDMA(bool cpuReadCycle) override {
    ++cycles;
    if (dmaPending && !dmaActive && cpuReadCycle) {
      dmaPending = false;
      dmaActive = true;
      dmaOffset = 0;
      dmaReadPhase = true;
      // Halt for one cycle, plus alignment when the next cycle cannot read.
      // OAM reads use even CPU clocks and writes use odd CPU clocks.
      dmaIdleCycles = (cycles & 1) ? 1 : 2;
    }
    if (!dmaActive) {
      return false;
    }
    if (dmaIdleCycles != 0) {
      --dmaIdleCycles;
    } else if (dmaReadPhase) {
      dmaValue = read((static_cast<uint16_t>(dmaPage) << 8) | dmaOffset);
      dmaReadPhase = false;
    } else {
      ppu.write_to_oam_data(dmaValue);
      dmaReadPhase = true;
      if (++dmaOffset == 256) {
        dmaActive = false;
      }
    }
    return true;
  }
  inline void setJoypad1Buttons(uint8_t buttons) {
    joypad1Buttons = buttons;
    if (joypadStrobe) {
      joypad1Shift = joypad1Buttons;
    }
  }

  inline uint8_t read(uint16_t addr) override {
    // cycles++;
    // CPU RAM mirror: 0x0000 - 0x1FFF
    if (addr <= 0x1FFF) {
      addr &= 0b0000011111111111;  // mirror down addr
      return cpu_ram[addr];
    } else if (addr == 0x2002) {
      return ppu.read_status();
    } else if (addr == 0x2004) {
      return ppu.read_oam_data();
    } else if (addr == 0x2007) {
      return ppu.cpuRead();
    } else if ((addr >= 0x2000) && (addr <= 0x2006)) {
      // 0x2000, 0x2001, 0x2003, 0x2005, 0x2006
      // PPU READ ONLY - return last value written to 0x2000 -> 0x2007
      return ppu.lastWrittenValue();
    }
    // PPU registers mirror: 0x2008 to 0x3FFF
    else if (addr >= 0x2008 && addr <= 0x3FFF) {
      // mirror down to 0x2000-0x2007 and recurse
      return read(addr & 0x2007);
    } else if (addr >= 0x4000 && addr <= 0x4015) {
      return 0;  // apu->readRegister(addr);
    } else if (addr == 0x4016) {
      uint8_t value = 0;
      if (joypadStrobe) {
        value = joypad1Buttons & 0x01;
      } else {
        value = joypad1Shift & 0x01;
        joypad1Shift = static_cast<uint8_t>((joypad1Shift >> 1) | 0x80);
      }
      return static_cast<uint8_t>(0x40 | value);
    } else if (addr == 0x4017) {
      return 0x40;
    } else if (addr >= 0x6000 && addr <= 0x7FFF) {
      return cart.read_prg_ram(addr);
    } else if (addr >= 0x8000) {
      return cart.read_prg_rom(addr);
    } else {
      // error point / TO-DO: missing exp_rom, s_ram and apu_io
      // cartridge PRG_ROM space: 0x8000 to 0xFFFF
      return 0;
    }
  }

  inline uint8_t peek(uint16_t addr) override {
    // Side-effect-free read used for tracing/logging.
    if (addr <= 0x1FFF) {
      addr &= 0b0000011111111111;
      return cpu_ram[addr];
    } else if (addr >= 0x2000 && addr <= 0x3FFF) {
      // Nintendulator-style trace convention for I/O space.
      return 0xFF;
    } else if (addr >= 0x4000 && addr <= 0x401F) {
      // Nintendulator-style trace convention for I/O space.
      return 0xFF;
    } else if (addr >= 0x6000 && addr <= 0x7FFF) {
      return cart.read_prg_ram(addr);
    } else if (addr >= 0x8000) {
      return cart.read_prg_rom(addr);
    } else {
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
      ppu.cpuWrite(value);
    } else if (addr >= 0x2008 && addr <= 0x3FFF) {
      // PPU registers mirror: 0x2008 to 0x3FFF
      // mirror down to 0x2000-0x2007 and recurse
      write(addr & 0x2007, value);
    } else if (addr == 0x4014) {
      // Transfer begins on the next CPU read cycle. Consecutive writes from
      // read-modify-write instructions replace the pending source page.
      dmaPage = value;
      dmaPending = true;
    } else if (addr >= 0x4000 && addr <= 0x4015) {
      // apu write
    } else if (addr == 0x4016) {
      bool newStrobe = (value & 0x01) != 0;
      if (!newStrobe && joypadStrobe) {
        // Latch controller state when strobe falls.
        joypad1Shift = joypad1Buttons;
      } else if (newStrobe) {
        joypad1Shift = joypad1Buttons;
      }
      joypadStrobe = newStrobe;
    } else if (addr == 0x4017) {
      // joypad 2
    } else if (addr >= 0x6000 && addr <= 0x7FFF) {
      cart.write_prg_ram(addr, value);
    } else {
      // error point / TO-DO: missing exp_rom, s_ram and apu_io
    }
  }
};

#endif  // BUS_H
