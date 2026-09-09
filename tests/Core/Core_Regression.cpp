#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "../../include/Bus.h"
#include "../../include/CPU/CPU.h"
#include "../../include/Cartridge.h"
#include "../../include/Logger.h"

namespace {
std::vector<uint8_t> makeNrom(bool chrRam = false) {
    std::vector<uint8_t> rom(16 + 0x4000 + (chrRam ? 0 : 0x2000), 0);
    rom[0] = 'N';
    rom[1] = 'E';
    rom[2] = 'S';
    rom[3] = 0x1A;
    rom[4] = 1;
    rom[5] = chrRam ? 0 : 1;
    rom[16] = 0xEA; // NOP
    rom[16 + 0x3FFA] = 0x00;
    rom[16 + 0x3FFB] = 0x90; // NMI vector
    return rom;
}

class RecordingBus : public BusInterface {
  public:
    struct Access {
        uint16_t address;
        uint8_t value;
        bool write;
        int cycle;
    };
    std::array<uint8_t, 0x10000> memory{};
    std::vector<Access> accesses;
    int cycle = 0;

    bool tickDMA(bool) override {
        ++cycle;
        return false;
    }
    uint8_t read(uint16_t address) override {
        accesses.push_back({address, memory[address], false, cycle});
        return memory[address];
    }
    uint8_t peek(uint16_t address) override { return memory[address]; }
    void write(uint16_t address, uint8_t value) override {
        accesses.push_back({address, value, true, cycle});
        memory[address] = value;
    }
    uint16_t getPPUScanline() override { return 0; }
    uint16_t getPPUCycle() override { return 0; }
};

int executeInstruction(CPU &cpu) {
    int cycles = 0;
    do {
        cpu.tick();
        ++cycles;
    } while (cpu.TEST_getCyclesRemainingInCurrentInstr() != 0 && cycles < 20);
    return cycles;
}
} // namespace

TEST(CPUBusAccess, IndexedStoresWriteOnceOnTheirFinalCycle) {
    for (uint8_t opcode : std::array<uint8_t, 3>{0x9D, 0x99, 0x91}) {
        SCOPED_TRACE(static_cast<int>(opcode));
        RecordingBus bus;
        Logger logger;
        logger.mute();
        CPU cpu(bus, logger);
        cpu.TEST_setA(0x47);
        cpu.TEST_setX(6);
        cpu.TEST_setY(6);
        bus.memory[0x8000] = opcode;
        if (opcode == 0x91) {
            bus.memory[0x8001] = 0x10;
            bus.memory[0x10] = 0xFF;
            bus.memory[0x11] = 0x1F;
        } else {
            bus.memory[0x8001] = 0xFF;
            bus.memory[0x8002] = 0x1F;
        }

        const int expectedCycles = opcode == 0x91 ? 6 : 5;
        EXPECT_EQ(executeInstruction(cpu), expectedCycles);
        ASSERT_EQ(bus.accesses.size(), expectedCycles);
        EXPECT_EQ(bus.accesses[expectedCycles - 2].address, 0x1F05);
        EXPECT_FALSE(bus.accesses[expectedCycles - 2].write);
        const auto &store = bus.accesses.back();
        EXPECT_TRUE(store.write);
        EXPECT_EQ(store.address, 0x2005);
        EXPECT_EQ(store.value, 0x47);
        EXPECT_EQ(store.cycle, expectedCycles);
    }
}

TEST(CPUBusAccess, IndexedPPUAddressWritesPreserveTheTwoWriteLatch) {
    Cartridge cart(makeNrom());
    PPU ppu(cart);
    Bus bus(ppu, cart);
    Logger logger;
    logger.mute();
    CPU cpu(bus, logger);
    cpu.TEST_setPC(0);
    cpu.TEST_setX(6);
    for (uint16_t base : std::array<uint16_t, 2>{0, 3}) {
        bus.write(base, 0x9D); // STA $2000,X
        bus.write(base + 1, 0x00);
        bus.write(base + 2, 0x20);
    }
    cpu.TEST_setA(0x23);
    ASSERT_EQ(executeInstruction(cpu), 5);
    cpu.TEST_setA(0x45);
    ASSERT_EQ(executeInstruction(cpu), 5);
    EXPECT_EQ(ppu.TEST_getaddr(), 0x2345);
}

