#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "../NestestTrace.h"

namespace {

std::filesystem::path localRom(const char *filename) {
    return std::filesystem::path(NES_SOURCE_DIR) / "romtests" / "games" /
           filename;
}

std::vector<uint8_t> readRom(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open ROM: " + path.string());
    }
    return {std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()};
}

struct RunStats {
    int frames = 0;
    int changedFrames = 0;
    int lateChangedFrames = 0;
    int spriteZeroFrames = 0;
};

RunStats runFrames(NES &nes, int frameLimit,
                   const std::function<uint8_t(int)> &buttons,
                   const std::function<void(int)> &observe = {}) {
    nestest::CpuPpuStepper stepper(nes);
    RunStats stats;
    uint64_t previousHash = 0;
    // NTSC frames take at most 29,781 CPU clocks. The extra margin also
    // covers the partial first frame; it does not depend on host wall time.
    const uint64_t tickLimit = static_cast<uint64_t>(frameLimit + 1) * 30000;
    nes.log.mute();
    nes.bus.setJoypad1Buttons(buttons(0));

    try {
        for (uint64_t tick = 0; tick < tickLimit && stats.frames < frameLimit;
             ++tick) {
            auto frame = stepper.tick();
            if (nes.cpu.isJammed()) {
                throw std::runtime_error("CPU executed a JAM opcode");
            }
            if (!frame) {
                continue;
            }
            if (frame->pixelData.size() != SCREEN_WIDTH * SCREEN_HEIGHT * 3 ||
                frame->backgroundOpaque.size() != SCREEN_WIDTH * SCREEN_HEIGHT ||
                frame->currentPixelIndex != SCREEN_WIDTH * SCREEN_HEIGHT) {
                throw std::runtime_error("PPU produced an incomplete frame");
            }
            uint64_t hash = 14695981039346656037ULL;
            for (uint8_t pixel : frame->pixelData) {
                hash = (hash ^ pixel) * 1099511628211ULL;
            }
            if (stats.frames != 0 && hash != previousHash) {
                ++stats.changedFrames;
                if (stats.frames >= frameLimit - 120) {
                    ++stats.lateChangedFrames;
                }
            }
            previousHash = hash;
            if ((nes.ppu.TEST_getstatus() & PPUStatus::SPRITE_ZERO_HIT) != 0) {
                ++stats.spriteZeroFrames;
            }
            if (observe) {
                observe(stats.frames);
            }
            ++stats.frames;
            nes.bus.setJoypad1Buttons(buttons(stats.frames));
        }
    } catch (const std::exception &error) {
        throw std::runtime_error("After " + std::to_string(stats.frames) +
                                 " frames, PC=" +
                                 std::to_string(nes.cpu.TEST_getPC()) + ": " +
                                 error.what());
    }
    return stats;
}

} // namespace

// Commercial ROMs are optional local inputs and are never downloaded by tests.
TEST(LocalROM, SuperMarioBrosScrollsAcrossNametablesAndKeepsRunning) {
    const auto path = localRom("smb.nes");
    if (!std::filesystem::exists(path)) {
        GTEST_SKIP() << "Place an NTSC Super Mario Bros. ROM at " << path;
    }

    NES nes(Renderer(nullptr, nullptr, nullptr), readRom(path));
    ASSERT_EQ(nes.cart.getRegion(), NESRegion::NTSC);

    // RAM symbols for the Japan/USA release, from doppelganger's disassembly:
    // https://6502disassembly.com/nes-smb/SuperMarioBros.html
    constexpr uint16_t kFrameCounter = 0x0009;
    constexpr uint16_t kScreenLeftPage = 0x071A;
    constexpr uint16_t kScreenLeftX = 0x071C;
    constexpr uint16_t kOperMode = 0x0770;
    constexpr int kFrameLimit = 1800; // 30 seconds of emulated gameplay.

    int maxCameraX = 0;
    int gameplayFrames = 0;
    int counterAdvances = 0;
    int lateCounterAdvances = 0;
    uint8_t previousCounter = 0;
    std::array<bool, 8> fineScrollSeen{};

    const auto buttons = [](int frame) -> uint8_t {
        if (frame >= 60 && frame < 65) {
            return Bus::JOYPAD_START;
        }
        if (frame < 65) {
            return 0;
        }
        // Run right and repeatedly hold/release jump to cross early obstacles.
        return static_cast<uint8_t>(Bus::JOYPAD_RIGHT | Bus::JOYPAD_B |
                                    (frame % 45 < 24 ? Bus::JOYPAD_A : 0));
    };
    const auto observe = [&](int frame) {
        const uint8_t counter = nes.bus.peek(kFrameCounter);
        if (counter != previousCounter) {
            ++counterAdvances;
            if (frame >= kFrameLimit - 120) {
                ++lateCounterAdvances;
            }
        }
        previousCounter = counter;
        if (nes.bus.peek(kOperMode) == 1) {
            ++gameplayFrames;
            const int cameraX = nes.bus.peek(kScreenLeftPage) * 256 +
                                nes.bus.peek(kScreenLeftX);
            maxCameraX = std::max(maxCameraX, cameraX);
            fineScrollSeen[cameraX & 7] = true;
        }
    };

    const auto stats = runFrames(nes, kFrameLimit, buttons, observe);
    std::cout << "SMB: frames=" << stats.frames << ", camera=" << maxCameraX
              << ", sprite-zero frames=" << stats.spriteZeroFrames
              << ", active game frames=" << gameplayFrames
              << ", frame-counter updates=" << counterAdvances << '\n';
    EXPECT_EQ(stats.frames, kFrameLimit);
    EXPECT_GE(gameplayFrames, 600);
    EXPECT_GE(maxCameraX, 256) << "Camera never crossed a nametable boundary";
    EXPECT_TRUE(std::all_of(fineScrollSeen.begin(), fineScrollSeen.end(),
                            [](bool seen) { return seen; }));
    EXPECT_GE(stats.spriteZeroFrames, 300);
    EXPECT_GE(stats.changedFrames, 300);
    EXPECT_GE(counterAdvances, 1500);
    EXPECT_GE(lateCounterAdvances, 100) << "Game stopped updating near run end";
}

TEST(LocalROM, PacmanContinuesProducingCompleteAnimatedFrames) {
    const auto path = localRom("pacman.nes");
    if (!std::filesystem::exists(path)) {
        GTEST_SKIP() << "Place a Pac-Man ROM at " << path;
    }

    NES nes(Renderer(nullptr, nullptr, nullptr), readRom(path));
    ASSERT_EQ(nes.cart.getRegion(), NESRegion::NTSC);
    const auto buttons = [](int frame) -> uint8_t {
        // The first press skips the title animation; the second starts play.
        if ((frame >= 120 && frame < 125) ||
            (frame >= 300 && frame < 305)) {
            return Bus::JOYPAD_START;
        }
        return frame >= 305 ? Bus::JOYPAD_RIGHT : 0;
    };
    const auto stats = runFrames(nes, 900, buttons);
    std::cout << "Pac-Man: frames=" << stats.frames
              << ", changed frames=" << stats.changedFrames
              << ", changed frames in final 120=" << stats.lateChangedFrames
              << '\n';
    EXPECT_EQ(stats.frames, 900);
    EXPECT_GE(stats.changedFrames, 300);
    // Require continuing maze animation after startup, not just the title's
    // entrance animation. Baseline and corrected cores both change 111 of
    // these 120 frames with this controller script.
    EXPECT_GE(stats.lateChangedFrames, 60);
}
