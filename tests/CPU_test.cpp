// test_cpu.cpp
#include <gtest/gtest.h>
#include "../include/CPU.h"
#include "../include/OpCode.h"
#include <stdexcept>
#include <vector>
#include <string>

// Define a test fixture for CPU tests.
class CPUTest : public ::testing::Test {
protected:
    CPU cpu;
};

// Test memory read/write for single bytes.
TEST_F(CPUTest, MemoryReadWrite8) {
    cpu.memWrite8(0x1234, 0xAB);
    EXPECT_EQ(cpu.memRead8(0x1234), 0xAB);
}

// Test memory read/write for 16-bit values (little-endian).
TEST_F(CPUTest, MemoryReadWrite16) {
    cpu.memWrite16(0x2000, 0xBEEF);
    EXPECT_EQ(cpu.memRead16(0x2000), 0xBEEF);
}

// Test updateZeroAndNegativeFlags with a zero result.
TEST_F(CPUTest, UpdateFlagsZero) {
    cpu.status = 0;
    cpu.updateZeroAndNegativeFlags(0);
    EXPECT_TRUE(cpu.status & 0b00000010) << "Zero flag should be set for result 0.";
    EXPECT_FALSE(cpu.status & 0b10000000) << "Negative flag should not be set for 0.";
}

// Test updateZeroAndNegativeFlags with a result that has the high bit set.
TEST_F(CPUTest, UpdateFlagsNegative) {
    cpu.status = 0;
    cpu.updateZeroAndNegativeFlags(0x80);
    EXPECT_FALSE(cpu.status & 0b00000010) << "Zero flag should not be set for nonzero result.";
    EXPECT_TRUE(cpu.status & 0b10000000) << "Negative flag should be set when bit 7 is 1.";
}

// Test updateZeroAndNegativeFlags with a nonzero, non-negative value.
TEST_F(CPUTest, UpdateFlagsNonZeroNonNegative) {
    cpu.status = 0xFF; // Start with all flags set.
    cpu.updateZeroAndNegativeFlags(0x05);
    EXPECT_FALSE(cpu.status & 0b00000010) << "Zero flag should be cleared for nonzero result.";
    EXPECT_FALSE(cpu.status & 0b10000000) << "Negative flag should be cleared if bit 7 is 0.";
}

// Test that loadProgram correctly copies the program into memory
// and sets the reset vector to the start of cartridge ROM (0x8000).
TEST_F(CPUTest, LoadProgram) {
    std::vector<uint8_t> program = {0xA9, 0x05, 0x00}; // LDA immediate 0x05, then BRK
    cpu.loadProgram(program);
    EXPECT_EQ(cpu.memRead8(0x8000), 0xA9);
    EXPECT_EQ(cpu.memRead8(0x8001), 0x05);
    EXPECT_EQ(cpu.memRead8(0x8002), 0x00);
    // Check the reset vector at 0xFFFC.
    EXPECT_EQ(cpu.memRead16(0xFFFC), 0x8000);
}

// Test resetInterrupt to ensure registers are cleared and the PC is set to the reset vector.
TEST_F(CPUTest, ResetInterrupt) {
    cpu.a_register = 0xFF;
    cpu.x_register = 0xFF;
    cpu.y_register = 0xFF;
    cpu.status = 0xFF;
    cpu.pc = 0x1234;
    // Set the reset vector to 0x8000.
    cpu.memWrite16(0xFFFC, 0x8000);
    cpu.resetInterrupt();
    EXPECT_EQ(cpu.pc, 0x8000);
    EXPECT_EQ(cpu.a_register, 0);
    EXPECT_EQ(cpu.x_register, 0);
    EXPECT_EQ(cpu.y_register, 0);
    EXPECT_EQ(cpu.status, 0);
}

