#include "../../include/PPU/PPU.h"
#include <string>

namespace {

// Sprite rendering happens after the background pass, so it writes directly
// into the frame buffer instead of advancing Frame::currentPixel.
inline void setFramePixel(Frame &frame, int x, int y, uint8_t colour) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }

    const std::size_t index = 3 * (static_cast<std::size_t>(y) * SCREEN_WIDTH +
                                   static_cast<std::size_t>(x));
    if (index + 2 >= frame.pixelData.size()) {
        return;
    }

    uint8_t r, g, b;
    std::tie(r, g, b) = NES_PALETTE[colour & 0x3F];
    frame.pixelData[index] = r;
    frame.pixelData[index + 1] = g;
    frame.pixelData[index + 2] = b;
}

inline bool frameBackgroundPixelOpaque(const Frame &frame, int x, int y) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return false;
    }

    const std::size_t index = (static_cast<std::size_t>(y) * SCREEN_WIDTH +
                               static_cast<std::size_t>(x));
    if (index >= frame.backgroundOpaque.size()) {
        return false;
    }
    return frame.backgroundOpaque[index] != 0;
}

inline uint8_t decodePatternPixel(uint8_t patternLow, uint8_t patternHigh,
                                  uint8_t bit) {
    const uint8_t lowBit = (patternLow >> bit) & 0x01;
    const uint8_t highBit = (patternHigh >> bit) & 0x01;
    return static_cast<uint8_t>((highBit << 1) | lowBit);
}

} // namespace

uint8_t PPU::mirrorPaletteAddress(uint8_t addr) {
    addr &= 0x1F;
    switch (addr) {
    case 0x10:
    case 0x14:
    case 0x18:
    case 0x1C:
        return static_cast<uint8_t>(addr - 0x10);
    default:
        return addr;
    }
}

void PPU::pushBackgroundPixelPair(Frame &frame, uint8_t attributeQuadrant,
                                  uint8_t leftBit,
                                  bool backgroundRenderingEnabled) {
    const uint8_t paletteSelection =
        static_cast<uint8_t>((attribute >> (attributeQuadrant * 2)) & 0b11);

    for (int bit = leftBit; bit >= static_cast<int>(leftBit) - 1; --bit) {
        const uint8_t pixelValue =
            backgroundRenderingEnabled
                ? decodePatternPixel(patternLow, patternHigh,
                                     static_cast<uint8_t>(bit))
                : 0;
        const uint8_t paletteIndex = mirrorPaletteAddress(static_cast<uint8_t>(
            pixelValue == 0 ? 0 : (paletteSelection * 4) + pixelValue));
        frame.push(palette_table[paletteIndex], pixelValue != 0);

        const std::size_t pixelIndex = frame.currentPixelIndex - 1;
        const int screenX = static_cast<int>(pixelIndex % SCREEN_WIDTH);
        const int screenY = static_cast<int>(pixelIndex / SCREEN_WIDTH);
        evaluateSpriteZeroHit(screenX, screenY, pixelValue != 0);
    }
}

bool PPU::spriteZeroPixelOpaque(int screenX, int screenY) const {
    const uint8_t spriteHeight = ctrl.sprite_size();
    const int spriteY = static_cast<int>(oam_data[0]) + 1;
    const int spriteX = static_cast<int>(oam_data[3]);

    if (screenX < spriteX || screenX >= spriteX + 8 || screenY < spriteY ||
        screenY >= spriteY + spriteHeight) {
        return false;
    }

    const uint8_t tileIndex = oam_data[1];
    const uint8_t attributes = oam_data[2];
    const bool flipHorizontal = (attributes & 0x40) != 0;
    const bool flipVertical = (attributes & 0x80) != 0;

    int spriteRow = screenY - spriteY;
    const int spriteColumn = screenX - spriteX;
    if (flipVertical) {
        spriteRow = spriteHeight - 1 - spriteRow;
    }

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
    const int bit = flipHorizontal ? spriteColumn : (7 - spriteColumn);

    return decodePatternPixel(patternLow, patternHigh,
                              static_cast<uint8_t>(bit)) != 0;
}

void PPU::evaluateSpriteZeroHit(int screenX, int screenY,
                                bool backgroundOpaque) {
    if ((status.snapshot() & PPUStatus::SPRITE_ZERO_HIT) != 0 ||
        !backgroundOpaque || !mask.show_background() || !mask.show_sprites()) {
        return;
    }

    if (screenX == 255) {
        return;
    }

    if (screenX < 8 && (!mask.leftmost_8pxl_background() ||
                        !mask.leftmost_8pxl_sprite())) {
        return;
    }

    if (spriteZeroPixelOpaque(screenX, screenY)) {
        status.set_sprite_zero_hit(true);
    }
}

