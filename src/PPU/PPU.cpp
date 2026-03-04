#include "../../include/PPU/PPU.h"

#include "../../include/Renderer/Frame.h"

namespace {

inline void setFramePixel(Frame& frame, int x, int y, uint8_t colour) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
    return;
  }

  const std::size_t index =
      (static_cast<std::size_t>(y) * SCREEN_WIDTH + static_cast<std::size_t>(x)) * 3;
  if (index + 2 >= frame.pixelData.size()) {
    return;
  }

  uint8_t r, g, b;
  std::tie(r, g, b) = NES_PALETTE[colour & 0x3F];
  frame.pixelData[index] = r;
  frame.pixelData[index + 1] = g;
  frame.pixelData[index + 2] = b;
}

inline bool frameBackgroundPixelOpaque(const Frame& frame, int x, int y) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
    return false;
  }

  const std::size_t index =
      (static_cast<std::size_t>(y) * SCREEN_WIDTH + static_cast<std::size_t>(x));
  if (index >= frame.backgroundOpaque.size()) {
    return false;
  }
  return frame.backgroundOpaque[index] != 0;
}

}  // namespace

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

std::unique_ptr<Frame> PPU::tick() {
  /**  ============ BACKGROUND ============
   * cycle 1 fetch nametable byte, cycle 2 burn cycle (read takes two cycles)
   * cycle 3 fetch attribute table byte, cycle 4 burn cycle
   * cycle 5 fetch pattern table tile low, cycle 6 burn cycle
   * cycle 7 pattern table tile high (+8 bytes from pattern table tile low)
   * cycle 8 burn cycle (process 8 pixels)
   */
  if (scanline == 0 && cycles == 1) {
    currentFrame = std::make_unique<Frame>();
  }

  if (scanline < 240) {  // visible scanlines 0-239
    // Note: this is not cycle accurate. In true hardware, each memory read
    // takes two cycles and there are four memory reads per 8 pixels. All of the
    // pixel rendering is done in the last cycle. However, I have decided to
    // stray from this to balance computational load across cycles.
    if (cycles < 256) {
      // Approximate coarse/fine scroll and nametable selection for background
      // fetches.
      const int scrolledX = static_cast<int>(cycles) + scroll.scroll_x;
      const int scrolledY = scanline + scroll.scroll_y;

      const int ntXOffset = scrolledX / SCREEN_WIDTH;
      const int ntYOffset = scrolledY / SCREEN_HEIGHT;

      const uint8_t pixelX = static_cast<uint8_t>(scrolledX % SCREEN_WIDTH);
      const uint8_t pixelY = static_cast<uint8_t>(scrolledY % SCREEN_HEIGHT);

      const uint8_t coarseX = static_cast<uint8_t>(pixelX >> 3);
      const uint8_t coarseY = static_cast<uint8_t>(pixelY >> 3);
      const uint8_t fineY = static_cast<uint8_t>(pixelY & 0x07);

      const uint16_t baseNametable = ctrl.nametable_addr();
      const uint8_t baseNtX =
          static_cast<uint8_t>((baseNametable == 0x2400 || baseNametable == 0x2C00) ? 1 : 0);
      const uint8_t baseNtY = static_cast<uint8_t>((baseNametable >= 0x2800) ? 1 : 0);

      const uint8_t ntX = static_cast<uint8_t>((baseNtX + ntXOffset) & 0x01);
      const uint8_t ntY = static_cast<uint8_t>((baseNtY + ntYOffset) & 0x01);
      const uint16_t effectiveNametableBase =
          static_cast<uint16_t>(0x2000 + (((ntY << 1) | ntX) * 0x400));

      const uint16_t nametableAddress = mirrorVRAMAddress(
          static_cast<uint16_t>(effectiveNametableBase + (coarseY * 32) + coarseX));
      const uint16_t attributeAddress = mirrorVRAMAddress(
          static_cast<uint16_t>(effectiveNametableBase + 0x03C0 +
                                ((coarseY / 4) * 8) + (coarseX / 4)));
      const uint8_t attributeQuadrant = static_cast<uint8_t>(
          ((coarseY & 0x02) ? 2 : 0) | ((coarseX & 0x02) ? 1 : 0));

      switch (cycles % 8) {
        case 0: {
          // fetch nametable byte
          tileID = vram[nametableAddress];
          break;
        }
        case 1: {
          // fetch attribute table byte
          attribute = vram[attributeAddress];
          break;
        }
        case 2: {
          // fetch pattern table tile low
          uint16_t address = ctrl.bg_pattern_addr() + (tileID * 16) + fineY;
          patternLow = cart.read_chr_rom(address);
          break;
        }
        case 3: {
          // pattern table tile high (+8 bytes from pattern table tile low)
          uint16_t address = ctrl.bg_pattern_addr() + (tileID * 16) + fineY + 8;
          patternHigh = cart.read_chr_rom(address);
          break;
        }
        case 4: {
          // compute pixels 11000000
          uint8_t paletteSelection =
              (attribute >> (attributeQuadrant * 2)) & 0b11;

          // compute pixels 6/7 values:
          uint8_t bit7Low = (patternLow >> 7) & 0b01;
          uint8_t bit7High = (patternHigh >> 7) & 0b01;
          uint8_t px7value = (bit7High << 1) | bit7Low;

          uint8_t bit6Low = (patternLow >> 6) & 0b01;
          uint8_t bit6High = (patternHigh >> 6) & 0b01;
          uint8_t px6value = (bit6High << 1) | bit6Low;

          if (!mask.show_background() ||
              (!mask.leftmost_8pxl_background() && cycles < 8)) {
            px7value = 0;
            px6value = 0;
          }

          uint8_t px7PaletteIndex = mirrorPaletteAddress(
              static_cast<uint8_t>(px7value == 0 ? 0
                                                 : (paletteSelection * 4) + px7value));
          uint8_t px6PaletteIndex = mirrorPaletteAddress(
              static_cast<uint8_t>(px6value == 0 ? 0
                                                 : (paletteSelection * 4) + px6value));

          uint8_t px7colour = palette_table[px7PaletteIndex];
          uint8_t px6colour = palette_table[px6PaletteIndex];

          currentFrame->push(px7colour, px7value != 0);
          currentFrame->push(px6colour, px6value != 0);

          break;
        }
        case 5: {
          // compute pixels 00110000
          uint8_t paletteSelection =
              (attribute >> (attributeQuadrant * 2)) & 0b11;

          // compute pixels 6/7 values:
          uint8_t bit5Low = (patternLow >> 5) & 0b01;
          uint8_t bit5High = (patternHigh >> 5) & 0b01;
          uint8_t px5value = (bit5High << 1) | bit5Low;

          uint8_t bit4Low = (patternLow >> 4) & 0b01;
          uint8_t bit4High = (patternHigh >> 4) & 0b01;
          uint8_t px4value = (bit4High << 1) | bit4Low;

          if (!mask.show_background() ||
              (!mask.leftmost_8pxl_background() && cycles < 8)) {
            px5value = 0;
            px4value = 0;
          }

          uint8_t px5PaletteIndex = mirrorPaletteAddress(
              static_cast<uint8_t>(px5value == 0 ? 0
                                                 : (paletteSelection * 4) + px5value));
          uint8_t px4PaletteIndex = mirrorPaletteAddress(
              static_cast<uint8_t>(px4value == 0 ? 0
                                                 : (paletteSelection * 4) + px4value));

          uint8_t px5colour = palette_table[px5PaletteIndex];
          uint8_t px4colour = palette_table[px4PaletteIndex];

          currentFrame->push(px5colour, px5value != 0);
          currentFrame->push(px4colour, px4value != 0);

          break;
        }
        case 6: {
          // compute pixels 00001100
          uint8_t paletteSelection =
              (attribute >> (attributeQuadrant * 2)) & 0b11;

          // compute pixels 6/7 values:
          uint8_t bit3Low = (patternLow >> 3) & 0b01;
          uint8_t bit3High = (patternHigh >> 3) & 0b01;
          uint8_t px3value = (bit3High << 1) | bit3Low;

          uint8_t bit2Low = (patternLow >> 2) & 0b01;
          uint8_t bit2High = (patternHigh >> 2) & 0b01;
          uint8_t px2value = (bit2High << 1) | bit2Low;

          if (!mask.show_background() ||
              (!mask.leftmost_8pxl_background() && cycles < 8)) {
            px3value = 0;
            px2value = 0;
          }

          uint8_t px3PaletteIndex = mirrorPaletteAddress(
              static_cast<uint8_t>(px3value == 0 ? 0
                                                 : (paletteSelection * 4) + px3value));
          uint8_t px2PaletteIndex = mirrorPaletteAddress(
              static_cast<uint8_t>(px2value == 0 ? 0
                                                 : (paletteSelection * 4) + px2value));

          uint8_t px3colour = palette_table[px3PaletteIndex];
          uint8_t px2colour = palette_table[px2PaletteIndex];

          currentFrame->push(px3colour, px3value != 0);
          currentFrame->push(px2colour, px2value != 0);

          break;
        }
        case 7: {
          // compute pixels 00000011
          uint8_t paletteSelection =
              (attribute >> (attributeQuadrant * 2)) & 0b11;

          // compute pixels 6/7 values:
          uint8_t bit1Low = (patternLow >> 1) & 0b01;
          uint8_t bit1High = (patternHigh >> 1) & 0b01;
          uint8_t px1value = (bit1High << 1) | bit1Low;

          uint8_t bit0Low = patternLow & 0b01;
          uint8_t bit0High = patternHigh & 0b01;
          uint8_t px0value = (bit0High << 1) | bit0Low;

          if (!mask.show_background() ||
              (!mask.leftmost_8pxl_background() && cycles < 8)) {
            px1value = 0;
            px0value = 0;
          }

          uint8_t px1PaletteIndex = mirrorPaletteAddress(
              static_cast<uint8_t>(px1value == 0 ? 0
                                                 : (paletteSelection * 4) + px1value));
          uint8_t px0PaletteIndex = mirrorPaletteAddress(
              static_cast<uint8_t>(px0value == 0 ? 0
                                                 : (paletteSelection * 4) + px0value));

          uint8_t px1colour = palette_table[px1PaletteIndex];
          uint8_t px0colour = palette_table[px0PaletteIndex];

          currentFrame->push(px1colour, px1value != 0);
          currentFrame->push(px0colour, px0value != 0);

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
      if (currentFrame && mask.show_sprites()) {
        // Draw sprites over the background frame after visible scanlines are
        // complete. This is not cycle-accurate but is sufficient for visual
        // output.
        const uint8_t spriteHeight = ctrl.sprite_size();

        // Render back-to-front so lower OAM indices have priority.
        for (int sprite = 63; sprite >= 0; --sprite) {
          const int base = sprite * 4;
          const int spriteY = static_cast<int>(oam_data[base]) + 1;
          const uint8_t tileIndex = oam_data[base + 1];
          const uint8_t attributes = oam_data[base + 2];
          const int spriteX = static_cast<int>(oam_data[base + 3]);

          const bool flipHorizontal = (attributes & 0x40) != 0;
          const bool flipVertical = (attributes & 0x80) != 0;
          const bool behindBackground = (attributes & 0x20) != 0;
          const uint8_t paletteSelection = attributes & 0x03;

          for (int py = 0; py < spriteHeight; py++) {
            const int screenY = spriteY + py;
            if (screenY < 0 || screenY >= SCREEN_HEIGHT) {
              continue;
            }

            const int spriteRow = flipVertical ? (spriteHeight - 1 - py) : py;

            uint16_t patternBase = 0;
            uint8_t tileNumber = tileIndex;
            uint8_t fineY = 0;

            if (spriteHeight == 16) {
              patternBase = (tileIndex & 0x01) ? 0x1000 : 0x0000;
              tileNumber = static_cast<uint8_t>(tileIndex & 0xFE);
              if (spriteRow >= 8) {
                tileNumber = static_cast<uint8_t>(tileNumber + 1);
                fineY = static_cast<uint8_t>(spriteRow - 8);
              } else {
                fineY = static_cast<uint8_t>(spriteRow);
              }
            } else {
              patternBase = ctrl.sprite_pattern_addr();
              fineY = static_cast<uint8_t>(spriteRow & 0x07);
            }

            const uint16_t patternAddress =
                static_cast<uint16_t>(patternBase + (tileNumber * 16) + fineY);
            const uint8_t patternLow = cart.read_chr_rom(patternAddress);
            const uint8_t patternHigh = cart.read_chr_rom(patternAddress + 8);

            for (int px = 0; px < 8; px++) {
              const int screenX = spriteX + px;
              if (screenX < 0 || screenX >= SCREEN_WIDTH) {
                continue;
              }
              if (!mask.leftmost_8pxl_sprite() && screenX < 8) {
                continue;
              }

              const int bit = flipHorizontal ? px : (7 - px);
              const uint8_t lowBit = (patternLow >> bit) & 0x01;
              const uint8_t highBit = (patternHigh >> bit) & 0x01;
              const uint8_t spritePixel = (highBit << 1) | lowBit;
              if (spritePixel == 0) {
                continue;
              }

              if (behindBackground && mask.show_background()) {
                // Behind-background sprites only appear through transparent
                // background pixels.
                if (frameBackgroundPixelOpaque(*currentFrame, screenX,
                                               screenY)) {
                  continue;
                }
              }

              const uint8_t paletteIndex = mirrorPaletteAddress(
                  static_cast<uint8_t>(0x10 + (paletteSelection * 4) + spritePixel));
              const uint8_t spriteColour = palette_table[paletteIndex];
              setFramePixel(*currentFrame, screenX, screenY, spriteColour);
            }
          }
        }
      }

      if (!suppressVblankThisFrame) {
        status.set_vblank_status(true);
        status.set_sprite_zero_hit(false);
        // trigger vblank
        if (ctrl.generate_vblank_nmi()) {
          nmiInterrupt = true;
        }
      }
      suppressVblankThisFrame = false;
      cycles++;
      return std::move(currentFrame);
    }
  } else if (scanline == 261) {
    // prerender scanline
    if (cycles == 1) {
      status.set_sprite_overflow(false);
      status.set_sprite_zero_hit(false);
      status.set_vblank_status(false);
      nmiInterrupt = false;
    }
  }

  cycles++;
  // NTSC odd-frame cycle skip: when rendering is enabled, prerender scanline
  // drops one PPU cycle on odd frames (skip cycle 340).
  if (scanline == 261 && cycles == 340 && oddFrame &&
      (mask.show_background() || mask.show_sprites())) {
    cycles = 0;
    scanline = 0;
    oddFrame = !oddFrame;
    currentYQuadrant = ((scanline % 8) < 4) ? 0 : 1;
    return nullptr;
  }
  if (cycles == 341) {
    cycles = 0;
    scanline++;
    if (scanline == 262) {
      scanline = 0;
      oddFrame = !oddFrame;
    }
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
    data_buf = cart.read_chr_rom(addr_val);
    last_written_value = result;
    return result;
  } else if (addr_val <= 0x3EFF) {
    // RAM read
    addr_val &= 0x2FFF;  // mirror down address
    uint8_t result = data_buf;
    data_buf = vram[mirrorVRAMAddress(addr_val)];
    last_written_value = result;
    return result;
  } else if (addr_val >= 0x3F00 && addr_val <= 0x3FFF) {
    // palette table read
    uint8_t index = (addr_val - 0x3F00) & 0x1F;
    uint8_t result = palette_table[mirrorPaletteAddress(index)];
    last_written_value = result;
    return result;
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
    cart.write_chr_rom(addr_val, value);
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
  switch (cart.getMirroring()) {
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
  if (priorNMI && !ctrl.generate_vblank_nmi()) {
    nmiInterrupt = false;
  }
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
  uint8_t statusSnapshot = status.snapshot();

  // In this core's CPU<->PPU phase alignment, a read at (240,338) lands in the
  // "one tick before vblank" window and suppresses vblank for this frame.
  if (scanline == 240 && cycles == 338) {
    suppressVblankThisFrame = true;
  }

  // A read on the first tick of scanline 241 races with vblank set:
  // return bit 7 as set, clear it immediately, and suppress vblank/NMI.
  if (scanline == 241 && cycles == 0 && !suppressVblankThisFrame) {
    statusSnapshot |= 0x80;
    suppressVblankThisFrame = true;
  }

  // Approximate open-bus behavior for lower 5 bits with PPU I/O bus latch.
  uint8_t data = static_cast<uint8_t>((statusSnapshot & 0xE0) |
                                      (last_written_value & 0x1F));
  last_written_value = data;
  status.set_vblank_status(false);
  nmiInterrupt = false;
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
uint8_t PPU::read_oam_data() {
  last_written_value = oam_data[oam_addr];
  return oam_data[oam_addr];
}

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
