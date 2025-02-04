#include "../include/CPU.h"
#include <gtest/gtest.h>
#include <vector>

/**
 * TO-DO : test negative flag set once implemented
 */

/** 
 * Test: 0xA9 [LDA immediate] - zero and negative flags clear
 * Loads a constant into the A register
 */
TEST(CPUTest, LDAImmediateLoadData) {
    CPU cpu;
    std::vector<uint8_t> instructions = {0xA9, 0x05, 0x00}; // LDA #$05
    cpu.executeProgram(instructions);
    EXPECT_EQ(cpu.a_register, 0x05);            // 0x05 loaded into A register
    EXPECT_EQ(cpu.status & 0b00000010, 0x00);   // zero flag cleared
    EXPECT_EQ(cpu.status & 0b10000000, 0x00);   // negative flag cleared
}

/** 
 * Test: 0xA9 [LDA immediate] - zero flag set
 * Loads zero into the A register
 */
TEST(CPUTest, LDAZeroFlag) {
    CPU cpu;
    std::vector<uint8_t> instructions = {0xA9, 0x00, 0x00}; // LDA #$00
    cpu.executeProgram(instructions);
    EXPECT_EQ(cpu.status & 0b00000010, 0b10);  // Zero flag set
}
