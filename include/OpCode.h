#ifndef OPCODE_H
#define OPCODE_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <functional>

class CPU; // forward declare CPU so we can use it in function pointers

// enum class for addressing modes
enum class AddressingMode {
  NoneAddressing,
  Immediate,
  ZeroPage,
  ZeroPage_X,
  ZeroPage_Y,
  Absolute,
  Absolute_X,
  Absolute_Y,
  Indirect_X,
  Indirect_Y
};

struct OpCode {
  uint8_t code;
  std::string name;
  uint8_t bytes;
  uint8_t cycles;
  AddressingMode mode;
  std::function<void(CPU&, AddressingMode)> execute;

  OpCode(uint8_t code, std::string name, uint8_t bytes, uint8_t cycles,
         AddressingMode mode, std::function<void(CPU&, AddressingMode)> execute)
      : code(code),
        name(std::move(name)),
        bytes(bytes),
        cycles(cycles),
        mode(mode),
        execute(std::move(execute)) {}
};

extern const OpCode* getOpCode(uint8_t opcode);
extern const std::unordered_map<uint8_t, OpCode> OPCODE_LOOKUP;

#endif  // OPCODE_H