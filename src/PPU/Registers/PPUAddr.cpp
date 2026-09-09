#include "../../../include/PPU/Registers/PPUAddr.h"

// Address layout: yyy NN YYYYY XXXXX (fine Y, nametable, coarse Y, coarse X).
// https://www.nesdev.org/wiki/PPU_scrolling
void PPUAddr::write_ctrl(uint8_t data) {
    t = static_cast<uint16_t>((t & ~0x0C00) | ((data & 3) << 10));
}

void PPUAddr::write_scroll(uint8_t data) {
    if (!w) {
        t = static_cast<uint16_t>((t & ~0x001F) | (data >> 3));
        x = data & 7;
    } else {
        t = static_cast<uint16_t>((t & ~0x73E0) | ((data & 0xF8) << 2) |
                                  ((data & 7) << 12));
    }
    w = !w;
}

void PPUAddr::update(uint8_t data) {
    if (!w) {
        // The first write also clears bit 14, but does not change v yet.
        t = static_cast<uint16_t>((t & 0x00FF) | ((data & 0x3F) << 8));
    } else {
        t = static_cast<uint16_t>((t & 0x7F00) | data);
        v = t;
    }
    w = !w;
}

void PPUAddr::increment(uint8_t inc) {
    v = static_cast<uint16_t>((v + inc) & 0x7FFF);
}

void PPUAddr::increment_x() {
    if ((v & 0x001F) == 31) {
        v = static_cast<uint16_t>((v & ~0x001F) ^ 0x0400);
    } else {
        ++v;
    }
}

void PPUAddr::increment_y() {
    if ((v & 0x7000) != 0x7000) {
        v += 0x1000;
        return;
    }

    v &= ~0x7000;
    uint16_t coarseY = (v & 0x03E0) >> 5;
    if (coarseY == 29) {
        coarseY = 0;
        v ^= 0x0800;
    } else if (coarseY == 31) {
        // Rows 30 and 31 address the attribute table; row 31 wraps without
        // crossing to the next nametable.
        coarseY = 0;
    } else {
        ++coarseY;
    }
    v = static_cast<uint16_t>((v & ~0x03E0) | (coarseY << 5));
}

void PPUAddr::copy_x() {
    v = static_cast<uint16_t>((v & ~0x041F) | (t & 0x041F));
}

void PPUAddr::copy_y() {
    v = static_cast<uint16_t>((v & ~0x7BE0) | (t & 0x7BE0));
}
