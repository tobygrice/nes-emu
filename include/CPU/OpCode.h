#ifndef OPCODE_H
#define OPCODE_H

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

class CPU;  // forward declare CPU so we can use it in function pointers

// enum class for addressing modes
enum class AddressingMode {
  Implied,
  Relative,
  Acc,
  Immediate,
  ZeroPage,
  ZeroPageX,
  ZeroPageY,
  Absolute,
  AbsoluteX,
  AbsoluteY,
  Indirect,
  IndirectX,
  IndirectY
};

using InstructionHandler = void (CPU::*)(uint16_t);

struct OpCode {
  uint8_t code;
  bool isDocumented;
  std::string name;
  uint8_t bytes;
  uint8_t cycles;
  AddressingMode mode;
  bool ignorePageCrossings;
  InstructionHandler handler;

  OpCode(uint8_t code, bool isDocumented, std::string name, uint8_t bytes,
         uint8_t cycles, AddressingMode mode, bool ignorePageCrossings,
         InstructionHandler handler)
      : code(code),
        isDocumented(isDocumented),
        name(std::move(name)),
        bytes(bytes),
        cycles(cycles),
        mode(mode),
        ignorePageCrossings(ignorePageCrossings),
        handler(handler) {}
};

extern const OpCode* getOpCode(uint8_t opcode);
extern const std::unordered_map<uint8_t, OpCode> OPCODE_LOOKUP;

#endif  // OPCODE_H