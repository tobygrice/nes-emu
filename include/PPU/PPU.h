#ifndef PPU_H
#define PPU_H

#include <vector>

#include "../BusInterface.h"
#include "BitmaskRegs.h"
#include "PPUAddr.h"

class PPU {
 private:
  // REGISTERS:
  /**
   * CTRL, MASK, and STATUS registers have a struct for explicit named access to
   * each flag for readability. The ADDR register also has a specific class to
   * allow for cycle-accurate read/writes.
   */
  PPUCtrl ctrl;      // 0x2000
  PPUMask mask;      // 0x2001
  PPUStatus status;  // 0x2002
  uint8_t oam_addr;  // 0x2003
  uint8_t oam_data;  // 0x2004
  uint8_t scroll;    // 0x2005
  PPUAddr addr;      // 0x2006
  uint8_t data;      // 0x2007
  uint8_t oam_dma;   // 0x4014

  // MEMORY:
  // chr_rom and mirroring mode in cartridge, accessed via MMU (bus)
  std::array<uint8_t, 2048> vram;         // 2048 bytes of vram
  std::array<uint8_t, 256> oam;           // 256 bytes of sprite memory
  std::array<uint8_t, 32> palette_table;  // 32 bytes
  uint8_t internal_buffer;
  BusInterface* bus;                      // bus/MMU
 public:
  inline void increment_vram_addr() { addr.increment(ctrl.get_increment()); }
  uint8_t read_data();
};

#endif