#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU.h"
#include "../../include/OpCode.h"
#include "../../include/TestBus.h"
#include "../../include/Logger.h"

// tests assisted by a LLM

class CPUBranchingTest : public ::testing::Test {
 protected:
  TestBus bus;  // create a shared bus
  CPU cpu;  // CPU instance that uses the shared bus
  Logger logger;

  CPUBranchingTest() : bus(), cpu(&bus, &logger) {}
};

//
// -------------------------
// Branch with negative offset (BNE) - tests branch target is calculated
// correctly
// -------------------------
//
TEST_F(CPUBranchingTest, BNE_Negative_Offset) {
  std::vector<uint8_t> program = {
      0xA2, 0x08,  // LDX #$08                        2
      0xCA,        // DEX (branch target)             2
      0xE0, 0x03,  // CPX #$03                        2
      0xD0, 0xFB,  // BNE -5      ;branch if x!=3     2 if not taken, 3 if taken
      0x00         // BRK                             7
  };
  cpu.loadAndExecute(program);
  EXPECT_EQ(cpu.getX(), 0x03) << "X should be decremented until it equals 3";
  // 2 + 2 (X=7) + 2 + 3 +
  // 2 (X=6) + 2 + 3 +
  // 2 (X=5) + 2 + 3 +
  // 2 (X=4) + 2 + 3 +
  // 2 (X=3) + 2 + 2 + 7

  // 2 + 2 + 2 + 3 + 2 + 2 + 3 + 2 + 2 + 3 + 2 + 2 + 3 + 2 + 2 + 2 + 7 = 43
  // EXPECT_EQ(cpu.getCycleCount(), 43) << "Cycle count should be 43";
}

//
// -------------------------
// BCC (Branch if Carry Clear; branch if the carry flag is clear)
// -------------------------
//

// Test 1: BCC not taken because the carry flag is set.
// We set the carry flag by overflowing with ADC. (Since SEC isn’t implemented.)
TEST_F(CPUBranchingTest, BCC_NotTaken) {
  // Program layout (starting at 0x8000):
  //   0x8000: LDA #$FF      (2 bytes, 2 cycles)
  //   0x8002: ADC #$01      (2 bytes, 2 cycles)
  //           -> 0xFF + 0x01 = 0x100, so A becomes 0x00 and the carry flag is
  //           set.
  //   0x8004: BCC +3        (2 bytes, 2 cycles when not taken)
  //           Since carry is set, branch is NOT taken.
  //   0x8006: LDA #$42      (2 bytes, 2 cycles) – this instruction is executed.
  //   0x8008: BRK           (1 byte, 7 cycles) – program ends.
  //   0x8009: LDA #$99      (2 bytes) – branch target if taken (should not
  //   run). 0x800B: BRK           (1 byte)
  std::vector<uint8_t> program = {
      0xA9, 0xFF,  // LDA #$FF
      0x69, 0x01,  // ADC #$01   -> sets the carry flag (result overflows)
      0x90, 0x03,  // BCC +3     -> if taken, would jump to address 0x8009
      0xA9, 0x42,  // LDA #$42   -> executed when branch is NOT taken
      0x00,        // BRK
      0xA9, 0x99,  // LDA #$99   -> branch target (should be skipped)
      0x00         // BRK
  };

  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x42) << "With carry set (from ADC), BCC should not "
                                 "branch and LDA #$42 should execute.";
  // Cycle count:
  //   LDA #$FF:    2 cycles
  //   ADC #$01:    2 cycles
  //   BCC not taken: 2 cycles
  //   LDA #$42:    2 cycles
  //   BRK:         7 cycles
  // Total = 2 + 2 + 2 + 2 + 7 = 15 cycles.
  // EXPECT_EQ(cpu.getCycleCount(), 15)
      // << "Cycle count should be 15 when branch is not taken.";
}

