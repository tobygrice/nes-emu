#include "../include/OpCode.h"
#include "../include/CPU.h"

// opcode lookup table
const std::unordered_map<uint8_t, OpCode> OPCODE_LOOKUP = {
    {0x00, OpCode(0x00, "BRK", 1, 7, AddressingMode::NoneAddressing,
                  [](CPU& cpu, AddressingMode mode) { cpu.brk(); })},

    {0xA9, OpCode(0xA9, "LDA", 2, 2, AddressingMode::Immediate,
                  [](CPU& cpu, AddressingMode mode) { cpu.lda(mode); })},

    {0xA5, OpCode(0xA5, "LDA", 2, 3, AddressingMode::ZeroPage,
                  [](CPU& cpu, AddressingMode mode) { cpu.lda(mode); })},

    {0xB5, OpCode(0xB5, "LDA", 2, 4, AddressingMode::ZeroPage_X,
                  [](CPU& cpu, AddressingMode mode) { cpu.lda(mode); })},

    {0xAD, OpCode(0xAD, "LDA", 3, 4, AddressingMode::Absolute,
                  [](CPU& cpu, AddressingMode mode) { cpu.lda(mode); })},

    {0xBD, OpCode(0xBD, "LDA", 3, 4, AddressingMode::Absolute_X,
                  [](CPU& cpu, AddressingMode mode) { cpu.lda(mode); })},

    {0xB9, OpCode(0xB9, "LDA", 3, 4, AddressingMode::Absolute_Y,
                  [](CPU& cpu, AddressingMode mode) { cpu.lda(mode); })},

    {0xA1, OpCode(0xA1, "LDA", 2, 6, AddressingMode::Indirect_X,
                  [](CPU& cpu, AddressingMode mode) { cpu.lda(mode); })},

    {0xB1, OpCode(0xB1, "LDA", 2, 5, AddressingMode::Indirect_Y,
                  [](CPU& cpu, AddressingMode mode) { cpu.lda(mode); })}};

/**
 * Look up an opcode in the lookup table.
 *
 * @param opcode 6502 opcode.
 * @return An OpCode object.
 */
const OpCode* getOpCode(uint8_t opcode) {
  auto it = OPCODE_LOOKUP.find(opcode);
  if (it != OPCODE_LOOKUP.end()) {
    return &it->second;
  }
  return nullptr;  // not found
}