#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "../../include/PPU/PPU.h"

namespace {
std::vector<uint8_t> makeChrRamRom() {
    std::vector<uint8_t> rom(16 + 0x4000, 0);
    rom[0] = 'N';
    rom[1] = 'E';
    rom[2] = 'S';
    rom[3] = 0x1A;
    rom[4] = 1;
    return rom;
}

void setAddress(PPU &ppu, uint16_t address) {
    ppu.read_status();
    ppu.write_to_ppu_addr(static_cast<uint8_t>(address >> 8));
    ppu.write_to_ppu_addr(static_cast<uint8_t>(address));
}

void setScroll(PPU &ppu, uint8_t x, uint8_t y, uint8_t control = 0) {
    ppu.read_status();
    ppu.write_to_ctrl(control);
    ppu.write_to_scroll(x);
    ppu.write_to_scroll(y);
}

void advanceTo(PPU &ppu, int scanline, int dot) {
    for (int i = 0; i < 341 * 262 * 2; ++i) {
        if (ppu.getScanline() == scanline && ppu.getCycle() == dot) {
            return;
        }
        ppu.tick();
    }
    throw std::runtime_error("PPU did not reach requested dot");
}

Frame finishFrame(PPU &ppu) {
    for (int i = 0; i < 341 * 262; ++i) {
        if (auto frame = ppu.tick()) {
            return std::move(*frame);
        }
    }
    throw std::runtime_error("PPU did not produce a frame");
}

Colour pixelAt(const Frame &frame, int x, int y) {
    const std::size_t i = (y * SCREEN_WIDTH + x) * 3;
    return {frame.pixelData[i], frame.pixelData[i + 1], frame.pixelData[i + 2]};
}

void solidTile(Cartridge &cart, uint8_t tile, uint8_t pixel) {
    for (int row = 0; row < 8; ++row) {
        cart.write_chr_ram(static_cast<uint16_t>(tile * 16 + row),
                           (pixel & 1) ? 0xFF : 0);
        cart.write_chr_ram(static_cast<uint16_t>(tile * 16 + row + 8),
                           (pixel & 2) ? 0xFF : 0);
    }
}

void writePalette(PPU &ppu) {
    setAddress(ppu, 0x3F00);
    for (uint8_t colour = 0; colour < 16; ++colour) {
        ppu.cpuWrite(colour);
    }
    setAddress(ppu, 0x3F11);
    ppu.cpuWrite(0x21);
    ppu.cpuWrite(0x22);
}

void setSprite(PPU &ppu, uint8_t index, uint8_t y, uint8_t tile,
               uint8_t attributes, uint8_t x) {
    ppu.write_to_oam_addr(static_cast<uint8_t>(index * 4));
    ppu.write_to_oam_data(y);
    ppu.write_to_oam_data(tile);
    ppu.write_to_oam_data(attributes);
    ppu.write_to_oam_data(x);
}

constexpr uint8_t allRendering = PPUMask::SHOW_BACKGROUND | PPUMask::SHOW_SPRITES |
    PPUMask::LEFTMOST_8PXL_BACKGROUND | PPUMask::LEFTMOST_8PXL_SPRITE;
} // namespace

TEST(PPUScrolling, AddressAndScrollWritesShareOneLatch) {
    Cartridge cart(makeChrRamRom());
    PPU ppu(cart);
    setAddress(ppu, 0x2142);
    ppu.write_to_ppu_addr(0x2C);
    EXPECT_EQ(ppu.TEST_getaddr(), 0x2142); // First write changes t only.
    ppu.write_to_ppu_addr(0x19);
    EXPECT_EQ(ppu.TEST_getaddr(), 0x2C19);

    ppu.read_status();
    ppu.write_to_scroll(0x28); // First write to $2005.
    ppu.write_to_ppu_addr(0x83); // Second write to $2006 commits t.
    EXPECT_EQ(ppu.TEST_getaddr(), 0x2C83);

    ppu.write_to_ppu_addr(0x21); // First write to $2006.
    ppu.write_to_scroll(0x3B); // Second write to $2005.
    EXPECT_EQ(ppu.TEST_getaddr(), 0x2C83);
    ppu.write_to_ppu_addr(0x22);
    ppu.write_to_ppu_addr(0x44);
    EXPECT_EQ(ppu.TEST_getaddr(), 0x2244);

    ppu.write_to_scroll(7);
    ppu.read_status(); // Reset the common latch.
    ppu.write_to_ppu_addr(0x25);
    ppu.write_to_ppu_addr(0x67);
    EXPECT_EQ(ppu.TEST_getaddr(), 0x2567);
}

TEST(PPUScrolling, CoarseXAndYWrapAccordingToNametableGeometry) {
    PPUAddr address;
    address.update(0x20);
    address.update(0x1F);
    address.increment_x();
    EXPECT_EQ(address.get(), 0x2400);

    address.write_ctrl(0);
    address.write_scroll(0);
    address.write_scroll(239); // Fine Y 7, coarse Y 29.
    address.copy_x();
    address.copy_y();
    address.increment_y();
    EXPECT_EQ(address.get(), 0x0800); // Toggle vertical nametable at row 29.

    address.write_scroll(0);
    address.write_scroll(255); // Fine Y 7, coarse Y 31.
    address.copy_y();
    address.increment_y();
    EXPECT_EQ(address.get(), 0); // Row 31 wraps without toggling the nametable.
}

