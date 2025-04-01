#ifndef PPU_H
#define PPU_H

#include <array>
#include <cstdint>
#include <vector>

#include "../Renderer/Frame.h"
#include "../Cartridge.h"
#include "Registers/PPUAddr.h"
#include "Registers/PPUCtrl.h"
#include "Registers/PPUMask.h"
#include "Registers/PPUScroll.h"
#include "Registers/PPUStatus.h"

class Renderer;

class PPU {
  friend class Renderer;

 private:
  // REGISTERS:
  /**
   * CTRL, MASK, and STATUS registers have a struct for explicit named access to
   * each flag for readability. The ADDR register also has a specific class to
   * allow for cycle-accurate read/writes.
   */
  Frame* currentFrame;
  uint8_t tileID = 0;
  uint8_t attribute = 0;
  uint8_t patternLow = 0;
  uint8_t patternHigh = 0;
  uint16_t v = 0;
  uint8_t currentYQuadrant = 0;

  PPUCtrl ctrl;      // 0x2000
  PPUMask mask;      // 0x2001
  PPUStatus status;  // 0x2002
  PPUScroll scroll;  // 0x2005
  PPUAddr addr;      // 0x2006
  uint8_t data_buf;  // 0x2007 buffer
  uint8_t oam_dma;   // 0x4014

  uint8_t oam_addr;                       // 0x2003
  std::array<uint8_t, 256> oam_data;      // 0x2004 256 bytes of sprite memory
  std::array<uint8_t, 32> palette_table;  // 32 bytes

  // chr_rom and mirroring mode in cartridge, accessed via bus
  std::array<uint8_t, 2048> vram;  // 2048 bytes of vram
  Cartridge* cart;

  uint16_t cycles;
  int scanline;  // -1 scanline
  bool oddFrame = false;
  bool nmiInterrupt;

  uint8_t last_written_value;

 public:
  PPU(Cartridge* cart)
      : ctrl(),
        mask(),
        status(),
        scroll(),
        addr(),
        data_buf(0),
        oam_dma(0),

        oam_addr(0),
        oam_data{},
        palette_table{},

        vram{},
        cart(cart),

        cycles(0),
        scanline(0),
        nmiInterrupt(false),
        last_written_value(0) {}

  Frame* tick();  // returns true if frame generation complete

  uint16_t mirrorVRAMAddress(uint16_t addr);
  uint8_t mirrorPaletteAddress(uint8_t addr) {
    addr &= 0x1F;
    if (addr == 0x10) {
      addr = 0x00;
    } else if (addr == 0x14) {
      addr = 0x04;
    } else if (addr == 0x18) {
      addr = 0x08;
    } else if (addr == 0x1C) {
      addr = 0x0C;
    }
    return addr;
  }

  bool getNMI() { return nmiInterrupt; }
  uint16_t getScanline() { return scanline; }
  uint16_t getCycle() { return cycles; }
  uint8_t lastWrittenValue() { return last_written_value; }

  // read/writes to 0x2007
  uint8_t cpuRead();
  void cpuWrite(uint8_t value);

  // register READ/WRITES:
  void write_to_ctrl(uint8_t value);
  void write_to_mask(uint8_t value);
  uint8_t read_status();
  void write_to_oam_addr(uint8_t value);
  void write_to_oam_data(uint8_t value);
  uint8_t read_oam_data() const;
  void write_to_scroll(uint8_t value);
  void write_to_ppu_addr(uint8_t value);
  void write_oam_dma(const std::array<uint8_t, 256>& data);

  uint8_t TEST_getvram(uint16_t address) { return vram[address]; }
  void TEST_setvram(uint16_t address, uint8_t value) { vram[address] = value; }
  uint8_t TEST_getstatus() { return status.snapshot(); }
  uint16_t TEST_getaddr() { return addr.get(); }
  void TEST_set_vblank_status(bool val) { status.set_vblank_status(val); }
};

#endif