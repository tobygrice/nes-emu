#ifndef OPCODE_H
#define OPCODE_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <functional>

class CPU; // forward declare CPU so we can use it in function pointers

// enum class for addressing modes
enum class AddressingMode {
  Implied,
  Relative,
  Accumulator,
  Immediate,
  ZeroPage,
  ZeroPage_X,
  ZeroPage_Y,
  Absolute,
  Absolute_X,
  Absolute_Y,
  Indirect,
  Indirect_X,
  Indirect_Y
};

using InstructionHandler = void (CPU::*)(uint16_t);

struct OpCode {
  uint8_t code;
  std::string name;
  uint8_t bytes;
  uint8_t cycles;
  AddressingMode mode;
  InstructionHandler handler;

  OpCode(uint8_t code, std::string name, uint8_t bytes, uint8_t cycles,
         AddressingMode mode, InstructionHandler handler)
      : code(code),
        name(std::move(name)),
        bytes(bytes),
        cycles(cycles),
        mode(mode),
        handler(handler) {}
};

extern const OpCode* getOpCode(uint8_t opcode);
extern const std::unordered_map<uint8_t, OpCode> OPCODE_LOOKUP;

#endif  // OPCODE_H