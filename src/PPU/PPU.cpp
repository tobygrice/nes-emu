#include "../../include/PPU/PPU.h"

#include "../../include/Renderer/Frame.h"

bool PPU::tick(uint8_t c) {
  // error point: PPU should drop a cycle on odd frames
  cycles += c;
  if (cycles >= 341) {
    cycles -= 341;
    scanline++;

    // scanline start at -1?
    if (scanline == 241) {
      // error point: may need to set vblank if >= 261
      status.set_vblank_status(true);
      status.set_sprite_zero_hit(true); // error point - true/false?
      if (ctrl.generate_vblank_nmi()) {
        nmiInterrupt = true;
      }
    }

    if (scanline >= 262) {
      // error point: may need to set vblank if == 261
      scanline = 0;
      nmiInterrupt = false;
      status.set_sprite_zero_hit(false);
      status.set_vblank_status(false);
      return true;  // frame complete
    }
  }
  return false;
}

uint8_t PPU::read_data() {
  uint16_t addr_val = addr.get();
  addr.increment(ctrl.vram_addr_increment());

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
  } else if (addr_val == 0x3F10 || addr_val == 0x3F14 || addr_val == 0x3F18 ||
             addr_val == 0x3F1C) {
    // addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C
    return palette_table[(addr_val - 0x10 - 0x3F00)];
  } else if (addr_val >= 0x3F00 && addr_val <= 0x3FFF) {
    // palette table read
    return palette_table[addr_val - 0x3F00];
  } else {
    throw std::runtime_error("PPU attempt to read from unsupported address: " +
                             std::to_string(addr_val));
  }
}

void PPU::write_to_data(uint8_t value) {
  last_written_value = value;

  uint16_t addr_val = addr.get();

  if (addr_val <= 0x1FFF) {
    // error point: write to CHR-RAM??
  } else if (addr_val <= 0x2FFF) {
    vram[mirror_vram_addr(addr_val)] = value;
  } else if (addr_val == 0x3F10 || addr_val == 0x3F14 || addr_val == 0x3F18 ||
             addr_val == 0x3F1C) {
    // addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C
    palette_table[(addr_val - 0x10 - 0x3F00)] = value;
  } else if (addr_val >= 0x3F00 && addr_val <= 0x3FFF) {
    palette_table[addr_val - 0x3F00] = value;
  } else {
    throw std::runtime_error("PPU attempt to write to unsupported address: " +
                             std::to_string(addr_val));
  }

  addr.increment(ctrl.vram_addr_increment());
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

/**
 * ========================================================================
 * REGISTER READ/WRITES
 * ========================================================================
 */
void PPU::write_to_ctrl(uint8_t value) {
  last_written_value = value;
  bool before_nmi_status = ctrl.generate_vblank_nmi();
  ctrl.update(value);
  // If the NMI flag just became set and we're in vblank, set the interrupt.
  if (!before_nmi_status && ctrl.generate_vblank_nmi() &&
      status.is_in_vblank()) {
    nmiInterrupt = true;
  }
}

// Updates the mask register.
void PPU::write_to_mask(uint8_t value) {
  last_written_value = value;
  mask.update(value);
}

// Reads the status register snapshot and resets various latches.
uint8_t PPU::read_status() {
  uint8_t data = 0;
  data += status.snapshot();
  status.set_vblank_status(false);
  addr.reset_latch();
  scroll.reset_latch();
  return data;
}

// Writes a value to the OAM (Object Attribute Memory) address.
void PPU::write_to_oam_addr(uint8_t value) {
  last_written_value = value;
  oam_addr = value;
}

// Writes a value to the OAM data at the current address and increments the
// address.
void PPU::write_to_oam_data(uint8_t value) {
  last_written_value = value;
  oam_data[oam_addr] = value;
  oam_addr = static_cast<uint8_t>(oam_addr + 1);
}

// Reads the OAM data at the current address.
uint8_t PPU::read_oam_data() const { return oam_data[oam_addr]; }

// Writes to the scroll register.
void PPU::write_to_scroll(uint8_t value) {
  last_written_value = value;
  scroll.write(value);
}

// Writes to the PPU address register.
void PPU::write_to_ppu_addr(uint8_t value) {
  last_written_value = value;
  addr.update(value);
}

// Writes 256 bytes of DMA data to the OAM, starting at the current address.
void PPU::write_oam_dma(const std::array<uint8_t, 256>& data) {
  for (const auto& x : data) {
    oam_data[oam_addr] = x;
    oam_addr = static_cast<uint8_t>(oam_addr + 1);
  }
}