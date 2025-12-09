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

Frame* PPU::tick() {
  /**  ============ BACKGROUND ============
   * cycle 1 fetch nametable byte, cycle 2 burn cycle (read takes two cycles)
   * cycle 3 fetch attribute table byte, cycle 4 burn cycle
   * cycle 5 fetch pattern table tile low, cycle 6 burn cycle
   * cycle 7 pattern table tile high (+8 bytes from pattern table tile low)
   * cycle 8 burn cycle (process 8 pixels)
   */
  if (scanline == 0) {
    // fill the shift registers with the data for the first two tiles of the
    // next scanline
    if (cycles == 1) {
      if (currentFrame) delete currentFrame;
      currentFrame = new Frame();
    }
  } else if (scanline < 240) { // 0-239
    // Note: this is not cycle accurate. In true hardware, each memory read
    // takes two cycles and there are four memory reads per 8 pixels. All of the
    // pixel rendering is done in the last cycle. However, I have decided to
    // stray from this to balance computational load across cycles.
    if (cycles < 256) {
      switch (cycles % 8) {
        case 0: {
          // fetch nametable byte
          uint16_t address = mirrorVRAMAddress(0x2000 | (v & 0x0FFF));
          tileID = vram[address];
          break;
        }
        case 1: {
          // fetch attribute table byte
          uint16_t address = mirrorVRAMAddress(
              0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07));
          attribute = vram[address];
          break;
        }
        case 2: {
          // fetch pattern table tile low
          uint8_t fineY = (v >> 12) & 0b0111;  // v register is 15 bits
          uint16_t address = ctrl.bg_pattern_addr() + (tileID * 16) + fineY;
          patternLow = cart->read_chr_rom(address);
          break;
        }
        case 3: {
          // pattern table tile high (+8 bytes from pattern table tile low)
          uint8_t fineY = (v >> 12) & 0x07;
          uint16_t address = ctrl.bg_pattern_addr() + (tileID * 16) + fineY + 8;
          patternHigh = cart->read_chr_rom(address);
          break;
        }
        case 4: {
          // compute pixels 11000000

          // determine quadrant within tile
          // (0b00 or 0b10 / top left or bottom left)
          uint8_t quadrant = currentYQuadrant << 1;

          // determine palette selection using attribute byte and quadrant
          uint8_t paletteSelection = (attribute >> (quadrant * 2)) & 0b11;

          // compute pixels 6/7 values:
          uint8_t bit7Low = (patternLow >> 7) & 0b01;
          uint8_t bit7High = (patternHigh >> 7) & 0b01;
          uint8_t px7value = (bit7High << 1) | bit7Low;

          uint8_t bit6Low = (patternLow >> 6) & 0b01;
          uint8_t bit6High = (patternHigh >> 6) & 0b01;
          uint8_t px6value = (bit6High << 1) | bit6Low;

          uint8_t px7PaletteIndex =
              mirrorPaletteAddress((paletteSelection * 4) + px7value);
          uint8_t px6PaletteIndex =
              mirrorPaletteAddress((paletteSelection * 4) + px6value);

          uint8_t px7colour = palette_table[px7PaletteIndex];
          uint8_t px6colour = palette_table[px6PaletteIndex];

          currentFrame->push(px7colour);
          currentFrame->push(px6colour);

          break;
        }
        case 5: {
          // compute pixels 00110000
          // determine quadrant within tile
          // (0b00 or 0b10 / top left or bottom left)
          uint8_t quadrant = currentYQuadrant << 1;

          // determine palette selection using attribute byte and quadrant
          uint8_t paletteSelection = (attribute >> (quadrant * 2)) & 0b11;

          // compute pixels 6/7 values:
          uint8_t bit5Low = (patternLow >> 5) & 0b01;
          uint8_t bit5High = (patternHigh >> 5) & 0b01;
          uint8_t px5value = (bit5High << 1) | bit5Low;

          uint8_t bit4Low = (patternLow >> 4) & 0b01;
          uint8_t bit4High = (patternHigh >> 4) & 0b01;
          uint8_t px4value = (bit4High << 1) | bit4Low;

          uint8_t px5PaletteIndex =
              mirrorPaletteAddress((paletteSelection * 4) + px5value);
          uint8_t px4PaletteIndex =
              mirrorPaletteAddress((paletteSelection * 4) + px4value);

          uint8_t px5colour = palette_table[px5PaletteIndex];
          uint8_t px4colour = palette_table[px4PaletteIndex];

          currentFrame->push(px5colour);
          currentFrame->push(px4colour);

          break;
        }
        case 6: {
          // compute pixels 00001100

          // determine quadrant within tile
          // (0b01 or 0b11 / top right or bottom right)
          uint8_t quadrant = (currentYQuadrant << 1) | 0b01;

          // determine palette selection using attribute byte and quadrant
          uint8_t paletteSelection = (attribute >> (quadrant * 2)) & 0b11;

          // compute pixels 6/7 values:
          uint8_t bit3Low = (patternLow >> 3) & 0b01;
          uint8_t bit3High = (patternHigh >> 3) & 0b01;
          uint8_t px3value = (bit3High << 1) | bit3Low;

          uint8_t bit2Low = (patternLow >> 2) & 0b01;
          uint8_t bit2High = (patternHigh >> 2) & 0b01;
          uint8_t px2value = (bit2High << 1) | bit2Low;

          uint8_t px3PaletteIndex =
              mirrorPaletteAddress((paletteSelection * 4) + px3value);
          uint8_t px2PaletteIndex =
              mirrorPaletteAddress((paletteSelection * 4) + px2value);

          uint8_t px3colour = palette_table[px3PaletteIndex];
          uint8_t px2colour = palette_table[px2PaletteIndex];

          currentFrame->push(px3colour);
          currentFrame->push(px2colour);

          break;
        }
        case 7: {
          // compute pixels 00000011
          // compute pixels 00001100

          // determine quadrant within tile
          // (0b01 or 0b11 / top right or bottom right)
          uint8_t quadrant = (currentYQuadrant << 1) | 0b01;

          // determine palette selection using attribute byte and quadrant
          uint8_t paletteSelection = (attribute >> (quadrant * 2)) & 0b11;

          // compute pixels 6/7 values:
          uint8_t bit1Low = (patternLow >> 1) & 0b01;
          uint8_t bit1High = (patternHigh >> 1) & 0b01;
          uint8_t px1value = (bit1High << 1) | bit1Low;

          uint8_t bit0Low = patternLow & 0b01;
          uint8_t bit0High = patternHigh & 0b01;
          uint8_t px0value = (bit0High << 1) | bit0Low;

          uint8_t px1PaletteIndex =
              mirrorPaletteAddress((paletteSelection * 4) + px1value);
          uint8_t px0PaletteIndex =
              mirrorPaletteAddress((paletteSelection * 4) + px0value);

          uint8_t px1colour = palette_table[px1PaletteIndex];
          uint8_t px0colour = palette_table[px0PaletteIndex];

          currentFrame->push(px1colour);
          currentFrame->push(px0colour);

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
    // 240 is idle scanline, nothing happens
    // vblank is triggered on second tick of scanline 241
    if (cycles == 1) {
      status.set_vblank_status(true);
      status.set_sprite_zero_hit(false);
      // trigger vblank
      if (ctrl.generate_vblank_nmi()) {
        nmiInterrupt = true;
      }
      cycles++;
      return currentFrame;
    }
  } else if (scanline == 262) {
    // prerender scanline
    status.set_sprite_overflow(false);
    status.set_sprite_zero_hit(false);
    status.set_vblank_status(false);
    nmiInterrupt = false;
    cycles = 0;
    scanline = 0;
    return nullptr;
  }

  cycles++;
  if (cycles == 341) {
    cycles = 0;
    scanline++;
    currentYQuadrant = ((scanline % 8) < 4) ? 0 : 1;
  }
  return nullptr; // return nullptr until frame completed
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
    return palette_table[mirrorPaletteAddress(index)];
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
    palette_table[mirrorPaletteAddress(index)] = value;
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