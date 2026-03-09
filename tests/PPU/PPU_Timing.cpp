#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "../../include/Cartridge.h"
#include "../../include/PPU/PPU.h"

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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
