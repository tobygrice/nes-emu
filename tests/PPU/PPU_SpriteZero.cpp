#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "../../include/Cartridge.h"
#include "../../include/PPU/PPU.h"
#include "../../include/PPU/Registers/PPUMask.h"
#include "../../include/PPU/Registers/PPUStatus.h"

namespace {
std::vector<uint8_t> makeMinimalChrRamNrom128() {
    // iNES header + 16 KiB PRG. CHR size 0 gives us 8 KiB of CHR-RAM.
    std::vector<uint8_t> rom(16 + 0x4000, 0);
    rom[0] = 'N';
    rom[1] = 'E';
    rom[2] = 'S';
    rom[3] = 0x1A;
    rom[4] = 1; // 1x 16 KiB PRG-ROM bank
    rom[5] = 0; // CHR-RAM
    return rom;
}

void writeSolidTile(Cartridge &cart, uint8_t tileIndex) {
    const uint16_t tileBase = static_cast<uint16_t>(tileIndex) * 16;
    for (uint16_t row = 0; row < 8; row++) {
        cart.write_chr_ram(tileBase + row, 0xFF);
        cart.write_chr_ram(tileBase + row + 8, 0x00);
    }
}

void setSpriteZero(PPU &ppu, uint8_t y, uint8_t tileIndex,
                   uint8_t attributes, uint8_t x) {
    ppu.write_to_oam_addr(0x00);
    ppu.write_to_oam_data(y);
    ppu.write_to_oam_data(tileIndex);
    ppu.write_to_oam_data(attributes);
    ppu.write_to_oam_data(x);
}

void enableRendering(PPU &ppu) {
    ppu.write_to_mask(PPUMask::SHOW_BACKGROUND | PPUMask::SHOW_SPRITES |
                      PPUMask::LEFTMOST_8PXL_BACKGROUND |
                      PPUMask::LEFTMOST_8PXL_SPRITE);
}

bool spriteZeroHitSet(const PPU &ppu) {
    return (ppu.TEST_getstatus() & PPUStatus::SPRITE_ZERO_HIT) != 0;
}

bool advanceUntilSpriteZeroHit(PPU &ppu, int maxTicks) {
    for (int tick = 0; tick < maxTicks; tick++) {
        ppu.tick();
        if (spriteZeroHitSet(ppu)) {
            return true;
        }
    }
    return false;
}
} // namespace

TEST(PPUSpriteZero, SetsSpriteZeroHitOnOpaqueOverlap) {
    Cartridge cart;
    cart.load(makeMinimalChrRamNrom128());
    PPU ppu(cart);

    writeSolidTile(cart, 1);
    writeSolidTile(cart, 2);
    ppu.TEST_setvram(0, 1);
    setSpriteZero(ppu, 0x00, 2, 0x00, 0x00);
    enableRendering(ppu);

    EXPECT_TRUE(advanceUntilSpriteZeroHit(ppu, 341 * 4));
}

TEST(PPUSpriteZero, DoesNotTriggerAtX255) {
    Cartridge cart;
    cart.load(makeMinimalChrRamNrom128());
    PPU ppu(cart);

    writeSolidTile(cart, 1);
    writeSolidTile(cart, 2);
    ppu.TEST_setvram(31, 1);
    setSpriteZero(ppu, 0x00, 2, 0x00, 0xFF);
    enableRendering(ppu);

    EXPECT_FALSE(advanceUntilSpriteZeroHit(ppu, 341 * 4));
}

TEST(PPUSpriteZero, PersistsUntilPrerenderLine) {
    Cartridge cart;
    cart.load(makeMinimalChrRamNrom128());
    PPU ppu(cart);

    writeSolidTile(cart, 1);
    writeSolidTile(cart, 2);
    ppu.TEST_setvram(0, 1);
    setSpriteZero(ppu, 0x00, 2, 0x00, 0x00);
    enableRendering(ppu);

    ASSERT_TRUE(advanceUntilSpriteZeroHit(ppu, 341 * 4));

    constexpr int toVblankMaxTicks = 341 * 260;
    int ticks = 0;
    while (ppu.getScanline() < 250 && ticks < toVblankMaxTicks) {
        ppu.tick();
        ticks++;
    }

    ASSERT_LT(ticks, toVblankMaxTicks);
    EXPECT_TRUE(spriteZeroHitSet(ppu));

    constexpr int toPrerenderMaxTicks = 341 * 20;
    ticks = 0;
    while ((ppu.getScanline() != 261 || ppu.getCycle() < 2) &&
           ticks < toPrerenderMaxTicks) {
        ppu.tick();
        ticks++;
    }

    ASSERT_LT(ticks, toPrerenderMaxTicks);
    EXPECT_FALSE(spriteZeroHitSet(ppu));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