TEST(PPUScrolling, HorizontalScrollCopiesAtDot257AndVerticalDuringPrerender) {
    Cartridge cart(makeChrRamRom());
    PPU ppu(cart);
    setAddress(ppu, 0x2000);
    setScroll(ppu, 24, 239, 1);
    ppu.write_to_mask(PPUMask::SHOW_BACKGROUND);
    advanceTo(ppu, 0, 257);
    const uint16_t verticalBefore = ppu.TEST_getaddr() & 0x7BE0;
    EXPECT_EQ(verticalBefore, 0x3000); // Initial v fine Y 2 increments to 3.
    ppu.tick();
    EXPECT_EQ(ppu.TEST_getaddr() & 0x041F, 0x0403);
    EXPECT_EQ(ppu.TEST_getaddr() & 0x7BE0, verticalBefore);

    advanceTo(ppu, 261, 280);
    ppu.tick();
    EXPECT_EQ(ppu.TEST_getaddr() & 0x7BE0, 0x73A0);
}

TEST(PPUScrolling, EveryFineXOffsetMatchesPixelsAcrossTilesAndNametables) {
    constexpr std::array<uint8_t, 15> scrollOffsets{
        0, 1, 2, 3, 4, 5, 6, 7, 249, 250, 251, 252, 253, 254, 255};
    for (const uint8_t scrollX : scrollOffsets) {
        SCOPED_TRACE(static_cast<int>(scrollX));
        Cartridge cart(makeChrRamRom());
        cart.setMirroring(MirroringMode::FourScreen);
        PPU ppu(cart);
        writePalette(ppu);
        for (int tile = 1; tile <= 3; ++tile) {
            for (int row = 0; row < 8; ++row) {
                uint8_t low = 0;
                uint8_t high = 0;
                for (int column = 0; column < 8; ++column) {
                    const int pixel = (tile + row + column) % 4;
                    low |= (pixel & 1) << (7 - column);
                    high |= ((pixel >> 1) & 1) << (7 - column);
                }
                cart.write_chr_ram(static_cast<uint16_t>(tile * 16 + row), low);
                cart.write_chr_ram(static_cast<uint16_t>(tile * 16 + row + 8),
                                   high);
            }
        }
        for (int nt = 0; nt < 4; ++nt) {
            for (int y = 0; y < 30; ++y) {
                for (int x = 0; x < 32; ++x) {
                    ppu.TEST_setvram(
                        static_cast<uint16_t>(nt * 0x400 + y * 32 + x),
                        static_cast<uint8_t>(1 + (x + 2 * y + nt) % 3));
                }
            }
            for (int i = 0; i < 64; ++i) {
                ppu.TEST_setvram(
                    static_cast<uint16_t>(nt * 0x400 + 0x3C0 + i), 0xE4);
            }
        }
        constexpr int scrollY = 237;
        setScroll(ppu, scrollX, scrollY);
        ppu.write_to_mask(PPUMask::SHOW_BACKGROUND |
                          PPUMask::LEFTMOST_8PXL_BACKGROUND);
        finishFrame(ppu); // Power-on starts without a pre-render fetch.
        const auto frame = finishFrame(ppu);
        ASSERT_EQ(frame.currentPixelIndex, SCREEN_WIDTH * SCREEN_HEIGHT);
        for (int y : {0, 1, 2, 3, 7, 8, 15, 16, 31, 239}) {
            for (int x = 0; x < SCREEN_WIDTH; ++x) {
                const int worldX = x + scrollX;
                const int worldY = y + scrollY;
                const int nt = (worldX / 256) + 2 * (worldY / 240);
                const int tileX = (worldX % 256) / 8;
                const int tileY = (worldY % 240) / 8;
                const int tile = 1 + (tileX + 2 * tileY + nt) % 3;
                const int pixel = (tile + worldY % 8 + worldX % 8) % 4;
                const int palette = ((tileY & 2) ? 2 : 0) |
                                    ((tileX & 2) ? 1 : 0);
                const int colour = pixel ? palette * 4 + pixel : 0;
                ASSERT_EQ(pixelAt(frame, x, y), NES_PALETTE[colour])
                    << "at (" << x << ", " << y << ')';
            }
        }
    }
}