// Test 2: BCC taken without a page crossing.
// Here the carry flag is clear (CPU resets with flags clear or via LDA #$00),
// so the branch is taken.
TEST_F(CPUBranchingTest, BCC_Taken_NoPageCross) {
  // Program layout (starting at 0x8000):
  //   0x8000: LDA #$00      (2 bytes, 2 cycles) – ensures carry is clear.
  //   0x8002: BCC +2        (2 bytes, 3 cycles when branch is taken with no
  //   page cross)
  //           After BCC, the next PC is 0x8004. With offset +2, branch target
  //           is 0x8006.
  //   0x8004: LDA #$55      (2 bytes) – should be skipped.
  //   0x8006: LDA #$AA      (2 bytes, 2 cycles) – branch target.
  //   0x8008: BRK           (1 byte, 7 cycles) – program ends.
  //   0x8009: LDA #$FF      (2 bytes) – if branch not taken (should not occur).
  //   0x800B: BRK           (1 byte)
  std::vector<uint8_t> program = {
      0xA9, 0x00,  // LDA #$00, ensures carry flag is clear.
      0x90, 0x02,  // BCC +2 -> branch taken because carry is clear.
      0xA9, 0x55,  // LDA #$55 (skipped if branch is taken)
      0xA9, 0xAA,  // LDA #$AA (branch target)
      0x00,        // BRK
      0xA9, 0xFF,  // LDA #$FF (would be executed if branch not taken)
      0x00         // BRK
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
//   EXPECT_EQ(cpu.getCycleCount(), 14)
//       << "Cycle count should be 14 when branch is taken without a page "
//          "boundary crossing.";
}

// Test 3: BCC taken with a page crossing.
// We set the starting PC so that the branch target falls in the next page,
// causing the branch instruction to incur an extra cycle.
TEST_F(CPUBranchingTest, BCC_Taken_PageCross) {
  // Set the starting program counter to 0x80F0.
  cpu.setPC(0x80F0);

  // Program layout (addresses starting at 0x80F0):
  //   0x80F0: LDA #$00      (2 bytes, 2 cycles) – ensures carry is clear.
  //   0x80F2: BCC +0x0C     (2 bytes, 4 cycles when branch is taken with a page
  //   crossing)
  //           After BCC, PC = 0x80F4.
  //           With an offset of 0x0C, branch target = 0x80F4 + 0x0C = 0x8100
  //           (different page).
  //   0x80F4–0x80FF: 12 filler bytes.
  //      (Since we do not have a NOP instruction in the allowed set, we fill
  //      with BRK (0x00).
  //       These filler bytes are not executed when the branch is taken.)
  //   0x8100: LDA #$AA      (2 bytes, 2 cycles) – branch target.
  //   0x8102: BRK           (1 byte, 7 cycles) – program ends.
  std::vector<uint8_t> program = {
      0xA9, 0x00,  // LDA #$00 at 0x80F0
      0x90, 0x0C,  // BCC +12 at 0x80F2; next PC is 0x80F4; target = 0x80F4 +
                   // 0x0C = 0x8100
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00,        // filler
      0xA9, 0xAA,  // LDA #$AA (branch target)
      0x00         // BRK
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
//   EXPECT_EQ(cpu.getCycleCount(), 15)
//       << "Cycle count should be 15 when branch is taken with a page boundary "
//          "crossing.";
}

//
// -------------------------
// BPL (Branch if Plus, i.e. if Negative flag is clear)
// -------------------------
//

TEST_F(CPUBranchingTest, BPL_NotTaken) {
  std::vector<uint8_t> program = {
      0xA9, 0x80,  // LDA #$80 (sets negative flag)
      0x10, 0x02,  // BPL +2 (should not branch because negative flag is set)
      0xA9, 0x42,  // LDA #$42 (executed)
      0x00,        // BRK
  };
  cpu.loadAndExecute(program);
  EXPECT_EQ(cpu.getA(), 0x42)
      << "With negative flag set, BPL should not branch.";
  // Cycle count: LDA (2) + BPL not taken (2) + LDA (2) + BRK (7) = 13 cycles.
//   EXPECT_EQ(cpu.getCycleCount(), 13)
//       << "Cycle count should be 13 when BPL is not taken.";
}

TEST_F(CPUBranchingTest, BPL_Taken) {
  // Here negative flag is clear (LDA #$00) so BPL is taken.
  std::vector<uint8_t> program = {0xA9, 0x00,  // LDA #$00 (negative flag clear)
                                  0x10, 0x01,  // BPL +1 (branch taken)
                                  0x00,        // BRK
                                  0xA9, 0xAA,  // LDA #$AA (branch target)
                                  0x00};
  cpu.loadAndExecute(program);
  EXPECT_EQ(cpu.getA(), 0xAA)
      << "With negative flag clear, BPL should branch to LDA #$AA.";
  // Cycle count: 2 + 3 + 2 + 7 = 14 cycles.
//   EXPECT_EQ(cpu.getCycleCount(), 14)
//       << "Cycle count should be 14 when BPL is taken without page crossing.";
}

//
// -------------------------
// BMI (Branch if Minus; branch if Negative flag is set)
// -------------------------
//

TEST_F(CPUBranchingTest, BMI_NotTaken) {
  // For BMI to branch, the negative flag must be set.
  // Here we clear the negative flag.
  std::vector<uint8_t> program = {
      0xA9, 0x00,  // LDA #$00 (negative flag clear)
      0x30, 0x02,  // BMI +2 (should not branch)
      0xA9, 0x42,  // LDA #$42 (executed)
      0x00,        // BRK
  };
  cpu.loadAndExecute(program);
  EXPECT_EQ(cpu.getA(), 0x42)
      << "With negative flag clear, BMI should not branch.";
//   EXPECT_EQ(cpu.getCycleCount(), 13)
//       << "Cycle count should be 13 when BMI is not taken.";
}

TEST_F(CPUBranchingTest, BMI_Taken) {
  std::vector<uint8_t> program = {0xA9, 0x81,  // LDA #$81 (negative flag set)
                                  0x30, 0x01,  // BMI +1 (branch taken)
                                  0x00,        // BRK
                                  0xA9, 0x55,  // branch target
                                  0x00};
  cpu.loadAndExecute(program);
  EXPECT_EQ(cpu.getA(), 0x55)
      << "With negative flag set, BMI should branch to LDA #$AA.";
//   EXPECT_EQ(cpu.getCycleCount(), 14)
//       << "Cycle count should be 14 when BMI is taken without page crossing.";
}