// Test LDA (Load Accumulator) with Immediate addressing.
TEST_F(CPUTest, LDAImmediate) {
    cpu.pc = 0x9000; // Set PC to point to the operand.
    cpu.memWrite8(cpu.pc, 0x42);
    cpu.lda(AddressingMode::Immediate);
    EXPECT_EQ(cpu.a_register, 0x42);
    EXPECT_FALSE(cpu.status & 0b00000010) << "Zero flag should not be set for 0x42.";
    EXPECT_FALSE(cpu.status & 0b10000000) << "Negative flag should not be set for 0x42.";
}

// Test LDA Immediate with a zero operand (should set the zero flag).
TEST_F(CPUTest, LDAImmediateZero) {
    cpu.pc = 0x9000;
    cpu.memWrite8(cpu.pc, 0x00);
    cpu.lda(AddressingMode::Immediate);
    EXPECT_EQ(cpu.a_register, 0x00);
    EXPECT_TRUE(cpu.status & 0b00000010) << "Zero flag should be set when operand is 0.";
}

// Test LDA Immediate with a negative operand (should set the negative flag).
TEST_F(CPUTest, LDAImmediateNegative) {
    cpu.pc = 0x9000;
    cpu.memWrite8(cpu.pc, 0x80);
    cpu.lda(AddressingMode::Immediate);
    EXPECT_EQ(cpu.a_register, 0x80);
    EXPECT_TRUE(cpu.status & 0b10000000) << "Negative flag should be set when operand is 0x80.";
}

// Test the executeProgram loop by loading a simple program
// that executes LDA immediate (0xA9) followed by an illegal opcode (0xFF)
// which will cause a runtime_error to be thrown.
TEST_F(CPUTest, ExecuteProgram_LDAAndUnknownOpcode) {
    // Program: LDA immediate 0x99, then an unknown opcode 0xFF.
    std::vector<uint8_t> program = {0xA9, 0x99, 0xFF};
    cpu.loadProgram(program);
    cpu.resetInterrupt();
    try {
        cpu.executeProgram();
        FAIL() << "Expected std::runtime_error due to unknown opcode.";
    } catch (const std::runtime_error& e) {
        std::string errMsg = e.what();
        EXPECT_NE(errMsg.find("Unknown opcode"), std::string::npos)
            << "Error message should mention unknown opcode.";
        // Verify that the LDA instruction was executed correctly.
        EXPECT_EQ(cpu.a_register, 0x99);
    } catch (...) {
        FAIL() << "Expected std::runtime_error due to unknown opcode.";
    }
}

// Test the BRK handler by calling the BRK opcode directly.
TEST_F(CPUTest, BRKHandler) {
    // Set initial conditions.
    cpu.pc = 0x8002;
    cpu.sp = 0xFF;
    cpu.status = 0x00;  // Clear status flags.
    // Set the BRK interrupt vector to 0x9000.
    cpu.memWrite16(0xFFFE, 0x9000);
    
    cpu.brk();

    // After BRK, the PC should jump to the interrupt vector.
    EXPECT_EQ(cpu.pc, 0x9000);
    // The Interrupt Disable flag (bit 2) should now be set.
    EXPECT_TRUE(cpu.status & 0b00000100);
    // BRK pushes three bytes onto the stack, so SP should be decreased by 3.
    EXPECT_EQ(cpu.sp, 0xFC);

    // Verify the values pushed onto the stack.
    // The push order is: high byte of PC, low byte of PC, then status | 0b00010000.
    // Initially, PC was 0x8002.
    uint8_t pushedHigh   = cpu.memRead8(0x0100 + 0xFF); // First pushed at 0x01FF.
    uint8_t pushedLow    = cpu.memRead8(0x0100 + 0xFE); // Second pushed at 0x01FE.
    uint8_t pushedStatus = cpu.memRead8(0x0100 + 0xFD); // Third pushed at 0x01FD.
    
    EXPECT_EQ(pushedHigh, 0x80) << "High byte of PC should be 0x80.";
    EXPECT_EQ(pushedLow, 0x02) << "Low byte of PC should be 0x02.";
    EXPECT_EQ(pushedStatus, 0x00 | 0b00010000)
        << "Status pushed onto stack should have the break flag (bit 4) set.";
}

// Main entry point for the tests.
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}