TEST(CPUBusAccess, ReadModifyWriteWritesOldThenNewValue) {
    const std::array<uint8_t, 6> opcodes{0x0E, 0x4E, 0x2E, 0x6E, 0xCE, 0xEE};
    const std::array<uint8_t, 6> results{0x82, 0x20, 0x82, 0x20, 0x40, 0x42};
    for (size_t i = 0; i < opcodes.size(); ++i) {
        SCOPED_TRACE(static_cast<int>(opcodes[i]));
        RecordingBus bus;
        Logger logger;
        logger.mute();
        CPU cpu(bus, logger);
        bus.memory[0x8000] = opcodes[i];
        bus.memory[0x8001] = 0x07;
        bus.memory[0x8002] = 0x20;
        bus.memory[0x2007] = 0x41;
        ASSERT_EQ(executeInstruction(cpu), 6);
        ASSERT_EQ(bus.accesses.size(), 6);
        EXPECT_TRUE(bus.accesses[4].write);
        EXPECT_EQ(bus.accesses[4].address, 0x2007);
        EXPECT_EQ(bus.accesses[4].value, 0x41);
        EXPECT_TRUE(bus.accesses[5].write);
        EXPECT_EQ(bus.accesses[5].address, 0x2007);
        EXPECT_EQ(bus.accesses[5].value, results[i]);
    }
}

TEST(CPUBusAccess, PageCrossingLoadReadsTheUncorrectedAddressFirst) {
    RecordingBus bus;
    Logger logger;
    logger.mute();
    CPU cpu(bus, logger);
    cpu.TEST_setX(3);
    bus.memory[0x8000] = 0xBD; // LDA $20FF,X
    bus.memory[0x8001] = 0xFF;
    bus.memory[0x8002] = 0x20;
    bus.memory[0x2102] = 0x53;
    ASSERT_EQ(executeInstruction(cpu), 5);
    ASSERT_EQ(bus.accesses.size(), 5);
    EXPECT_EQ(bus.accesses[3].address, 0x2002);
    EXPECT_EQ(bus.accesses[4].address, 0x2102);
    EXPECT_EQ(cpu.TEST_getA(), 0x53);
}

TEST(BusDMA, StallsFor513Or514CyclesWhileThePPUContinues) {
    for (int oddWriteCycle : {0, 1}) {
        SCOPED_TRACE(oddWriteCycle);
        Cartridge cart(makeNrom());
        PPU ppu(cart);
        Bus bus(ppu, cart);
        Logger logger;
        logger.mute();
        CPU cpu(bus, logger);
        for (uint16_t i = 0; i < 256; ++i) {
            bus.write(0x200 + i, static_cast<uint8_t>(i ^ 0xA5));
        }
        if (oddWriteCycle) bus.tickDMA(true);
        bus.write(0x2003, 0xF0); // DMA must wrap at the current OAM address.
        bus.write(0x4014, 2);
        EXPECT_EQ(bus.read(0x2004), 0xFF); // transfer has not started yet
        int stalledCycles = 0;
        while (bus.isDMAActive() && stalledCycles < 520) {
            cpu.tick();
            for (int ppuTick = 0; ppuTick < 3; ++ppuTick) ppu.tick();
            EXPECT_EQ(cpu.TEST_getPC(), 0x8000);
            ++stalledCycles;
        }
        ASSERT_EQ(stalledCycles, 513 + oddWriteCycle);
        EXPECT_EQ(ppu.getScanline() * 341 + ppu.getCycle(), stalledCycles * 3);
        for (uint16_t i = 0; i < 256; ++i) {
            bus.write(0x2003, static_cast<uint8_t>(0xF0 + i));
            EXPECT_EQ(bus.read(0x2004), static_cast<uint8_t>(i ^ 0xA5));
        }
        cpu.tick();
        EXPECT_EQ(cpu.TEST_getPC(), 0x8001);
    }
}

TEST(BusDMA, CopiesOnAlternatingReadAndWriteCycles) {
    Cartridge cart(makeNrom());
    PPU ppu(cart);
    Bus bus(ppu, cart);
    bus.write(0x0200, 0x12);
    bus.write(0x4014, 2);
    ASSERT_TRUE(bus.tickDMA(true)); // halt
    ASSERT_TRUE(bus.tickDMA(true)); // read the first source byte
    bus.write(0x0200, 0x34);
    EXPECT_EQ(bus.read(0x2004), 0xFF);
    ASSERT_TRUE(bus.tickDMA(true)); // write the buffered byte
    bus.write(0x2003, 0);
    EXPECT_EQ(bus.read(0x2004), 0x12);
}

