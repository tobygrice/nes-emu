#include "../../include/PPU/PPU.h"

#include "../../include/Renderer/Frame.h"

/**
 * | Address Range   | Size  | Description               | Mapped by         |
 * |-----------------|-------|---------------------------|-------------------|
 * | $0000–$0FFF     | $1000 | Pattern table 0           | Cartridge         |
 * | $1000–$1FFF     | $1000 | Pattern table 1           | Cartridge         |
 * | $2000–$23FF     | $0400 | Nametable 0               | Cartridge         |
 * | $2400–$27FF     | $0400 | Nametable 1               | Cartridge         |
 * | $2800–$2BFF     | $0400 | Nametable 2               | Cartridge         |
 * | $2C00–$2FFF     | $0400 | Nametable 3               | Cartridge         |
 * | $3000–$3EFF     | $0F00 | Unused                    | Cartridge         |
 * | $3F00–$3F1F     | $0020 | Palette RAM indexes       | Internal to PPU   |
 * | $3F20–$3FFF     | $00E0 | Mirrors of $3F00–$3F1F    | Internal to PPU   |
 */

void PPU::tick() {
  if (cycles == 0) {
    cycles++;
    return;  // idle cycle on odd frames
  }
  /**  ============ BACKGROUND ============
   * cycle 1 fetch nametable byte, cycle 2 burn cycle (read takes two cycles)
   * cycle 3 fetch attribute table byte, cycle 4 burn cycle
   * cycle 5 fetch pattern table tile low, cycle 6 burn cycle
   * cycle 7 pattern table tile high (+8 bytes from pattern table tile low)
   * cycle 8 burn cycle (process 8 pixels)
   */
  if (scanline == -1) {
    // fill the shift registers with the data for the first two tiles of the
    // next scanline
  } else if (scanline <= 239) {
    if (cycles <= 256) {
      switch ((cycles - 1) % 8) {
        case 0: {
          // fetch nametable byte
          break;
        }
        case 1: {
          // burn
          break;
        }
        case 2: {
          // fetch attribute table byte
          break;
        }
        case 3: {
          // burn
          break;
        }
        case 4: {
          // pattern table tile low
          break;
        }
        case 5: {
          // burn
          break;
        }
        case 6: {
          // pattern table tile high (+8 bytes from pattern table tile low)
          break;
        }
        case 7: {
          // burn
          break;
        }
      }
    } else if (cycles <= 320) {
      // fetch tile data for sprites on the next scanline
    } else if (cycles <= 336) {
      // fetch first two tiles for the next scanline
    } else if (cycles <= 340) {
      // two bytes are fetched, but the purpose for this is unknown
    }
  } else if (scanline == 241) {
    // 240 is idle scanline
    // vblank is triggered on second tick of scanline 241
    if (cycles == 1) {
      status.set_vblank_status(true);
      status.set_sprite_zero_hit(false);
      // trigger vblank
      if (ctrl.generate_vblank_nmi()) {
        nmiInterrupt = true;
      }
    }
  } else if (scanline == 261) {
    // clear vblank
    status.set_vblank_status(false);
    nmiInterrupt = false;
    cycles = 0;
    scanline = -1;
    return;
  }

  cycles++;
  if (cycles == 341) {
    cycles = 0;
    scanline++;
  }
}

uint8_t PPU::cpuRead() {
  uint16_t addr_val = addr.get();
  addr.increment(ctrl.vram_addr_increment());

  if (addr_val <= 0x1FFF) {
    // CHR-ROM read
    uint8_t result = data_buf;
    data_buf = cart->read_chr_rom(addr_val);
    return result;
  } else if (addr_val <= 0x3EFF) {
    // RAM read
    addr_val &= 0x2FFF;  // mirror down address
    uint8_t result = data_buf;
    data_buf = vram[mirrorVRAMAddress(addr_val)];
    return result;
  } else if (addr_val >= 0x3F00 && addr_val <= 0x3FFF) {
    // palette table read
    uint8_t index = (addr_val - 0x3F00) & 0x1F;
    if (index == 0x10)
      index = 0x00;
    else if (index == 0x14)
      index = 0x04;
    else if (index == 0x18)
      index = 0x08;
    else if (index == 0x1C)
      index = 0x0C;

    return palette_table[index];
  } else {
    throw std::runtime_error("PPU attempt to read from unsupported address: " +
                             std::to_string(addr_val));
  }
}

void PPU::cpuWrite(uint8_t value) {
  last_written_value = value;

  uint16_t addr_val = addr.get();
  addr.increment(ctrl.vram_addr_increment());

  if (addr_val <= 0x1FFF) {
    // error point: write to CHR-RAM??
  } else if (addr_val <= 0x3EFF) {
    addr_val &= 0x2FFF;  // mirror down address
    vram[mirrorVRAMAddress(addr_val)] = value;
  } else if (addr_val >= 0x3F00 && addr_val <= 0x3FFF) {
    // palette table read
    uint8_t index = (addr_val - 0x3F00) & 0x1F;
    if (index == 0x10)
      index = 0x00;
    else if (index == 0x14)
      index = 0x04;
    else if (index == 0x18)
      index = 0x08;
    else if (index == 0x1C)
      index = 0x0C;

    palette_table[index] = value;
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
uint16_t PPU::mirrorVRAMAddress(uint16_t addr) {
  addr &= 0x0FFF;
  switch (cart->getMirroring()) {
    case MirroringMode::Vertical: {
      // Vertical mirroring layout:
      //   NT0: $2000-$23FF and $2800-$2BFF → maps to first 1K (0x000–0x3FF)
      //   NT1: $2400-$27FF and $2C00-$2FFF → maps to second 1K (0x400–0x7FF)
      if (addr < 0x400 || (addr >= 0x800 && addr < 0xC00)) {
        return addr % 0x400;  // NT0
      } else {
        return (addr % 0x400) + 0x400;  // NT1
      }
      break;
    }
    case MirroringMode::Horizontal: {
      // Horizontal mirroring layout:
      //   NT0: $2000-$23FF and $2400-$27FF → both map to first 1K (0x000–0x3FF)
      //   NT1: $2800-$2BFF and $2C00-$2FFF → both map to second 1K
      //   (0x400–0x7FF)
      if (addr < 0x800) {
        // Addresses $2000-$27FF (0x000–0x7FF when reduced) map to NT0.
        return addr % 0x400;
      } else {
        // Addresses $2800-$2FFF map to NT1.
        return (addr % 0x400) + 0x400;
      }
      break;
    }
    case MirroringMode::FourScreen: {
      // Four-screen mirroring means each of the four nametables is unique.
      // In a proper four-screen setup, additional VRAM is needed.
      // If our VRAM array is only 2 KiB, we’ll simply wrap the address into
      // that range.
      return addr % 0x800;
      break;
    }
    default: {
      throw std::runtime_error(
          "PPU attempted to mirror VRAM address, but no mirroring mode is "
          "set.");
    }
  }
}

/**
 * ========================================================================
 * REGISTER READ/WRITES
 * ========================================================================
 */
void PPU::write_to_ctrl(uint8_t value) {
  last_written_value = value;
  bool priorNMI = ctrl.generate_vblank_nmi();
  ctrl.update(value);
  if (!priorNMI && ctrl.generate_vblank_nmi() && status.is_in_vblank()) {
    nmiInterrupt = true;
  }
}

// Updates the mask register.
void PPU::write_to_mask(uint8_t value) {
  last_written_value = value;
  mask.update(value);
}

// reads the status register snapshot and resets various latches.
uint8_t PPU::read_status() {
  uint8_t data = status.snapshot();
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