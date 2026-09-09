#include "../../include/PPU/PPU.h"

#include <utility>

namespace {
uint8_t decodePatternPixel(uint8_t low, uint8_t high, uint8_t bit) {
    return static_cast<uint8_t>(((low >> bit) & 1) |
                                (((high >> bit) & 1) << 1));
}
} // namespace

uint8_t PPU::mirrorPaletteAddress(uint8_t address) {
    address &= 0x1F;
    if ((address & 0x13) == 0x10) {
        address -= 0x10;
    }
    return address;
}

bool PPU::renderingEnabled() const {
    return mask.show_background() || mask.show_sprites();
}

void PPU::incrementDataAddress() {
    if (renderingEnabled() && (scanline < 240 || scanline == 261)) {
        addr.increment_x();
        addr.increment_y();
    } else {
        addr.increment(ctrl.vram_addr_increment());
    }
}

void PPU::fetchBackground() {
    // Two tiles are prefetched at dots 321-336 for the next scanline. Visible
    // pixels sample the high byte before each shift; fine X selects its bit.
    patternShiftLow <<= 1;
    patternShiftHigh <<= 1;
    attributeShiftLow <<= 1;
    attributeShiftHigh <<= 1;

    const uint16_t v = addr.get();
    switch (cycles & 7) {
    case 1:
        tileID = vram[mirrorVRAMAddress(0x2000 | (v & 0x0FFF))];
        break;
    case 3: {
        const uint16_t attributeAddress = static_cast<uint16_t>(
            0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 7));
        const uint8_t shift = static_cast<uint8_t>(((v >> 4) & 4) | (v & 2));
        attribute = (vram[mirrorVRAMAddress(attributeAddress)] >> shift) & 3;
        break;
    }
    case 5:
        patternLow = cart.read_chr_rom(static_cast<uint16_t>(
            ctrl.bg_pattern_addr() + tileID * 16 + ((v >> 12) & 7)));
        break;
    case 7:
        patternHigh = cart.read_chr_rom(static_cast<uint16_t>(
            ctrl.bg_pattern_addr() + tileID * 16 + ((v >> 12) & 7) + 8));
        break;
    case 0:
        patternShiftLow = (patternShiftLow & 0xFF00) | patternLow;
        patternShiftHigh = (patternShiftHigh & 0xFF00) | patternHigh;
        attributeShiftLow = (attributeShiftLow & 0xFF00) |
                            ((attribute & 1) ? 0xFF : 0);
        attributeShiftHigh = (attributeShiftHigh & 0xFF00) |
                             ((attribute & 2) ? 0xFF : 0);
        addr.increment_x();
        break;
    default:
        break;
    }
}

void PPU::evaluateSprites(int nextScanline) {
    spriteCount = 0;
    if (nextScanline <= 0 || nextScanline >= SCREEN_HEIGHT) {
        return;
    }

    const int height = ctrl.sprite_size();
    for (std::size_t sprite = 0; sprite < 64; ++sprite) {
        const std::size_t base = sprite * 4;
        int row = nextScanline - (static_cast<int>(oam_data[base]) + 1);
        if (row < 0 || row >= height) {
            continue;
        }
        if (spriteCount == sprites.size()) {
            // Model the eight-sprite limit, but not the hardware's diagonal
            // OAM scan that can produce false positive overflow flags.
            status.set_sprite_overflow(true);
            break;
        }

        const uint8_t tile = oam_data[base + 1];
        const uint8_t attributes = oam_data[base + 2];
        if (attributes & 0x80) {
            row = height - 1 - row;
        }
        uint16_t patternAddress;
        if (height == 16) {
            patternAddress = static_cast<uint16_t>(
                ((tile & 1) << 12) + ((tile & 0xFE) + row / 8) * 16 + row % 8);
        } else {
            patternAddress = static_cast<uint16_t>(
                ctrl.sprite_pattern_addr() + tile * 16 + row);
        }
        sprites[spriteCount++] = {
            oam_data[base + 3], attributes,
            cart.read_chr_rom(patternAddress),
            cart.read_chr_rom(patternAddress + 8), sprite == 0};
    }
}