TEST(BusDMA, ReadModifyWriteUsesTheSecondPageWithoutHaltingTheFinalWrite) {
    Cartridge cart(makeNrom());
    PPU ppu(cart);
    Bus bus(ppu, cart);
    Logger logger;
    logger.mute();
    CPU cpu(bus, logger);
    cpu.TEST_setPC(0x400);
    bus.write(0x400, 0xEE); // INC $4014: writes 0, then 1
    bus.write(0x401, 0x14);
    bus.write(0x402, 0x40);
    for (uint16_t i = 0; i < 256; ++i) bus.write(0x100 + i, 0x66);
    ASSERT_EQ(executeInstruction(cpu), 6);
    ASSERT_TRUE(bus.isDMAActive());
    int stalledCycles = 0;
    while (bus.isDMAActive() && stalledCycles < 520) {
        cpu.tick();
        ++stalledCycles;
    }
    ASSERT_EQ(stalledCycles, 513);
    for (uint16_t i = 0; i < 256; ++i) {
        bus.write(0x2003, static_cast<uint8_t>(i));
        EXPECT_EQ(bus.read(0x2004), 0x66);
    }
}

TEST(BusDMA, PendingNMIRunsAfterTheTransfer) {
    Cartridge cart(makeNrom());
    PPU ppu(cart);
    Bus bus(ppu, cart);
    Logger logger;
    logger.mute();
    CPU cpu(bus, logger);
    bus.write(0x4014, 2);
    cpu.triggerNMI();
    for (int cycle = 0; cycle < 513; ++cycle) cpu.tick();
    EXPECT_EQ(cpu.TEST_getPC(), 0x8000);
    EXPECT_EQ(cpu.checkInterrupt(), Interrupt::NONE);
    for (int cycle = 0; cycle < 7; ++cycle) cpu.tick();
    EXPECT_EQ(cpu.TEST_getPC(), 0x9000);
}

TEST(CartridgeRegression, ProtectsChrRomAndAllowsChrRam) {
    auto rom = makeNrom();
    rom[16 + 0x4000 + 0x123] = 0x5A;
    Cartridge cart(rom);
    cart.write_chr_ram(0x123, 0xA5);
    EXPECT_EQ(cart.read_chr_rom(0x123), 0x5A);
    cart.load(makeNrom(true));
    cart.write_chr_ram(0x123, 0xA5);
    EXPECT_EQ(cart.read_chr_rom(0x123), 0xA5);
}

TEST(CartridgeRegression, Mirrors16KiBPrgAndMaps32KiBPrg) {
    auto rom = makeNrom(true);
    rom[16] = 0x42;
    Cartridge cart(rom);
    EXPECT_EQ(cart.read_prg_rom(0x8000), 0x42);
    EXPECT_EQ(cart.read_prg_rom(0xC000), 0x42);
    EXPECT_THROW(cart.read_prg_rom(0x7FFF), std::out_of_range);
    rom.resize(16 + 0x8000);
    rom[4] = 2;
    rom[16 + 0x4000] = 0x76;
    cart.load(rom);
    EXPECT_EQ(cart.read_prg_rom(0x8000), 0x42);
    EXPECT_EQ(cart.read_prg_rom(0xC000), 0x76);
}

TEST(CartridgeRegression, RejectsInvalidSizesAndTruncatedPayloads) {
    Cartridge cart;
    auto rom = makeNrom();
    for (uint8_t banks : std::array<uint8_t, 2>{0, 3}) {
        rom[4] = banks;
        EXPECT_THROW(cart.load(rom), std::invalid_argument);
    }
    rom = makeNrom();
    rom[5] = 2;
    EXPECT_THROW(cart.load(rom), std::invalid_argument);
    rom = makeNrom();
    rom.pop_back();
    EXPECT_THROW(cart.load(rom), std::invalid_argument);
    rom = makeNrom();
    rom[6] = 0x10;
    EXPECT_THROW(cart.load(rom), std::invalid_argument);
    rom = makeNrom();
    rom[7] = 0x08;
    EXPECT_THROW(cart.load(rom), std::invalid_argument);
}

TEST(CartridgeRegression, FailedReplacementLeavesTheLoadedCartridgeIntact) {
    auto rom = makeNrom();
    rom[6] = 1; // vertical mirroring
    Cartridge cart(rom);
    cart.write_prg_ram(0x6000, 0x65);
    rom[6] = 0;
    rom[9] = 1;
    rom.pop_back();
    EXPECT_THROW(cart.load(rom), std::invalid_argument);
    EXPECT_EQ(cart.getMirroring(), MirroringMode::Vertical);
    EXPECT_EQ(cart.getRegion(), NESRegion::NTSC);
    EXPECT_EQ(cart.read_prg_rom(0x8000), 0xEA);
    EXPECT_EQ(cart.read_prg_ram(0x6000), 0x65);
}

