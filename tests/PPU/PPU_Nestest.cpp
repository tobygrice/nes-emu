#include <gtest/gtest.h>

#include "../NestestTrace.h"

TEST(PPUNestest, TraceMatchesExpectedLog) {
    const std::vector<std::string> expectedLines =
        nestest::readLines("nestest_ppu_exp.log");
    ASSERT_FALSE(expectedLines.empty());

    Renderer renderer(nullptr, nullptr, nullptr);
    NES nes(std::move(renderer), nestest::readBinaryFile("nestest.nes"));
    nes.cpu.TEST_setPC(0xC004);

    nestest::LineCompareStreamBuf compareBuf(expectedLines);
    nestest::ScopedStreamCapture capture(&compareBuf);
    nestest::CpuPpuStepper stepper(nes);

    constexpr uint64_t kMaxCpuTicks = 2000000;
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
