#include "../include/OpCode.h"
#include "../include/CPU.h"

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

/** 
 * Opcode lookup table
 */ 
const std::unordered_map<uint8_t, OpCode> OPCODE_LOOKUP = {
    // 151 opcodes (CHECK WHEN FINISHED)
    // =====================================================
    // Control and Subroutine Instructions
    // =====================================================
    {0x00, OpCode(0x00, "BRK", 1, 7, AddressingMode::Implied,     &CPU::op_BRK)},
    {0x20, OpCode(0x20, "JSR", 3, 6, AddressingMode::Absolute,    &CPU::op_JSR)},
    {0x4C, OpCode(0x4C, "JMP", 3, 3, AddressingMode::Absolute,    &CPU::op_JMP)},
    {0x6C, OpCode(0x6C, "JMP", 3, 5, AddressingMode::Indirect,    &CPU::op_JMP)},
    {0x40, OpCode(0x40, "RTI", 1, 6, AddressingMode::Implied,     &CPU::op_RTI)},
    {0x60, OpCode(0x60, "RTS", 1, 6, AddressingMode::Implied,     &CPU::op_RTS)},
    {0xEA, OpCode(0xEA, "NOP", 1, 2, AddressingMode::Implied,     &CPU::op_NOP)},

    // =====================================================
    // Load/Store Instructions
    // =====================================================
    // --- LDA (Load Accumulator)
    {0xA9, OpCode(0xA9, "LDA", 2, 2, AddressingMode::Immediate,   &CPU::op_LDA)},
    {0xA5, OpCode(0xA5, "LDA", 2, 3, AddressingMode::ZeroPage,    &CPU::op_LDA)},
    {0xB5, OpCode(0xB5, "LDA", 2, 4, AddressingMode::ZeroPage_X,  &CPU::op_LDA)},
    {0xAD, OpCode(0xAD, "LDA", 3, 4, AddressingMode::Absolute,    &CPU::op_LDA)},
    {0xBD, OpCode(0xBD, "LDA", 3, 4, AddressingMode::Absolute_X,  &CPU::op_LDA)}, // +1 cycle if page crossed
    {0xB9, OpCode(0xB9, "LDA", 3, 4, AddressingMode::Absolute_Y,  &CPU::op_LDA)}, // +1 cycle if page crossed
    {0xA1, OpCode(0xA1, "LDA", 2, 6, AddressingMode::Indirect_X,  &CPU::op_LDA)},
    {0xB1, OpCode(0xB1, "LDA", 2, 5, AddressingMode::Indirect_Y,  &CPU::op_LDA)},

    // --- LDX (Load X Register)
    {0xA2, OpCode(0xA2, "LDX", 2, 2, AddressingMode::Immediate,   &CPU::op_LDX)},
    {0xA6, OpCode(0xA6, "LDX", 2, 3, AddressingMode::ZeroPage,    &CPU::op_LDX)},
    {0xB6, OpCode(0xB6, "LDX", 2, 4, AddressingMode::ZeroPage_Y,  &CPU::op_LDX)},
    {0xAE, OpCode(0xAE, "LDX", 3, 4, AddressingMode::Absolute,    &CPU::op_LDX)},
    {0xBE, OpCode(0xBE, "LDX", 3, 4, AddressingMode::Absolute_Y,  &CPU::op_LDX)}, // +1 cycle if page crossed

    // --- LDY (Load Y Register)
    {0xA0, OpCode(0xA0, "LDY", 2, 2, AddressingMode::Immediate,   &CPU::op_LDY)},
    {0xA4, OpCode(0xA4, "LDY", 2, 3, AddressingMode::ZeroPage,    &CPU::op_LDY)},
    {0xB4, OpCode(0xB4, "LDY", 2, 4, AddressingMode::ZeroPage_X,  &CPU::op_LDY)},
    {0xAC, OpCode(0xAC, "LDY", 3, 4, AddressingMode::Absolute,    &CPU::op_LDY)},
    {0xBC, OpCode(0xBC, "LDY", 3, 4, AddressingMode::Absolute_X,  &CPU::op_LDY)}, // +1 cycle if page crossed

    // --- STA (Store Accumulator)
    {0x85, OpCode(0x85, "STA", 2, 3, AddressingMode::ZeroPage,    &CPU::op_STA)},
    {0x95, OpCode(0x95, "STA", 2, 4, AddressingMode::ZeroPage_X,  &CPU::op_STA)},
    {0x8D, OpCode(0x8D, "STA", 3, 4, AddressingMode::Absolute,    &CPU::op_STA)},
    {0x9D, OpCode(0x9D, "STA", 3, 5, AddressingMode::Absolute_X,  &CPU::op_STA)},
    {0x99, OpCode(0x99, "STA", 3, 5, AddressingMode::Absolute_Y,  &CPU::op_STA)},
    {0x81, OpCode(0x81, "STA", 2, 6, AddressingMode::Indirect_X,  &CPU::op_STA)},
    {0x91, OpCode(0x91, "STA", 2, 6, AddressingMode::Indirect_Y,  &CPU::op_STA)},

    // --- STX (Store X Register)
    {0x86, OpCode(0x86, "STX", 2, 3, AddressingMode::ZeroPage,    &CPU::op_STX)},
    {0x8E, OpCode(0x8E, "STX", 3, 4, AddressingMode::Absolute,    &CPU::op_STX)},

    // --- STY (Store Y Register)
    {0x84, OpCode(0x84, "STY", 2, 3, AddressingMode::ZeroPage,    &CPU::op_STY)},
    {0x8C, OpCode(0x8C, "STY", 3, 4, AddressingMode::Absolute,    &CPU::op_STY)},

    // =====================================================
    // Arithmetic Instructions
    // =====================================================
    // --- ADC (Add with Carry)
    {0x69, OpCode(0x69, "ADC", 2, 2, AddressingMode::Immediate,   &CPU::op_ADC)},
    {0x65, OpCode(0x65, "ADC", 2, 3, AddressingMode::ZeroPage,    &CPU::op_ADC)},
    {0x75, OpCode(0x75, "ADC", 2, 4, AddressingMode::ZeroPage_X,  &CPU::op_ADC)},
    {0x6D, OpCode(0x6D, "ADC", 3, 4, AddressingMode::Absolute,    &CPU::op_ADC)},
    {0x7D, OpCode(0x7D, "ADC", 3, 4, AddressingMode::Absolute_X,  &CPU::op_ADC)}, // +1 cycle if page crossed
    {0x79, OpCode(0x79, "ADC", 3, 4, AddressingMode::Absolute_Y,  &CPU::op_ADC)}, // +1 cycle if page crossed
    {0x61, OpCode(0x61, "ADC", 2, 6, AddressingMode::Indirect_X,  &CPU::op_ADC)},
    {0x71, OpCode(0x71, "ADC", 2, 5, AddressingMode::Indirect_Y,  &CPU::op_ADC)}, // +1 cycle if page crossed

    // --- SBC (Subtract with Carry)
    {0xE9, OpCode(0xE9, "SBC", 2, 2, AddressingMode::Immediate,   &CPU::op_SBC)},
    {0xE5, OpCode(0xE5, "SBC", 2, 3, AddressingMode::ZeroPage,    &CPU::op_SBC)},
    {0xF5, OpCode(0xF5, "SBC", 2, 4, AddressingMode::ZeroPage_X,  &CPU::op_SBC)},
    {0xED, OpCode(0xED, "SBC", 3, 4, AddressingMode::Absolute,    &CPU::op_SBC)},
    {0xFD, OpCode(0xFD, "SBC", 3, 4, AddressingMode::Absolute_X,  &CPU::op_SBC)},
    {0xF9, OpCode(0xF9, "SBC", 3, 4, AddressingMode::Absolute_Y,  &CPU::op_SBC)},
    {0xE1, OpCode(0xE1, "SBC", 2, 6, AddressingMode::Indirect_X,  &CPU::op_SBC)},
    {0xF1, OpCode(0xF1, "SBC", 2, 5, AddressingMode::Indirect_Y,  &CPU::op_SBC)},

    // --- INC
    {0xE6, OpCode(0xE6, "INC", 2, 5, AddressingMode::ZeroPage,    &CPU::op_INC)},
    {0xF6, OpCode(0xF6, "INC", 2, 6, AddressingMode::ZeroPage_X,  &CPU::op_INC)},
    {0xEE, OpCode(0xEE, "INC", 3, 6, AddressingMode::Absolute,    &CPU::op_INC)},
    {0xFE, OpCode(0xFE, "INC", 3, 7, AddressingMode::Absolute_X,  &CPU::op_INC)},

    // --- INX
    {0xE8, OpCode(0xE8, "INX", 1, 2, AddressingMode::Implied,     &CPU::op_INX)},

    // --- INY
    {0xC8, OpCode(0xC8, "INY", 1, 2, AddressingMode::Implied,     &CPU::op_INY)},

    // --- DEC
    {0xC6, OpCode(0xC6, "DEC", 2, 5, AddressingMode::ZeroPage,    &CPU::op_DEC)},
    {0xD6, OpCode(0xD6, "DEC", 2, 6, AddressingMode::ZeroPage_X,  &CPU::op_DEC)},
    {0xCE, OpCode(0xCE, "DEC", 3, 6, AddressingMode::Absolute,    &CPU::op_DEC)},
    {0xDE, OpCode(0xDE, "DEC", 3, 7, AddressingMode::Absolute_X,  &CPU::op_DEC)},

    // --- DEX
    {0xCA, OpCode(0xCA, "DEX", 1, 2, AddressingMode::Implied, &CPU::op_DEX)},

    // --- DEY
    {0x88, OpCode(0x88, "DEY", 1, 2, AddressingMode::Implied, &CPU::op_DEY)},


    // =====================================================
    // Logical Instructions
    // =====================================================
    // --- AND
    {0x29, OpCode(0x29, "AND", 2, 2, AddressingMode::Immediate,   &CPU::op_AND)},
    {0x25, OpCode(0x25, "AND", 2, 3, AddressingMode::ZeroPage,    &CPU::op_AND)},
    {0x35, OpCode(0x35, "AND", 2, 4, AddressingMode::ZeroPage_X,  &CPU::op_AND)},
    {0x2D, OpCode(0x2D, "AND", 3, 4, AddressingMode::Absolute,    &CPU::op_AND)},
    {0x3D, OpCode(0x3D, "AND", 3, 4, AddressingMode::Absolute_X,  &CPU::op_AND)}, // +1 cycle if page crossed
    {0x39, OpCode(0x39, "AND", 3, 4, AddressingMode::Absolute_Y,  &CPU::op_AND)}, // +1 cycle if page crossed
    {0x21, OpCode(0x21, "AND", 2, 6, AddressingMode::Indirect_X,  &CPU::op_AND)},
    {0x31, OpCode(0x31, "AND", 2, 5, AddressingMode::Indirect_Y,  &CPU::op_AND)}, // +1 cycle if page crossed

    // --- ORA
    {0x09, OpCode(0x09, "ORA", 2, 2, AddressingMode::Immediate,   &CPU::op_ORA)},
    {0x05, OpCode(0x05, "ORA", 2, 3, AddressingMode::ZeroPage,    &CPU::op_ORA)},
    {0x15, OpCode(0x15, "ORA", 2, 4, AddressingMode::ZeroPage_X,  &CPU::op_ORA)},
    {0x0D, OpCode(0x0D, "ORA", 3, 4, AddressingMode::Absolute,    &CPU::op_ORA)},
    {0x1D, OpCode(0x1D, "ORA", 3, 4, AddressingMode::Absolute_X,  &CPU::op_ORA)},
    {0x19, OpCode(0x19, "ORA", 3, 4, AddressingMode::Absolute_Y,  &CPU::op_ORA)},
    {0x01, OpCode(0x01, "ORA", 2, 6, AddressingMode::Indirect_X,  &CPU::op_ORA)},
    {0x11, OpCode(0x11, "ORA", 2, 5, AddressingMode::Indirect_Y,  &CPU::op_ORA)},

    // --- EOR
    {0x49, OpCode(0x49, "EOR", 2, 2, AddressingMode::Immediate,   &CPU::op_EOR)},
    {0x45, OpCode(0x45, "EOR", 2, 3, AddressingMode::ZeroPage,    &CPU::op_EOR)},
    {0x55, OpCode(0x55, "EOR", 2, 4, AddressingMode::ZeroPage_X,  &CPU::op_EOR)},
    {0x4D, OpCode(0x4D, "EOR", 3, 4, AddressingMode::Absolute,    &CPU::op_EOR)},
    {0x5D, OpCode(0x5D, "EOR", 3, 4, AddressingMode::Absolute_X,  &CPU::op_EOR)},
    {0x59, OpCode(0x59, "EOR", 3, 4, AddressingMode::Absolute_Y,  &CPU::op_EOR)},
    {0x41, OpCode(0x41, "EOR", 2, 6, AddressingMode::Indirect_X,  &CPU::op_EOR)},
    {0x51, OpCode(0x51, "EOR", 2, 5, AddressingMode::Indirect_Y,  &CPU::op_EOR)},

    // --- BIT
    {0x24, OpCode(0x24, "BIT", 2, 3, AddressingMode::ZeroPage,    &CPU::op_BIT)},
    {0x2C, OpCode(0x2C, "BIT", 3, 4, AddressingMode::Absolute,    &CPU::op_BIT)},

    // =====================================================
    // Shift and Rotate Instructions
    // =====================================================
    // --- ASL
    {0x0A, OpCode(0x0A, "ASL", 1, 2, AddressingMode::Accumulator, &CPU::op_ASL_ACC)},
    {0x06, OpCode(0x06, "ASL", 2, 5, AddressingMode::ZeroPage,    &CPU::op_ASL)},
    {0x16, OpCode(0x16, "ASL", 2, 6, AddressingMode::ZeroPage_X,  &CPU::op_ASL)},
    {0x0E, OpCode(0x0E, "ASL", 3, 6, AddressingMode::Absolute,    &CPU::op_ASL)},
    {0x1E, OpCode(0x1E, "ASL", 3, 7, AddressingMode::Absolute_X,  &CPU::op_ASL)},

    // --- LSR
    {0x4A, OpCode(0x4A, "LSR", 1, 2, AddressingMode::Accumulator, &CPU::op_LSR_ACC)},
    {0x46, OpCode(0x46, "LSR", 2, 5, AddressingMode::ZeroPage,    &CPU::op_LSR)},
    {0x56, OpCode(0x56, "LSR", 2, 6, AddressingMode::ZeroPage_X,  &CPU::op_LSR)},
    {0x4E, OpCode(0x4E, "LSR", 3, 6, AddressingMode::Absolute,    &CPU::op_LSR)},
    {0x5E, OpCode(0x5E, "LSR", 3, 7, AddressingMode::Absolute_X,  &CPU::op_LSR)},

    // --- ROL
    {0x2A, OpCode(0x2A, "ROL", 1, 2, AddressingMode::Accumulator, &CPU::op_ROL_ACC)},
    {0x26, OpCode(0x26, "ROL", 2, 5, AddressingMode::ZeroPage,    &CPU::op_ROL)},
    {0x36, OpCode(0x36, "ROL", 2, 6, AddressingMode::ZeroPage_X,  &CPU::op_ROL)},
    {0x2E, OpCode(0x2E, "ROL", 3, 6, AddressingMode::Absolute,    &CPU::op_ROL)},
    {0x3E, OpCode(0x3E, "ROL", 3, 7, AddressingMode::Absolute_X,  &CPU::op_ROL)},

    // --- ROR
    {0x6A, OpCode(0x6A, "ROR", 1, 2, AddressingMode::Accumulator, &CPU::op_ROR_ACC)},
    {0x66, OpCode(0x66, "ROR", 2, 5, AddressingMode::ZeroPage,    &CPU::op_ROR)},
    {0x76, OpCode(0x76, "ROR", 2, 6, AddressingMode::ZeroPage_X,  &CPU::op_ROR)},
    {0x6E, OpCode(0x6E, "ROR", 3, 6, AddressingMode::Absolute,    &CPU::op_ROR)},
    {0x7E, OpCode(0x7E, "ROR", 3, 7, AddressingMode::Absolute_X,  &CPU::op_ROR)},

    // =====================================================
    // Branch Instructions
    // =====================================================
    // 2 cycles if not taken (base, added by execution loop)
	  // +1 cycles if taken (total 3)
	  // +2 cycles if taken and crossing a page (total 4) 
    {0x10, OpCode(0x10, "BPL", 2, 2, AddressingMode::Relative, &CPU::op_BPL)},
    {0x30, OpCode(0x30, "BMI", 2, 2, AddressingMode::Relative, &CPU::op_BMI)},
    {0x50, OpCode(0x50, "BVC", 2, 2, AddressingMode::Relative, &CPU::op_BVC)},
    {0x70, OpCode(0x70, "BVS", 2, 2, AddressingMode::Relative, &CPU::op_BVS)},
    {0x90, OpCode(0x90, "BCC", 2, 2, AddressingMode::Relative, &CPU::op_BCC)},
    {0xB0, OpCode(0xB0, "BCS", 2, 2, AddressingMode::Relative, &CPU::op_BCS)},
    {0xD0, OpCode(0xD0, "BNE", 2, 2, AddressingMode::Relative, &CPU::op_BNE)},
    {0xF0, OpCode(0xF0, "BEQ", 2, 2, AddressingMode::Relative, &CPU::op_BEQ)},

    // =====================================================
    // Compare Instructions
    // =====================================================
    // --- CMP
    {0xC9, OpCode(0xC9, "CMP", 2, 2, AddressingMode::Immediate,   &CPU::op_CMP)},
    {0xC5, OpCode(0xC5, "CMP", 2, 3, AddressingMode::ZeroPage,    &CPU::op_CMP)},
    {0xD5, OpCode(0xD5, "CMP", 2, 4, AddressingMode::ZeroPage_X,  &CPU::op_CMP)},
    {0xCD, OpCode(0xCD, "CMP", 3, 4, AddressingMode::Absolute,    &CPU::op_CMP)},
    {0xDD, OpCode(0xDD, "CMP", 3, 4, AddressingMode::Absolute_X,  &CPU::op_CMP)},
    {0xD9, OpCode(0xD9, "CMP", 3, 4, AddressingMode::Absolute_Y,  &CPU::op_CMP)},
    {0xC1, OpCode(0xC1, "CMP", 2, 6, AddressingMode::Indirect_X,  &CPU::op_CMP)},
    {0xD1, OpCode(0xD1, "CMP", 2, 5, AddressingMode::Indirect_Y,  &CPU::op_CMP)},

    // --- CPX
    {0xE0, OpCode(0xE0, "CPX", 2, 2, AddressingMode::Immediate,   &CPU::op_CPX)},
    {0xE4, OpCode(0xE4, "CPX", 2, 3, AddressingMode::ZeroPage,    &CPU::op_CPX)},
    {0xEC, OpCode(0xEC, "CPX", 3, 4, AddressingMode::Absolute,    &CPU::op_CPX)},

    // --- CPY
    {0xC0, OpCode(0xC0, "CPY", 2, 2, AddressingMode::Immediate,   &CPU::op_CPY)},
    {0xC4, OpCode(0xC4, "CPY", 2, 3, AddressingMode::ZeroPage,    &CPU::op_CPY)},
    {0xCC, OpCode(0xCC, "CPY", 3, 4, AddressingMode::Absolute,    &CPU::op_CPY)},

    // =====================================================
    // Stack and Register Transfer Instructions
    // =====================================================
    // --- Stack Operations
    {0x48, OpCode(0x48, "PHA", 1, 3, AddressingMode::Implied, &CPU::op_PHA)},
    {0x08, OpCode(0x08, "PHP", 1, 3, AddressingMode::Implied, &CPU::op_PHP)},
    {0x68, OpCode(0x68, "PLA", 1, 4, AddressingMode::Implied, &CPU::op_PLA)},
    {0x28, OpCode(0x28, "PLP", 1, 4, AddressingMode::Implied, &CPU::op_PLP)},

    // --- Register Transfers
    {0xAA, OpCode(0xAA, "TAX", 1, 2, AddressingMode::Implied, &CPU::op_TAX)},
    {0xA8, OpCode(0xA8, "TAY", 1, 2, AddressingMode::Implied, &CPU::op_TAY)},
    {0xBA, OpCode(0xBA, "TSX", 1, 2, AddressingMode::Implied, &CPU::op_TSX)},
    {0x8A, OpCode(0x8A, "TXA", 1, 2, AddressingMode::Implied, &CPU::op_TXA)},
    {0x9A, OpCode(0x9A, "TXS", 1, 2, AddressingMode::Implied, &CPU::op_TXS)},
    {0x98, OpCode(0x98, "TYA", 1, 2, AddressingMode::Implied, &CPU::op_TYA)},

    // =====================================================
    // Flag Instructions
    // =====================================================
    {0x18, OpCode(0x18, "CLC", 1, 2, AddressingMode::Implied, &CPU::op_CLC)},
    {0x38, OpCode(0x38, "SEC", 1, 2, AddressingMode::Implied, &CPU::op_SEC)},
    {0x58, OpCode(0x58, "CLI", 1, 2, AddressingMode::Implied, &CPU::op_CLI)},
    {0x78, OpCode(0x78, "SEI", 1, 2, AddressingMode::Implied, &CPU::op_SEI)},
    {0xB8, OpCode(0xB8, "CLV", 1, 2, AddressingMode::Implied, &CPU::op_CLV)},
    {0xD8, OpCode(0xD8, "CLD", 1, 2, AddressingMode::Implied, &CPU::op_CLD)},
    {0xF8, OpCode(0xF8, "SED", 1, 2, AddressingMode::Implied, &CPU::op_SED)},

};