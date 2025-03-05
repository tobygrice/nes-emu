#ifndef PPU_H
#define PPU_H

#include <vector>

#include "../MMU.h"
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
  uint8_t data_buf;  // 0x2007 buffer
  uint8_t oam_dma;   // 0x4014

  // MEMORY:
  // chr_rom and mirroring mode in cartridge, accessed via MMU (bus)
  std::array<uint8_t, 2048> vram;         // 2048 bytes of vram
  std::array<uint8_t, 256> oam;           // 256 bytes of sprite memory
  std::array<uint8_t, 32> palette_table;  // 32 bytes
  Cartridge* cart;                        // cartridge

 public:
  PPU() {}
  PPU(Cartridge* cart) : cart(cart) {}
  uint16_t mirror_vram_addr(uint16_t addr);

  // read/writes to 0x2007
  uint8_t readData();
  void writeData(uint8_t value);

  uint8_t getStatus() { return status.reg; }
  uint8_t getOam_data() { return oam_data; }

  void setCtrl(uint8_t value) { ctrl.reg = value; }
  void setMask(uint8_t value) { mask.reg = value; }
  void setOam_addr(uint8_t value) { oam_addr = value; }
  void setOam_data(uint8_t value) { oam_data = value; }
  void setScroll(uint8_t value) { scroll = value; }
  void setAddr(uint8_t value) { addr.update(value); }
};

#endif