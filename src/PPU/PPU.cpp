#include "../../include/PPU/PPU.h"

uint8_t PPU::readData() {
  uint16_t addr_val = addr.get();
  // increment addr according to flag in ctrl (error point)
  addr.increment(ctrl.get_increment() ? 32 : 1);

  if (addr_val <= 0x1FFF) {
    // CHR-ROM read
    uint8_t result = data_buf;
    data_buf = cart->read_chr_rom(addr_val);
    return result;
  } else if (addr_val <= 0x2FFF) {
    // RAM read
    uint8_t result = data_buf;
    data_buf = vram[mirror_vram_addr(addr_val)];
    return result;
  } else if (addr_val >= 0x3F00 && addr_val <= 0x3FFF) {
    // palette table read
    data_buf = palette_table[addr_val - 0x3F00];
    return data_buf;
  } else {
    throw std::runtime_error("PPU attempt to read from unsupported address: " +
                             std::to_string(addr_val));
  }
}

void PPU::writeData(uint8_t value) {
  uint16_t addr_val = addr.get();
  // increment addr according to flag in ctrl (error point)
  addr.increment(ctrl.get_increment() ? 32 : 1);

  if (addr_val <= 0x1FFF) {
      // error point: write to CHR-RAM??
  } else if (addr_val <= 0x2FFF) {
      vram[mirror_vram_addr(addr_val)] = value;
  } else if (addr_val >= 0x3F00 && addr_val <= 0x3FFF) {
      palette_table[addr_val - 0x3F00] = value;
  } else {
      throw std::runtime_error("PPU attempt to write to unsupported address: " +
                               std::to_string(addr_val));
  }
}

/**
   * NES uses 1 KiB of VRAM to represent a screen state
   * 2 KiB of VRAM means NES can keep a state of 2 screens
   *
   * Range [0x2000...0x3F00] (4 KiB) is reserved for Nametables (screens states)
   * Two additional screens have to be mapped to existing ones. Mirroring type
   * in header of iNES files indicates the way they are mapped.
   *
   * Horizontal:
   *    [ A ] [ a ]
   *    [ B ] [ b ]
   * Vertical:
   *    [ A ] [ B ]
   *    [ a ] [ b ]
   *
   */
uint16_t PPU::mirror_vram_addr(uint16_t addr) {
  uint16_t vram_index = (addr & 0b10111111111111) - 0x2000;
  uint16_t name_table = vram_index / 0x400;
  
  MirroringMode mirroring = cart->getMirroring();
  if (mirroring == MirroringMode::Vertical) {
    if (name_table == 2 || name_table == 3) vram_index -= 0x800;
  } else if (mirroring == MirroringMode::Horizontal) {
    if (name_table == 1 || name_table == 2)
      vram_index -= 0x400;
    else if (name_table == 3)
      vram_index -= 0x800;
  }
  return vram_index;
}