void PPU::renderPixel() {
    const int screenX = cycles - 1;
    uint8_t background = 0;
    uint8_t palette = 0;
    if (mask.show_background() &&
        (screenX >= 8 || mask.leftmost_8pxl_background())) {
        const uint16_t select = static_cast<uint16_t>(0x8000 >> addr.fine_x());
        background = static_cast<uint8_t>(
            ((patternShiftLow & select) ? 1 : 0) |
            ((patternShiftHigh & select) ? 2 : 0));
        palette = static_cast<uint8_t>(
            ((attributeShiftLow & select) ? 1 : 0) |
            ((attributeShiftHigh & select) ? 2 : 0));
    }
    uint8_t paletteIndex = background ? palette * 4 + background : 0;

    if (mask.show_sprites() &&
        (screenX >= 8 || mask.leftmost_8pxl_sprite())) {
        for (uint8_t i = 0; i < spriteCount; ++i) {
            const auto &sprite = sprites[i];
            const int column = screenX - sprite.x;
            if (column < 0 || column >= 8) {
                continue;
            }
            const uint8_t bit = static_cast<uint8_t>(
                (sprite.attributes & 0x40) ? column : 7 - column);
            const uint8_t pixel = decodePatternPixel(
                sprite.patternLow, sprite.patternHigh, bit);
            if (pixel == 0) {
                continue;
            }
            if (sprite.spriteZero && background != 0 && screenX != 255) {
                status.set_sprite_zero_hit(true);
            }
            if (background == 0 || (sprite.attributes & 0x20) == 0) {
                paletteIndex = static_cast<uint8_t>(
                    0x10 + (sprite.attributes & 3) * 4 + pixel);
            }
            // First opaque OAM pixel wins even if it is behind background.
            break;
        }
    }

    // With rendering disabled, pointing v at palette RAM changes the backdrop.
    if (!renderingEnabled() && (addr.get() & 0x3F00) == 0x3F00) {
        paletteIndex = static_cast<uint8_t>(addr.get() & 0x1F);
    }
    uint8_t colour = palette_table[mirrorPaletteAddress(paletteIndex)];
    if (mask.is_grayscale()) {
        colour &= 0x30;
    }
    currentFrame->push(colour, background != 0);
}

std::optional<Frame> PPU::tick() {
    if (scanline == 0 && cycles == 0) {
        currentFrame.emplace();
    }

    const bool renderLine = scanline < 240 || scanline == 261;
    if (scanline < 240 && cycles >= 1 && cycles <= 256) {
        renderPixel();
    }
    if (renderLine && renderingEnabled()) {
        if ((cycles >= 1 && cycles <= 256) ||
            (cycles >= 321 && cycles <= 336)) {
            fetchBackground();
        }
        if (cycles == 256) {
            addr.increment_y();
        }
        if (cycles == 257) {
            addr.copy_x();
            // Pattern/OAM evaluation is batched here for the next scanline;
            // pixel compositing and sprite-zero detection still occur per dot.
            evaluateSprites(scanline == 261 ? 0 : scanline + 1);
        }
        if (scanline == 261 && cycles >= 280 && cycles <= 304) {
            addr.copy_y();
        }
    }

    if (scanline == 241 && cycles == 1) {
        if (!suppressVblankThisFrame) {
            status.set_vblank_status(true);
            if (ctrl.generate_vblank_nmi()) {
                nmiInterrupt = true;
            }
        }
        suppressVblankThisFrame = false;
        auto completedFrame = std::move(currentFrame);
        currentFrame.reset();
        ++cycles;
        return completedFrame;
    }
    if (scanline == 261 && cycles == 1) {
        status.set_sprite_overflow(false);
        status.set_sprite_zero_hit(false);
        status.set_vblank_status(false);
        nmiInterrupt = false;
    }

    if (scanline == 261 && cycles == 339 && oddFrame && renderingEnabled()) {
        scanline = 0;
        cycles = 0;
        oddFrame = false;
        return std::nullopt;
    }
    if (++cycles > 340) {
        cycles = 0;
        if (++scanline > 261) {
            scanline = 0;
            oddFrame = !oddFrame;
        }
    }
    return std::nullopt;
}