TEST(PPUScrolling, MidframeScrollChangesFollowingScanlineWithoutMovingStatusBar) {
    Cartridge cart(makeChrRamRom());
    cart.setMirroring(MirroringMode::Vertical);
    PPU ppu(cart);
    writePalette(ppu);
    solidTile(cart, 1, 1);
    solidTile(cart, 2, 2);
    for (uint16_t i = 0; i < 960; ++i) {
        ppu.TEST_setvram(i, 1);
        ppu.TEST_setvram(static_cast<uint16_t>(0x400 + i), 2);
    }
    setScroll(ppu, 0, 0);
    ppu.write_to_mask(PPUMask::SHOW_BACKGROUND |
                      PPUMask::LEFTMOST_8PXL_BACKGROUND);
    finishFrame(ppu);
    advanceTo(ppu, 31, 200);
    setScroll(ppu, 0, 0, 1);
    const auto frame = finishFrame(ppu);
    EXPECT_EQ(pixelAt(frame, 220, 31), NES_PALETTE[1]);
    EXPECT_EQ(pixelAt(frame, 0, 32), NES_PALETTE[2]);
    EXPECT_EQ(pixelAt(frame, 255, 32), NES_PALETTE[2]);
}

TEST(PPUScrolling, SpriteZeroHitOccursOnTheOverlappingPixelDot) {
    Cartridge cart(makeChrRamRom());
    PPU ppu(cart);
    solidTile(cart, 1, 1);
    solidTile(cart, 2, 1);
    ppu.TEST_setvram(1, 1);
    setSprite(ppu, 0, 0, 2, 0, 11);
    setScroll(ppu, 3, 0);
    ppu.write_to_mask(allRendering);
    advanceTo(ppu, 1, 12); // Dot 12 draws x=11.
    EXPECT_EQ(ppu.TEST_getstatus() & PPUStatus::SPRITE_ZERO_HIT, 0);
    ppu.tick();
    EXPECT_NE(ppu.TEST_getstatus() & PPUStatus::SPRITE_ZERO_HIT, 0);
}

TEST(PPUScrolling, BackgroundPriorityOfFirstSpriteOccludesLaterSprites) {
    Cartridge cart(makeChrRamRom());
    PPU ppu(cart);
    writePalette(ppu);
    solidTile(cart, 1, 1);
    solidTile(cart, 2, 2);
    ppu.TEST_setvram(1, 1);
    setSprite(ppu, 0, 0, 1, 0x20, 8);
    setSprite(ppu, 1, 0, 2, 0, 8);
    setScroll(ppu, 0, 0);
    ppu.write_to_mask(allRendering);
    finishFrame(ppu);
    const auto frame = finishFrame(ppu);
    EXPECT_EQ(pixelAt(frame, 8, 1), NES_PALETTE[1]);
}

TEST(PPUScrolling, SpritePixelsKeepMaskStateFromTheirRenderingDot) {
    Cartridge cart(makeChrRamRom());
    PPU ppu(cart);
    writePalette(ppu);
    solidTile(cart, 1, 1);
    setSprite(ppu, 0, 0, 1, 0, 16);
    setScroll(ppu, 0, 0);
    ppu.write_to_mask(allRendering);
    advanceTo(ppu, 10, 0);
    ppu.write_to_mask(0);
    const auto frame = finishFrame(ppu);
    EXPECT_EQ(pixelAt(frame, 16, 1), NES_PALETTE[0x21]);
    EXPECT_EQ(frame.currentPixelIndex, SCREEN_WIDTH * SCREEN_HEIGHT);
}

TEST(PPUScrolling, OnlyFirstEightSpritesAreSelectedForEachScanline) {
    Cartridge cart(makeChrRamRom());
    PPU ppu(cart);
    writePalette(ppu);
    solidTile(cart, 1, 1);
    for (uint8_t i = 0; i < 9; ++i) {
        setSprite(ppu, i, 0, 1, 0, static_cast<uint8_t>(16 + i * 8));
    }
    setScroll(ppu, 0, 0);
    ppu.write_to_mask(allRendering);
    const auto frame = finishFrame(ppu);
    EXPECT_EQ(pixelAt(frame, 72, 1), NES_PALETTE[0x21]);
    EXPECT_EQ(pixelAt(frame, 80, 1), NES_PALETTE[0]);
    EXPECT_NE(ppu.TEST_getstatus() & PPUStatus::SPRITE_OVERFLOW, 0);
}

TEST(PPUScrolling, PaletteReadRefreshesBufferFromUnderlyingNametable) {
    Cartridge cart(makeChrRamRom());
    PPU ppu(cart);
    setAddress(ppu, 0x2F00);
    ppu.cpuWrite(0xA5);
    setAddress(ppu, 0x3F00);
    ppu.cpuWrite(0xFF);
    setAddress(ppu, 0x3F00);
    EXPECT_EQ(ppu.cpuRead(), 0x3F);
    setAddress(ppu, 0x2000);
    EXPECT_EQ(ppu.cpuRead(), 0xA5);
}

TEST(PPUScrolling, DataPortMasksInternalFineYBitBeforeAddressingMemory) {
    Cartridge cart(makeChrRamRom());
    PPU ppu(cart);
    setAddress(ppu, 0x3FFF);
    ppu.cpuWrite(0x12);
    EXPECT_EQ(ppu.TEST_getaddr(), 0x4000);
    ppu.cpuWrite(0xA5); // External address is $0000, not an unsupported $4000.
    EXPECT_EQ(cart.read_chr_rom(0), 0xA5);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
