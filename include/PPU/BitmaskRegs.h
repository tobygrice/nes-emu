#ifndef BITMASKREGS_H
#define BITMASKREGS_H

#include <cstdint>

/**
 * PPU Control Register (0x2000)
 *
 * 7  bit  0
 * ---- ----
 * VPHB SINN
 * |||| ||||
 * |||| ||++- Base nametable address
 * |||| ||    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
 * |||| |+--- VRAM address increment per CPU read/write of PPUDATA
 * |||| |     (0: add 1, going across; 1: add 32, going down)
 * |||| +---- Sprite pattern table address for 8x8 sprites
 * ||||       (0: $0000; 1: $1000; ignored in 8x16 mode)
 * |||+------ Background pattern table address (0: $0000; 1: $1000)
 * ||+------- Sprite size (0: 8x8 pixels; 1: 8x16 pixels – see PPU OAM#Byte 1)
 * |+-------- PPU master/slave select
 * |          (0: read backdrop from EXT pins; 1: output color on EXT pins)
 * +--------- Vblank NMI enable (0: off, 1: on)
 */
struct PPUCTRL {
  uint8_t reg = 0;  // Full 8-bit register

  uint8_t nametable() const { return reg & 0b00000011; }
  void set_nametable(uint8_t value) {
    reg = (reg & ~0b00000011) | (value & 0b00000011);
  }

  bool increment() const { return reg & 0b00000100; }
  void set_increment(bool value) {
    reg = (reg & ~0b00000100) | (value ? 0b00000100 : 0);
  }

  bool spritePattern() const { return reg & 0b00001000; }
  void set_spritePattern(bool value) {
    reg = (reg & ~0b00001000) | (value ? 0b00001000 : 0);
  }

  bool bgPattern() const { return reg & 0b00010000; }
  void set_bgPattern(bool value) {
    reg = (reg & ~0b00010000) | (value ? 0b00010000 : 0);
  }

  bool spriteSize() const { return reg & 0b00100000; }
  void set_spriteSize(bool value) {
    reg = (reg & ~0b00100000) | (value ? 0b00100000 : 0);
  }

  bool slaveMode() const { return reg & 0b01000000; }
  void set_slaveMode(bool value) {
    reg = (reg & ~0b01000000) | (value ? 0b01000000 : 0);
  }

  bool generateNMI() const { return reg & 0b10000000; }
  void set_generateNMI(bool value) {
    reg = (reg & ~0b10000000) | (value ? 0b10000000 : 0);
  }
};

/**
 * PPU Mask Register (0x2001)
 *
 * 7  bit  0
 * ---- ----
 * BGRs bMmG
 * |||| ||||
 * |||| |||+- Greyscale (0: normal color, 1: greyscale)
 * |||| ||+-- 1: Show background in leftmost 8 pixels of screen, 0: Hide
 * |||| |+--- 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
 * |||| +---- 1: Enable background rendering
 * |||+------ 1: Enable sprite rendering
 * ||+------- Emphasize red (green on PAL/Dendy)
 * |+-------- Emphasize green (red on PAL/Dendy)
 * +--------- Emphasize blue
 */
struct PPUMASK {
  uint8_t reg = 0;

  bool grayscale() const { return reg & 0b00000001; }
  void set_grayscale(bool value) {
    reg = (reg & ~0b00000001) | (value ? 0b00000001 : 0);
  }

  bool showLeftBackground() const { return reg & 0b00000010; }
  void set_showLeftBackground(bool value) {
    reg = (reg & ~0b00000010) | (value ? 0b00000010 : 0);
  }

  bool showLeftSprites() const { return reg & 0b00000100; }
  void set_showLeftSprites(bool value) {
    reg = (reg & ~0b00000100) | (value ? 0b00000100 : 0);
  }

  bool showBackground() const { return reg & 0b00001000; }
  void set_showBackground(bool value) {
    reg = (reg & ~0b00001000) | (value ? 0b00001000 : 0);
  }

  bool showSprites() const { return reg & 0b00010000; }
  void set_showSprites(bool value) {
    reg = (reg & ~0b00010000) | (value ? 0b00010000 : 0);
  }

  bool emphasizeRed() const { return reg & 0b00100000; }
  void set_emphasizeRed(bool value) {
    reg = (reg & ~0b00100000) | (value ? 0b00100000 : 0);
  }

  bool emphasizeGreen() const { return reg & 0b01000000; }
  void set_emphasizeGreen(bool value) {
    reg = (reg & ~0b01000000) | (value ? 0b01000000 : 0);
  }

  bool emphasizeBlue() const { return reg & 0b10000000; }
  void set_emphasizeBlue(bool value) {
    reg = (reg & ~0b10000000) | (value ? 0b10000000 : 0);
  }
};

/** PPU Status Register (0x2002)
 *  7  bit  0
 * ---- ----
 * VSOx xxxx
 * |||| ||||
 * |||+-++++- (PPU open bus or 2C05 PPU identifier)
 * ||+------- Sprite overflow flag
 * |+-------- Sprite 0 hit flag
 * +--------- Vblank flag, cleared on read.
 */

struct PPUSTATUS {
  uint8_t reg = 0;

  bool spriteOverflow() const { return reg & 0b00100000; }
  void set_spriteOverflow(bool value) {
    reg = (reg & ~0b00100000) | (value ? 0b00100000 : 0);
  }

  bool spriteZeroHit() const { return reg & 0b01000000; }
  void set_spriteZeroHit(bool value) {
    reg = (reg & ~0b01000000) | (value ? 0b01000000 : 0);
  }

  bool vblank() const { return reg & 0b10000000; }
  void set_vblank(bool value) {
    reg = (reg & ~0b10000000) | (value ? 0b10000000 : 0);
  }
};

#endif