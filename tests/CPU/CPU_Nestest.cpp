#include <gtest/gtest.h>

#include "../NestestTrace.h"

namespace {

std::vector<std::string> readExpectedCpuTrace() {
    std::vector<std::string> lines = nestest::readLines("nestest_cpu_exp.log");
    for (std::string &line : lines) {
        line = nestest::swapPpuFields(line);
    }
    return lines;
}

} // namespace

TEST(CPUNestest, TraceMatchesExpectedLog) {
    const std::vector<std::string> expectedLines = readExpectedCpuTrace();
    ASSERT_FALSE(expectedLines.empty());

    Renderer renderer(nullptr, nullptr, nullptr);
    NES nes(std::move(renderer), nestest::readBinaryFile("nestest.nes"));
    nes.cpu.TEST_setPC(0xC000);

    // The reference log assumes the PPU advanced during the 7-cycle reset.
    for (int i = 0; i < 21; i++) {
        nes.ppu.tick();
    }

    nestest::LineCompareStreamBuf compareBuf(expectedLines);
    nestest::ScopedStreamCapture capture(&compareBuf);
    nestest::CpuPpuStepper stepper(nes);

    constexpr uint64_t kMaxCpuTicks = 100000;
    for (uint64_t cpuTicks = 0;
         cpuTicks < kMaxCpuTicks && !compareBuf.done() && !compareBuf.failed();
         cpuTicks++) {
        stepper.tick();
    }

    std::cout.flush();

    ASSERT_FALSE(compareBuf.failed()) << compareBuf.failure();
    ASSERT_TRUE(compareBuf.done())
        << "Trace ended early after " << compareBuf.linesCompared() << " lines";
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