void PPU::renderSprites(Frame &frame) {
    if (!mask.show_sprites()) {
        return;
    }

    const uint8_t spriteHeight = ctrl.sprite_size();

    // Render back-to-front so lower OAM indices keep priority.
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
                const uint8_t spritePixel = decodePatternPixel(
                    patternLow, patternHigh, static_cast<uint8_t>(bit));
                if (spritePixel == 0) {
                    continue;
                }

                if (behindBackground && mask.show_background() &&
                    frameBackgroundPixelOpaque(frame, screenX, screenY)) {
                    continue;
                }

                const uint8_t paletteIndex =
                    mirrorPaletteAddress(static_cast<uint8_t>(
                        0x10 + (paletteSelection * 4) + spritePixel));
                setFramePixel(frame, screenX, screenY,
                              palette_table[paletteIndex]);
            }
        }
    }
}

std::optional<Frame> PPU::tick() {
    if (scanline == 0 && cycles == 1) {
        currentFrame.emplace(); // initialise new frame
    }

    if (scanline < 240) {
        if (cycles < 256) {
            // visible pixels

            // apply scroll
            const int scrolledX = static_cast<int>(cycles) + scroll.scroll_x;
            const int scrolledY = scanline + scroll.scroll_y;

            const int ntXOffset = scrolledX / SCREEN_WIDTH;
            const int ntYOffset = scrolledY / SCREEN_HEIGHT;

            const uint8_t pixelX =
                static_cast<uint8_t>(scrolledX % SCREEN_WIDTH);
            const uint8_t pixelY =
                static_cast<uint8_t>(scrolledY % SCREEN_HEIGHT);

            const uint8_t coarseX = static_cast<uint8_t>(pixelX >> 3);
            const uint8_t coarseY = static_cast<uint8_t>(pixelY >> 3);
            const uint8_t fineY = static_cast<uint8_t>(pixelY & 0x07);

            // determine nametable of scrolled position
            const uint16_t baseNametable = ctrl.nametable_addr();
            const uint8_t baseNtX = static_cast<uint8_t>(
                (baseNametable == 0x2400 || baseNametable == 0x2C00) ? 1 : 0);
            const uint8_t baseNtY =
                static_cast<uint8_t>((baseNametable >= 0x2800) ? 1 : 0);

            // find tile pos inside nametable
            const uint8_t ntX =
                static_cast<uint8_t>((baseNtX + ntXOffset) & 0x01);
            const uint8_t ntY =
                static_cast<uint8_t>((baseNtY + ntYOffset) & 0x01);
            const uint16_t effectiveNametableBase =
                static_cast<uint16_t>(0x2000 + (((ntY << 1) | ntX) * 0x400));

            // find attribute-table quadrant for tile
            const uint16_t nametableAddress =
                mirrorVRAMAddress(static_cast<uint16_t>(
                    effectiveNametableBase + (coarseY * 32) + coarseX));
            const uint16_t attributeAddress = mirrorVRAMAddress(
                static_cast<uint16_t>(effectiveNametableBase + 0x03C0 +
                                      ((coarseY / 4) * 8) + (coarseX / 4)));
            const uint8_t attributeQuadrant = static_cast<uint8_t>(
                ((coarseY & 0x02) ? 2 : 0) | ((coarseX & 0x02) ? 1 : 0));

            // fetch tile data and render pixels
            // action depends on current phase
            //      phase 0-3 fetches tile data
            //      phase 4-7 renders pixels (2 per phase)
            const uint8_t phase = cycles % 8;
            switch (phase) {
            case 0: {
                tileID = vram[nametableAddress];
                break;
            }
            case 1: {
                attribute = vram[attributeAddress];
                break;
            }
            case 2: {
                const uint16_t address =
                    ctrl.bg_pattern_addr() + (tileID * 16) + fineY;
                patternLow = cart.read_chr_rom(address);
                break;
            }
            case 3: {
                const uint16_t address =
                    ctrl.bg_pattern_addr() + (tileID * 16) + fineY + 8;
                patternHigh = cart.read_chr_rom(address);
                break;
            }
            case 4:
            case 5:
            case 6:
            case 7: {
                const bool backgroundRenderingEnabled =
                    mask.show_background() &&
                    (mask.leftmost_8pxl_background() || cycles >= 8);
                const uint8_t leftBit = static_cast<uint8_t>(15 - (phase * 2));
                pushBackgroundPixelPair(*currentFrame, attributeQuadrant,
                                        leftBit, backgroundRenderingEnabled);
                break;
            }
            }
        }
        // overscan behaviour not modelled
    } else if (scanline == 241) {
        // scanline 240 is idle
        // trigger vblank at (241, 1).
        if (cycles == 1) {
            // start vblank
            renderSprites(*currentFrame);

            if (!suppressVblankThisFrame) {
                status.set_vblank_status(true);
                if (ctrl.generate_vblank_nmi()) {
                    nmiInterrupt = true;
                }
            }
            suppressVblankThisFrame = false;
            std::optional<Frame> completedFrame = std::move(currentFrame);
            currentFrame.reset();
            cycles++;
            return completedFrame;
        }
    } else if (scanline == 261) {
        if (cycles == 1) {
            status.set_sprite_overflow(false);
            status.set_sprite_zero_hit(false);
            status.set_vblank_status(false);
            nmiInterrupt = false;
        }
    }

    // this point is reached on all scanlines/cycles except for (241, 1), which
    // returns a completed frame (see above)

    if (scanline == 261 && cycles == 339 && oddFrame &&
        (mask.show_background() || mask.show_sprites())) {
        scanline = 0;
        cycles = 0;
        oddFrame = false;
        return std::nullopt;
    }

    cycles++;

    // check end of scanline reached
    if (cycles > 340) {
        // reset cycle counter and inc scanline
        cycles = 0;
        scanline++;
        // check end of frame reached
        if (scanline > 261) {
            scanline = 0;
            oddFrame = !oddFrame;
        }
    }

    // return null option until frame complete
    return std::nullopt;
}