uint8_t PPU::cpuRead() {
    // v has 15 internal bits, but only 14 reach the external address bus.
    const uint16_t address = addr.get() & 0x3FFF;
    incrementDataAddress();
    uint8_t result = data_buf;
    if (address < 0x2000) {
        data_buf = cart.read_chr_rom(address);
    } else if (address < 0x3F00) {
        data_buf = vram[mirrorVRAMAddress(address)];
    } else {
        result = palette_table[mirrorPaletteAddress(address & 0x1F)];
        result &= mask.is_grayscale() ? 0x30 : 0x3F;
        result |= last_written_value & 0xC0;
        // Palette reads bypass the buffer, but fill it from mirrored CIRAM.
        data_buf = vram[mirrorVRAMAddress(address - 0x1000)];
    }
    last_written_value = result;
    return result;
}

void PPU::cpuWrite(uint8_t value) {
    last_written_value = value;
    const uint16_t address = addr.get() & 0x3FFF;
    incrementDataAddress();
    if (address < 0x2000) {
        cart.write_chr_ram(address, value);
    } else if (address < 0x3F00) {
        vram[mirrorVRAMAddress(address)] = value;
    } else {
        palette_table[mirrorPaletteAddress(address & 0x1F)] = value & 0x3F;
    }
}

uint16_t PPU::mirrorVRAMAddress(uint16_t address) {
    address &= 0x0FFF;
    const uint16_t offset = address & 0x03FF;
    switch (cart.getMirroring()) {
    case MirroringMode::Vertical:
        return static_cast<uint16_t>((address & 0x0400) | offset);
    case MirroringMode::Horizontal:
        return static_cast<uint16_t>(((address & 0x0800) >> 1) | offset);
    case MirroringMode::FourScreen:
        return address;
    }
    throw std::runtime_error("Invalid nametable mirroring mode");
}

void PPU::write_to_ctrl(uint8_t value) {
    last_written_value = value;
    const bool priorNMI = ctrl.generate_vblank_nmi();
    ctrl.update(value);
    addr.write_ctrl(value);
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
    // Preserve the CPU/PPU scheduler's existing vblank race approximation.
    if (scanline == 240 && cycles == 338) {
        suppressVblankThisFrame = true;
    }
    if (scanline == 241 && cycles == 0 && !suppressVblankThisFrame) {
        statusSnapshot |= 0x80;
        suppressVblankThisFrame = true;
    }
    const uint8_t data = static_cast<uint8_t>((statusSnapshot & 0xE0) |
                                             (last_written_value & 0x1F));
    last_written_value = data;
    status.set_vblank_status(false);
    nmiInterrupt = false;
    addr.reset_latch();
    return data;
}

void PPU::write_to_oam_addr(uint8_t value) {
    last_written_value = value;
    oam_addr = value;
}

void PPU::write_to_oam_data(uint8_t value) {
    last_written_value = value;
    oam_data[oam_addr++] = value;
}

uint8_t PPU::read_oam_data() {
    last_written_value = oam_data[oam_addr];
    return last_written_value;
}

void PPU::write_to_scroll(uint8_t value) {
    last_written_value = value;
    addr.write_scroll(value);
}

void PPU::write_to_ppu_addr(uint8_t value) {
    last_written_value = value;
    addr.update(value);
}

void PPU::write_oam_dma(const std::array<uint8_t, 256> &data) {
    for (const uint8_t value : data) {
        oam_data[oam_addr++] = value;
    }
}
