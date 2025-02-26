#ifndef PPU_H
#define PPU_H

#include <vector>

#include "../BusInterface.h"

class PPU {
 private:
  // REGISTERS:
  // PPU control register with bitfields for readability
  union PPUCTRL {
    struct {
      uint8_t nametable : 2;      // base nametable address
      uint8_t increment : 1;      // vram address increment per CPU read/write
      uint8_t spritePattern : 1;  // sprite pattern table address
      uint8_t bgPattern : 1;      // background pattern table address
      uint8_t spriteSize : 1;
      uint8_t slaveMode : 1;
      uint8_t generateNMI : 1;
    };
    uint8_t reg;
  } ctrl;

  // mask register with bitfields
  union PPUMASK {
    struct {
      uint8_t grayscale : 1;           // greyscale mode
      uint8_t showLeftBackground : 1;  // show background in leftmost 8 pixels
      uint8_t showLeftSprites : 1;     // show sprites in leftmost 8 pixels
      uint8_t showBackground : 1;      // show background
      uint8_t showSprites : 1;         // show sprites
      uint8_t emphasizeRed : 1;        // emphasize red
      uint8_t emphasizeGreen : 1;      // emphasize green
      uint8_t emphasizeBlue : 1;       // emphasize blue
    };
    uint8_t reg;
  } mask;

  // status register with bitfields
  union PPUSTATUS {
    struct {
      uint8_t unused : 5;          // unused bits
      uint8_t spriteOverflow : 1;  // sprite overflow flag
      uint8_t spriteZeroHit : 1;   // sprite zero hit flag
      uint8_t verticalBlank : 1;   // vertical blank has started
    };
    uint8_t reg;
  } status;

  uint16_t oam_addr;  // 0x2003
  uint8_t oam_data;  // 0x2004
  uint8_t scroll;    // 0x2005
  uint16_t addr;      // 0x2006
  uint8_t data;      // 0x2007
  uint8_t oam_dma;   // 0x4014

  // ADDRESSABLE MEMORY:
  // chr_rom and mirroring mode in cartridge, accessed via MMU
  std::array<uint8_t, 2048> vram;         // 2048 bytes of vram
  std::array<uint8_t, 256> oam;           // 256 bytes of sprite memory
  std::array<uint8_t, 32> palette_table;  // 32 bytes
  BusInterface* bus;                      // bus/MMU
 public:
};

#endif