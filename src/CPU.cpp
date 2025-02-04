#include "../include/CPU.h"

CPU::CPU(): 
  a_register(0),  // accumulator starts at 0
  x_register(0),  // X starts at 0
  y_register(0),  // Y starts at 0
  status(0x00),   // status register starts with all flags clear
  pc(0x0000),     // program counter starts at 0
  sp(0xFF) {}     // stack pointer starts at 0xFF

void CPU::updateZeroAndNegativeFlags(uint8_t result) {
    // zero flag is bit 1
    if (result == 0) {
        status |= 0b00000010; // set zero flag if result is 0
    } else {
        status &= 0b11111101; // else clear zero flag
    }

    // negative flag is bit 7
    if (result & 0b10000000) {
        status |= 0b10000000; // set negative flag if bit 7 of result is 1
    } else {
        status &= 0b01111111; // else clear negative flag
    }
}

void CPU::executeProgram(const std::vector<uint8_t>& instructions) {
  pc = 0; // reset program counter

  while (true) {
    if (pc >= instructions.size()) {
      throw std::runtime_error("Instruction set terminated unexpectedly.");
    }
    
    // get next opcode and increment pc
    uint8_t opcode = instructions[pc];
    pc += 1;
    
    // decode and execute instruction
    // https://www.nesdev.org/obelisk-6502-guide/reference.html#LDA
    switch (opcode) {
      case 0xA9: { 
        // LDA immediate: loads a constant into the A register
        if (pc >= instructions.size()) {
          throw std::runtime_error("Expected parameter after opcode 0xA9 [LDA immediate]");
        }
        
        // get param from instruction set
        uint8_t param = instructions[pc];
        pc += 1;

        // set accumulator to value of param and update flags
        a_register = param;
        updateZeroAndNegativeFlags(a_register);
        break;
      }
      case 0x00: // BRK
        return;
      default:
        throw std::runtime_error("Invalid opcode.");
    }
  }
}