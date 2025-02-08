#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU.h"
#include "../../include/OpCode.h"

// Define a test fixture for CPU tests.
class CPUBranchingTest : public ::testing::Test {
 protected:
  CPU cpu;
};

// The CPUBranchingTest fixture gives you access to an emulator instance called "cpu".
// It supports methods such as:
//    cpu.loadAndExecute(program)   // loads a vector of bytes as the program and executes it.
//    cpu.setPC(address)            // sets the program counter (for branch‐crossing tests)
//    cpu.getA()                    // returns the current A register value.
//    cpu.getCycleCount()           // returns the total cycle count executed.


// Test 1: BCC not taken because the carry flag is set.
// We set the carry flag by overflowing with ADC. (Since SEC isn’t implemented.)
TEST_F(CPUBranchingTest, BCC_NotTaken) {
  // Program layout (starting at 0x8000):
  //   0x8000: LDA #$FF      (2 bytes, 2 cycles)
  //   0x8002: ADC #$01      (2 bytes, 2 cycles)
  //           -> 0xFF + 0x01 = 0x100, so A becomes 0x00 and the carry flag is set.
  //   0x8004: BCC +3        (2 bytes, 2 cycles when not taken)
  //           Since carry is set, branch is NOT taken.
  //   0x8006: LDA #$42      (2 bytes, 2 cycles) – this instruction is executed.
  //   0x8008: BRK           (1 byte, 7 cycles) – program ends.
  //   0x8009: LDA #$99      (2 bytes) – branch target if taken (should not run).
  //   0x800B: BRK           (1 byte)
  std::vector<uint8_t> program = {
      0xA9, 0xFF,    // LDA #$FF
      0x69, 0x01,    // ADC #$01   -> sets the carry flag (result overflows)
      0x90, 0x03,    // BCC +3     -> if taken, would jump to address 0x8009
      0xA9, 0x42,    // LDA #$42   -> executed when branch is NOT taken
      0x00,          // BRK
      0xA9, 0x99,    // LDA #$99   -> branch target (should be skipped)
      0x00           // BRK
  };

  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x42)
      << "With carry set (from ADC), BCC should not branch and LDA #$42 should execute.";
  // Cycle count:
  //   LDA #$FF:    2 cycles
  //   ADC #$01:    2 cycles
  //   BCC not taken: 2 cycles
  //   LDA #$42:    2 cycles
  //   BRK:         7 cycles
  // Total = 2 + 2 + 2 + 2 + 7 = 15 cycles.
  EXPECT_EQ(cpu.getCycleCount(), 15)
      << "Cycle count should be 15 when branch is not taken.";
}


// Test 2: BCC taken without a page crossing.
// Here the carry flag is clear (CPU resets with flags clear or via LDA #$00),
// so the branch is taken.
TEST_F(CPUBranchingTest, BCC_Taken_NoPageCross) {
  // Program layout (starting at 0x8000):
  //   0x8000: LDA #$00      (2 bytes, 2 cycles) – ensures carry is clear.
  //   0x8002: BCC +2        (2 bytes, 3 cycles when branch is taken with no page cross)
  //           After BCC, the next PC is 0x8004. With offset +2, branch target is 0x8006.
  //   0x8004: LDA #$55      (2 bytes) – should be skipped.
  //   0x8006: LDA #$AA      (2 bytes, 2 cycles) – branch target.
  //   0x8008: BRK           (1 byte, 7 cycles) – program ends.
  //   0x8009: LDA #$FF      (2 bytes) – if branch not taken (should not occur).
  //   0x800B: BRK           (1 byte)
  std::vector<uint8_t> program = {
      0xA9, 0x00,    // LDA #$00, ensures carry flag is clear.
      0x90, 0x02,    // BCC +2 -> branch taken because carry is clear.
      0xA9, 0x55,    // LDA #$55 (skipped if branch is taken)
      0xA9, 0xAA,    // LDA #$AA (branch target)
      0x00,          // BRK
      0xA9, 0xFF,    // LDA #$FF (would be executed if branch not taken)
      0x00           // BRK
  };

  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0xAA)
      << "With carry clear, BCC should branch to LDA #$AA.";
  // Cycle count:
  //   LDA #$00:       2 cycles
  //   BCC taken (no page cross): 3 cycles
  //   LDA #$AA:       2 cycles
  //   BRK:            7 cycles
  // Total = 2 + 3 + 2 + 7 = 14 cycles.
  EXPECT_EQ(cpu.getCycleCount(), 14)
      << "Cycle count should be 14 when branch is taken without a page boundary crossing.";
}


// Test 3: BCC taken with a page crossing.
// We set the starting PC so that the branch target falls in the next page,
// causing the branch instruction to incur an extra cycle.
TEST_F(CPUBranchingTest, BCC_Taken_PageCross) {
  // Set the starting program counter to 0x80F0.
  cpu.setPC(0x80F0);

  // Program layout (addresses starting at 0x80F0):
  //   0x80F0: LDA #$00      (2 bytes, 2 cycles) – ensures carry is clear.
  //   0x80F2: BCC +0x0C     (2 bytes, 4 cycles when branch is taken with a page crossing)
  //           After BCC, PC = 0x80F4.
  //           With an offset of 0x0C, branch target = 0x80F4 + 0x0C = 0x8100 (different page).
  //   0x80F4–0x80FF: 12 filler bytes.
  //      (Since we do not have a NOP instruction in the allowed set, we fill with BRK (0x00).
  //       These filler bytes are not executed when the branch is taken.)
  //   0x8100: LDA #$AA      (2 bytes, 2 cycles) – branch target.
  //   0x8102: BRK           (1 byte, 7 cycles) – program ends.
  std::vector<uint8_t> program = {
      0xA9, 0x00, // LDA #$00 at 0x80F0
      0x90, 0x0C, // BCC +12 at 0x80F2; next PC is 0x80F4; target = 0x80F4 + 0x0C = 0x8100
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // filler
      0xA9, 0xAA, // LDA #$AA (branch target)
      0x00        // BRK
  };

  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0xAA)
      << "With carry clear and a page crossing, BCC should branch to LDA #$AA.";
  // Cycle count:
  //   LDA #$00:                  2 cycles
  //   BCC taken with page cross: 4 cycles
  //   LDA #$AA:                  2 cycles
  //   BRK:                       7 cycles
  // Total = 2 + 4 + 2 + 7      = 15 cycles.
  EXPECT_EQ(cpu.getCycleCount(), 15)
      << "Cycle count should be 15 when branch is taken with a page boundary crossing.";
}