TEST(CartridgeRegression, LoadsTrainerAt7000AndMapsPrgRam) {
    auto rom = makeNrom(true);
    rom[6] = 4;
    rom.insert(rom.begin() + 16, 512, 0x5A);
    Cartridge cart(rom);
    PPU ppu(cart);
    Bus bus(ppu, cart);
    EXPECT_EQ(bus.read(0x7000), 0x5A);
    EXPECT_EQ(bus.read(0x71FF), 0x5A);
    EXPECT_EQ(bus.read(0x8000), 0xEA);
    bus.write(0x6000, 0xA5);
    bus.write(0x7FFF, 0xC3);
    EXPECT_EQ(bus.peek(0x6000), 0xA5);
    EXPECT_EQ(bus.read(0x7FFF), 0xC3);
}

TEST(CartridgeRegression, RegionUsesOnlyTheTvSystemBit) {
    auto rom = makeNrom();
    rom[9] = 0x02;
    Cartridge cart(rom);
    EXPECT_EQ(cart.getRegion(), NESRegion::NTSC);
}

TEST(CPUJam, KilHaltsUntilResetEvenWhenInterruptsArePending) {
    RecordingBus bus;
    Logger logger;
    logger.mute();
    CPU cpu(bus, logger);
    bus.memory[0x8000] = 0x02; // KIL
    bus.memory[0xFFFC] = 0x00;
    bus.memory[0xFFFD] = 0x90;
    bus.memory[0x9000] = 0xEA;
    ASSERT_EQ(executeInstruction(cpu), 2);
    ASSERT_TRUE(cpu.isJammed());
    const auto haltedPC = cpu.TEST_getPC();
    cpu.triggerNMI();
    cpu.triggerIRQ();
    for (int cycle = 0; cycle < 20; ++cycle) cpu.tick();
    EXPECT_EQ(cpu.TEST_getPC(), haltedPC);
    EXPECT_TRUE(cpu.isJammed());
    cpu.triggerRES();
    for (int cycle = 0; cycle < 7; ++cycle) cpu.tick();
    EXPECT_FALSE(cpu.isJammed());
    EXPECT_EQ(cpu.TEST_getPC(), 0x9000);
    ASSERT_EQ(executeInstruction(cpu), 2);
    EXPECT_EQ(cpu.TEST_getPC(), 0x9001);
}

TEST(CPUBusAccess, SbxSetsCarryWhenSubtractionDoesNotBorrow) {
    for (uint8_t operand : std::array<uint8_t, 3>{0x0F, 0x10, 0x11}) {
        RecordingBus bus;
        Logger logger;
        logger.mute();
        CPU cpu(bus, logger);
        cpu.TEST_setA(0x30);
        cpu.TEST_setX(0x50); // A AND X is $10
        bus.memory[0x8000] = 0xCB; // SBX #operand
        bus.memory[0x8001] = operand;
        ASSERT_EQ(executeInstruction(cpu), 2);
        EXPECT_EQ(cpu.TEST_getX(), static_cast<uint8_t>(0x10 - operand));
        EXPECT_EQ((cpu.TEST_getStatus() & CPU::FLAG_CARRY) != 0, operand <= 0x10);
        EXPECT_EQ((cpu.TEST_getStatus() & CPU::FLAG_ZERO) != 0, operand == 0x10);
        EXPECT_EQ((cpu.TEST_getStatus() & CPU::FLAG_NEGATIVE) != 0, operand > 0x10);
        EXPECT_EQ(cpu.TEST_getA(), 0x30);
    }
}

TEST(FrameRegression, RejectsPixelsBeyondTheEndOfTheFrame) {
    Frame frame;
    for (int pixel = 0; pixel < SCREEN_WIDTH * SCREEN_HEIGHT; ++pixel) {
        frame.push(0x21, true);
    }
    const auto pixels = frame.pixelData;
    EXPECT_THROW(frame.push(0x30), std::out_of_range);
    EXPECT_EQ(frame.pixelData, pixels);
    EXPECT_EQ(frame.currentPixelIndex, SCREEN_WIDTH * SCREEN_HEIGHT);
    EXPECT_EQ(frame.backgroundOpaque.back(), 1);
}
