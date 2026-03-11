#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "../../include/Cartridge.h"
#include "../../include/PPU/PPU.h"
#include "../../include/PPU/Registers/PPUMask.h"

namespace {
std::vector<uint8_t> makeMinimalNrom128() {
    // iNES header + 16 KiB PRG + 8 KiB CHR.
    std::vector<uint8_t> rom(16 + 0x4000 + 0x2000, 0);
    rom[0] = 'N';
    rom[1] = 'E';
    rom[2] = 'S';
    rom[3] = 0x1A;
    rom[4] = 1; // 1x 16 KiB PRG-ROM bank
    rom[5] = 1; // 1x 8 KiB CHR-ROM bank
    return rom;
}
} // namespace

TEST(PPUTiming, FrameWrapDoesNotDropOneTick) {
    Cartridge cart;
    cart.load(makeMinimalNrom128());
    PPU ppu(cart);

    constexpr int maxTicks = 300000;
    int ticks = 0;
    while ((ppu.getScanline() != 261 || ppu.getCycle() != 339) &&
           ticks < maxTicks) {
        ppu.tick();
        ticks++;
    }

    ASSERT_LT(ticks, maxTicks);
    ASSERT_EQ(ppu.getScanline(), 261);
    ASSERT_EQ(ppu.getCycle(), 339);

    for (int i = 0; i < 6; i++) {
        ppu.tick();
    }

    EXPECT_EQ(ppu.getScanline(), 0);
    EXPECT_EQ(ppu.getCycle(), 4);
}

TEST(PPUTiming, TickReturnsOptionalFrameOnlyWhenReady) {
    Cartridge cart;
    cart.load(makeMinimalNrom128());
    PPU ppu(cart);

    EXPECT_FALSE(ppu.tick().has_value());

    constexpr int maxTicks = 300000;
    for (int ticks = 1; ticks < maxTicks; ticks++) {
        auto frame = ppu.tick();
        if (frame.has_value()) {
            EXPECT_EQ(frame->pixelData.size(), SCREEN_WIDTH * SCREEN_HEIGHT * 3);
            EXPECT_EQ(frame->backgroundOpaque.size(),
                      SCREEN_WIDTH * SCREEN_HEIGHT);
            return;
        }
    }

    FAIL() << "PPU did not produce a frame within " << maxTicks << " ticks";
}

TEST(PPUTiming, OddFrameWithRenderingEnabledSkipsOneTick) {
    Cartridge cart;
    cart.load(makeMinimalNrom128());
    PPU ppu(cart);
    ppu.write_to_mask(PPUMask::SHOW_BACKGROUND);

    constexpr int maxTicks = 300000;
    int prerender339Sightings = 0;

    for (int ticks = 0; ticks < maxTicks; ticks++) {
        if (ppu.getScanline() == 261 && ppu.getCycle() == 339) {
            prerender339Sightings++;
            ppu.tick();

            if (prerender339Sightings == 2) {
                EXPECT_EQ(ppu.getScanline(), 0);
                EXPECT_EQ(ppu.getCycle(), 0);
                return;
            }

            continue;
        }

        ppu.tick();
    }

    FAIL() << "PPU did not reach the odd-frame prerender skip within "
           << maxTicks << " ticks";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