uint8_t PPU::cpuRead() {
    uint16_t addr_val = addr.get();
    addr.increment(ctrl.vram_addr_increment());

    if (addr_val <= 0x1FFF) {
        uint8_t result = data_buf;
        data_buf = cart.read_chr_rom(addr_val);
        last_written_value = result;
        return result;
    } else if (addr_val <= 0x3EFF) {
        addr_val &= 0x2FFF;
        uint8_t result = data_buf;
        data_buf = vram[mirrorVRAMAddress(addr_val)];
        last_written_value = result;
        return result;
    } else if (addr_val >= 0x3F00 && addr_val <= 0x3FFF) {
        const uint8_t index = static_cast<uint8_t>((addr_val - 0x3F00) & 0x1F);
        uint8_t result = palette_table[mirrorPaletteAddress(index)];
        last_written_value = result;
        return result;
    } else {
        throw std::runtime_error(
            "PPU attempt to read from unsupported address: " +
            std::to_string(addr_val));
    }
}

void PPU::cpuWrite(uint8_t value) {
    last_written_value = value;

    uint16_t addr_val = addr.get();
    addr.increment(ctrl.vram_addr_increment());

    if (addr_val <= 0x1FFF) {
        cart.write_chr_ram(addr_val, value);
    } else if (addr_val <= 0x3EFF) {
        addr_val &= 0x2FFF;
        vram[mirrorVRAMAddress(addr_val)] = value;
    } else if (addr_val >= 0x3F00 && addr_val <= 0x3FFF) {
        const uint8_t index = static_cast<uint8_t>((addr_val - 0x3F00) & 0x1F);
        palette_table[mirrorPaletteAddress(index)] = value;
    } else {
        throw std::runtime_error(
            "PPU attempt to write to unsupported address: " +
            std::to_string(addr_val));
    }
}

uint16_t PPU::mirrorVRAMAddress(uint16_t addr) {
    addr &= 0x0FFF;

    const uint16_t tileOffset = static_cast<uint16_t>(addr & 0x03FF);
    switch (cart.getMirroring()) {
    case MirroringMode::Vertical:
        return static_cast<uint16_t>(((addr & 0x0400) != 0 ? 0x0400 : 0x0000) +
                                     tileOffset);
    case MirroringMode::Horizontal:
        return static_cast<uint16_t>(((addr & 0x0800) != 0 ? 0x0400 : 0x0000) +
                                     tileOffset);
    case MirroringMode::FourScreen:
        // The core only models 2 KiB of CIRAM, so four-screen mode wraps.
        return static_cast<uint16_t>(addr & 0x07FF);
    default:
        throw std::runtime_error(
            "PPU attempted to mirror VRAM address, but no mirroring mode is "
            "set.");
    }
}

// Writes to PPUCTRL can assert or clear NMI immediately when the write lands
// during vblank.
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

void PPU::write_to_mask(uint8_t value) {
    last_written_value = value;
    mask.update(value);
}

uint8_t PPU::read_status() {
    uint8_t statusSnapshot = status.snapshot();

    // read at (240,338) is one tick before vblank and suppresses vblank for
    // this frame
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

void PPU::write_to_oam_addr(uint8_t value) {
    last_written_value = value;
    oam_addr = value;
}

void PPU::write_to_oam_data(uint8_t value) {
    last_written_value = value;
    oam_data[oam_addr] = value;
    oam_addr = static_cast<uint8_t>(oam_addr + 1);
}

uint8_t PPU::read_oam_data() {
    last_written_value = oam_data[oam_addr];
    return oam_data[oam_addr];
}

void PPU::write_to_scroll(uint8_t value) {
    last_written_value = value;
    scroll.write(value);
}

void PPU::write_to_ppu_addr(uint8_t value) {
    last_written_value = value;
    addr.update(value);
}

void PPU::write_oam_dma(const std::array<uint8_t, 256> &data) {
    for (const auto &x : data) {
        oam_data[oam_addr] = x;
        oam_addr = static_cast<uint8_t>(oam_addr + 1);
    }
}
