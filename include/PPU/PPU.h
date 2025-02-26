#ifndef PPU_H
#define PPU_H

#include <vector>

#include "../BusInterface.h"
#include "AddrRegister.h"
#include "BitmaskRegs.h"

class PPU {
 private:
  // REGISTERS:
  /**
   * CTRL, MASK, and STATUS registers have a struct for explicit named access to
   * each flag for readability. ADDR also has a specific class to allow for 
   * cycle-accurate read/writes.
   */
  PPUCTRL ctrl;       // 0x2000
  PPUMASK mask;       // 0x2001
  PPUSTATUS status;   // 0x2002
  uint8_t oam_addr;   // 0x2003
  uint8_t oam_data;   // 0x2004
  uint8_t scroll;     // 0x2005
  AddrRegister addr;  // 0x2006
  uint8_t data;       // 0x2007
  uint8_t oam_dma;    // 0x4014

  // ADDRESSABLE MEMORY:
  // chr_rom and mirroring mode in cartridge, accessed via MMU
  std::array<uint8_t, 2048> vram;         // 2048 bytes of vram
  std::array<uint8_t, 256> oam;           // 256 bytes of sprite memory
  std::array<uint8_t, 32> palette_table;  // 32 bytes
  BusInterface* bus;                      // bus/MMU
 public:
};

#endif