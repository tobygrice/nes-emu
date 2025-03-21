#include "../../include/CPU/OpCode.h"

#include "../../include/CPU/CPU.h"

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
    // 151 official opcodes
    // =====================================================
    // Control and Subroutine Instructions
    // =====================================================
    // all control/subroutine instructions -1 cycle to account for implementation
    {0x00, OpCode(0x00, true, "BRK", 2, 6, AddressingMode::Implied, false, &CPU::op_BRK)},
    {0x20, OpCode(0x20, true, "JSR", 3, 5, AddressingMode::Absolute, false, &CPU::op_JSR)},
    {0x4C, OpCode(0x4C, true, "JMP", 3, 2, AddressingMode::Absolute, false, &CPU::op_JMP)},
    {0x6C, OpCode(0x6C, true, "JMP", 3, 4, AddressingMode::Indirect, false, &CPU::op_JMP)},
    {0x40, OpCode(0x40, true, "RTI", 1, 5, AddressingMode::Implied, false, &CPU::op_RTI)},
    {0x60, OpCode(0x60, true, "RTS", 1, 5, AddressingMode::Implied, false, &CPU::op_RTS)},
    {0xEA, OpCode(0xEA, true, "NOP", 1, 2, AddressingMode::Implied, false, &CPU::op_NOP)},

    // =====================================================
    // Load/Store Instructions
    // =====================================================
    // --- LDA (Load Accumulator)
    {0xA9,
     OpCode(0xA9, true, "LDA", 2, 2, AddressingMode::Immediate, false, &CPU::op_LDA)},
    {0xA5,
     OpCode(0xA5, true, "LDA", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_LDA)},
    {0xB5,
     OpCode(0xB5, true, "LDA", 2, 4, AddressingMode::ZeroPageX, false, &CPU::op_LDA)},
    {0xAD,
     OpCode(0xAD, true, "LDA", 3, 4, AddressingMode::Absolute, false, &CPU::op_LDA)},
    {0xBD, OpCode(0xBD, true, "LDA", 3, 4, AddressingMode::AbsoluteX, false, &CPU::op_LDA)},
    {0xB9, OpCode(0xB9, true, "LDA", 3, 4, AddressingMode::AbsoluteY, false, &CPU::op_LDA)},
    {0xA1,
     OpCode(0xA1, true, "LDA", 2, 6, AddressingMode::IndirectX, false, &CPU::op_LDA)},
    {0xB1,
     OpCode(0xB1, true, "LDA", 2, 5, AddressingMode::IndirectY, false, &CPU::op_LDA)},

    // --- LDX (Load X Register)
    {0xA2,
     OpCode(0xA2, true, "LDX", 2, 2, AddressingMode::Immediate, false, &CPU::op_LDX)},
    {0xA6,
     OpCode(0xA6, true, "LDX", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_LDX)},
    {0xB6,
     OpCode(0xB6, true, "LDX", 2, 4, AddressingMode::ZeroPageY, false, &CPU::op_LDX)},
    {0xAE,
     OpCode(0xAE, true, "LDX", 3, 4, AddressingMode::Absolute, false, &CPU::op_LDX)},
    {0xBE, 
     OpCode(0xBE, true, "LDX", 3, 4, AddressingMode::AbsoluteY, false, &CPU::op_LDX)},  // +1 cycle if page crossed

    // --- LDY (Load Y Register)
    {0xA0,
     OpCode(0xA0, true, "LDY", 2, 2, AddressingMode::Immediate, false, &CPU::op_LDY)},
    {0xA4,
     OpCode(0xA4, true, "LDY", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_LDY)},
    {0xB4,
     OpCode(0xB4, true, "LDY", 2, 4, AddressingMode::ZeroPageX, false, &CPU::op_LDY)},
    {0xAC,
     OpCode(0xAC, true, "LDY", 3, 4, AddressingMode::Absolute, false, &CPU::op_LDY)},
    {0xBC, OpCode(0xBC, true, "LDY", 3, 4, AddressingMode::AbsoluteX, false, &CPU::op_LDY)},  // +1 cycle if page crossed

    // --- STA (Store Accumulator)
    {0x85,
     OpCode(0x85, true, "STA", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_STA)},
    {0x95,
     OpCode(0x95, true, "STA", 2, 4, AddressingMode::ZeroPageX, false, &CPU::op_STA)},
    {0x8D,
     OpCode(0x8D, true, "STA", 3, 4, AddressingMode::Absolute, false, &CPU::op_STA)},
    {0x9D,
     OpCode(0x9D, true, "STA", 3, 5, AddressingMode::AbsoluteX, true, &CPU::op_STA)},
    {0x99,
     OpCode(0x99, true, "STA", 3, 5, AddressingMode::AbsoluteY, true, &CPU::op_STA)},
    {0x81,
     OpCode(0x81, true, "STA", 2, 6, AddressingMode::IndirectX, false, &CPU::op_STA)},
    {0x91,
     OpCode(0x91, true, "STA", 2, 6, AddressingMode::IndirectY, true, &CPU::op_STA)},

    // --- STX (Store X Register)
    {0x86,
     OpCode(0x86, true, "STX", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_STX)},
    {0x96,
     OpCode(0x96, true, "STX", 2, 4, AddressingMode::ZeroPageY, true, &CPU::op_STX)},
    {0x8E,
     OpCode(0x8E, true, "STX", 3, 4, AddressingMode::Absolute, false, &CPU::op_STX)},

    // --- STY (Store Y Register)
    {0x84,
     OpCode(0x84, true, "STY", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_STY)},
    {0x94,
     OpCode(0x94, true, "STY", 2, 4, AddressingMode::ZeroPageX, true, &CPU::op_STY)},
    {0x8C,
     OpCode(0x8C, true, "STY", 3, 4, AddressingMode::Absolute, false, &CPU::op_STY)},

    // =====================================================
    // Arithmetic Instructions
    // =====================================================
    // --- ADC (Add with Carry)
    {0x69,
     OpCode(0x69, true, "ADC", 2, 2, AddressingMode::Immediate, false, &CPU::op_ADC)},
    {0x65,
     OpCode(0x65, true, "ADC", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_ADC)},
    {0x75,
     OpCode(0x75, true, "ADC", 2, 4, AddressingMode::ZeroPageX, false, &CPU::op_ADC)},
    {0x6D,
     OpCode(0x6D, true, "ADC", 3, 4, AddressingMode::Absolute, false, &CPU::op_ADC)},
    {0x7D, OpCode(0x7D, true, "ADC", 3, 4, AddressingMode::AbsoluteX, false, &CPU::op_ADC)},  // +1 cycle if page crossed
    {0x79, OpCode(0x79, true, "ADC", 3, 4, AddressingMode::AbsoluteY, false, &CPU::op_ADC)},  // +1 cycle if page crossed
    {0x61,
     OpCode(0x61, true, "ADC", 2, 6, AddressingMode::IndirectX, false, &CPU::op_ADC)},
    {0x71, OpCode(0x71, true, "ADC", 2, 5, AddressingMode::IndirectY, false, &CPU::op_ADC)},  // +1 cycle if page crossed

    // --- SBC (Subtract with Carry)
    {0xE9,
     OpCode(0xE9, true, "SBC", 2, 2, AddressingMode::Immediate, false, &CPU::op_SBC)},
    {0xE5,
     OpCode(0xE5, true, "SBC", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_SBC)},
    {0xF5,
     OpCode(0xF5, true, "SBC", 2, 4, AddressingMode::ZeroPageX, false, &CPU::op_SBC)},
    {0xED,
     OpCode(0xED, true, "SBC", 3, 4, AddressingMode::Absolute, false, &CPU::op_SBC)},
    {0xFD,
     OpCode(0xFD, true, "SBC", 3, 4, AddressingMode::AbsoluteX, false, &CPU::op_SBC)},
    {0xF9,
     OpCode(0xF9, true, "SBC", 3, 4, AddressingMode::AbsoluteY, false, &CPU::op_SBC)},
    {0xE1,
     OpCode(0xE1, true, "SBC", 2, 6, AddressingMode::IndirectX, false, &CPU::op_SBC)},
    {0xF1,
     OpCode(0xF1, true, "SBC", 2, 5, AddressingMode::IndirectY, false, &CPU::op_SBC)},

    // --- INC
    {0xE6,
     OpCode(0xE6, true, "INC", 2, 5, AddressingMode::ZeroPage, false, &CPU::op_INC)},
    {0xF6,
     OpCode(0xF6, true, "INC", 2, 6, AddressingMode::ZeroPageX, false, &CPU::op_INC)},
    {0xEE,
     OpCode(0xEE, true, "INC", 3, 6, AddressingMode::Absolute, false, &CPU::op_INC)},
    {0xFE,
     OpCode(0xFE, true, "INC", 3, 7, AddressingMode::AbsoluteX, true, &CPU::op_INC)},

    // --- INX
    {0xE8,
     OpCode(0xE8, true, "INX", 1, 2, AddressingMode::Implied, false, &CPU::op_INX)},

    // --- INY
    {0xC8,
     OpCode(0xC8, true, "INY", 1, 2, AddressingMode::Implied, false, &CPU::op_INY)},

    // --- DEC
    {0xC6,
     OpCode(0xC6, true, "DEC", 2, 5, AddressingMode::ZeroPage, false, &CPU::op_DEC)},
    {0xD6,
     OpCode(0xD6, true, "DEC", 2, 6, AddressingMode::ZeroPageX, false, &CPU::op_DEC)},
    {0xCE,
     OpCode(0xCE, true, "DEC", 3, 6, AddressingMode::Absolute, false, &CPU::op_DEC)},
    {0xDE,
     OpCode(0xDE, true, "DEC", 3, 7, AddressingMode::AbsoluteX, true, &CPU::op_DEC)},

    // --- DEX
    {0xCA,
     OpCode(0xCA, true, "DEX", 1, 2, AddressingMode::Implied, false, &CPU::op_DEX)},

    // --- DEY
    {0x88,
     OpCode(0x88, true, "DEY", 1, 2, AddressingMode::Implied, false, &CPU::op_DEY)},

    // =====================================================
    // Logical Instructions
    // =====================================================
    // --- AND
    {0x29,
     OpCode(0x29, true, "AND", 2, 2, AddressingMode::Immediate, false, &CPU::op_AND)},
    {0x25,
     OpCode(0x25, true, "AND", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_AND)},
    {0x35,
     OpCode(0x35, true, "AND", 2, 4, AddressingMode::ZeroPageX, false, &CPU::op_AND)},
    {0x2D,
     OpCode(0x2D, true, "AND", 3, 4, AddressingMode::Absolute, false, &CPU::op_AND)},
    {0x3D, OpCode(0x3D, true, "AND", 3, 4, AddressingMode::AbsoluteX, false, &CPU::op_AND)},  // +1 cycle if page crossed
    {0x39, OpCode(0x39, true, "AND", 3, 4, AddressingMode::AbsoluteY, false, &CPU::op_AND)},  // +1 cycle if page crossed
    {0x21,
     OpCode(0x21, true, "AND", 2, 6, AddressingMode::IndirectX, false, &CPU::op_AND)},
    {0x31, OpCode(0x31, true, "AND", 2, 5, AddressingMode::IndirectY, false, &CPU::op_AND)},  // +1 cycle if page crossed

    // --- ORA
    {0x09,
     OpCode(0x09, true, "ORA", 2, 2, AddressingMode::Immediate, false, &CPU::op_ORA)},
    {0x05,
     OpCode(0x05, true, "ORA", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_ORA)},
    {0x15,
     OpCode(0x15, true, "ORA", 2, 4, AddressingMode::ZeroPageX, false, &CPU::op_ORA)},
    {0x0D,
     OpCode(0x0D, true, "ORA", 3, 4, AddressingMode::Absolute, false, &CPU::op_ORA)},
    {0x1D,
     OpCode(0x1D, true, "ORA", 3, 4, AddressingMode::AbsoluteX, false, &CPU::op_ORA)},
    {0x19,
     OpCode(0x19, true, "ORA", 3, 4, AddressingMode::AbsoluteY, false, &CPU::op_ORA)},
    {0x01,
     OpCode(0x01, true, "ORA", 2, 6, AddressingMode::IndirectX, false, &CPU::op_ORA)},
    {0x11,
     OpCode(0x11, true, "ORA", 2, 5, AddressingMode::IndirectY, false, &CPU::op_ORA)},

    // --- EOR
    {0x49,
     OpCode(0x49, true, "EOR", 2, 2, AddressingMode::Immediate, false, &CPU::op_EOR)},
    {0x45,
     OpCode(0x45, true, "EOR", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_EOR)},
    {0x55,
     OpCode(0x55, true, "EOR", 2, 4, AddressingMode::ZeroPageX, false, &CPU::op_EOR)},
    {0x4D,
     OpCode(0x4D, true, "EOR", 3, 4, AddressingMode::Absolute, false, &CPU::op_EOR)},
    {0x5D,
     OpCode(0x5D, true, "EOR", 3, 4, AddressingMode::AbsoluteX, false, &CPU::op_EOR)},
    {0x59,
     OpCode(0x59, true, "EOR", 3, 4, AddressingMode::AbsoluteY, false, &CPU::op_EOR)},
    {0x41,
     OpCode(0x41, true, "EOR", 2, 6, AddressingMode::IndirectX, false, &CPU::op_EOR)},
    {0x51,
     OpCode(0x51, true, "EOR", 2, 5, AddressingMode::IndirectY, false, &CPU::op_EOR)},

    // --- BIT
    {0x24,
     OpCode(0x24, true, "BIT", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_BIT)},
    {0x2C,
     OpCode(0x2C, true, "BIT", 3, 4, AddressingMode::Absolute, false, &CPU::op_BIT)},

    // =====================================================
    // Shift and Rotate Instructions
    // =====================================================
    // --- ASL
    {0x0A,
     OpCode(0x0A, true, "ASL", 1, 2, AddressingMode::Acc, false, &CPU::op_ASL_ACC)},
    {0x06,
     OpCode(0x06, true, "ASL", 2, 5, AddressingMode::ZeroPage, false, &CPU::op_ASL)},
    {0x16,
     OpCode(0x16, true, "ASL", 2, 6, AddressingMode::ZeroPageX, false, &CPU::op_ASL)},
    {0x0E,
     OpCode(0x0E, true, "ASL", 3, 6, AddressingMode::Absolute, false, &CPU::op_ASL)},
    {0x1E,
     OpCode(0x1E, true, "ASL", 3, 7, AddressingMode::AbsoluteX, true, &CPU::op_ASL)},

    // --- LSR
    {0x4A,
     OpCode(0x4A, true, "LSR", 1, 2, AddressingMode::Acc, false, &CPU::op_LSR_ACC)},
    {0x46,
     OpCode(0x46, true, "LSR", 2, 5, AddressingMode::ZeroPage, false, &CPU::op_LSR)},
    {0x56,
     OpCode(0x56, true, "LSR", 2, 6, AddressingMode::ZeroPageX, false, &CPU::op_LSR)},
    {0x4E,
     OpCode(0x4E, true, "LSR", 3, 6, AddressingMode::Absolute, false, &CPU::op_LSR)},
    {0x5E,
     OpCode(0x5E, true, "LSR", 3, 7, AddressingMode::AbsoluteX, true, &CPU::op_LSR)},

    // --- ROL
    {0x2A,
     OpCode(0x2A, true, "ROL", 1, 2, AddressingMode::Acc, false, &CPU::op_ROL_ACC)},
    {0x26,
     OpCode(0x26, true, "ROL", 2, 5, AddressingMode::ZeroPage, false, &CPU::op_ROL)},
    {0x36,
     OpCode(0x36, true, "ROL", 2, 6, AddressingMode::ZeroPageX, false, &CPU::op_ROL)},
    {0x2E,
     OpCode(0x2E, true, "ROL", 3, 6, AddressingMode::Absolute, false, &CPU::op_ROL)},
    {0x3E,
     OpCode(0x3E, true, "ROL", 3, 7, AddressingMode::AbsoluteX, true, &CPU::op_ROL)},

    // --- ROR
    {0x6A,
     OpCode(0x6A, true, "ROR", 1, 2, AddressingMode::Acc, false, &CPU::op_ROR_ACC)},
    {0x66,
     OpCode(0x66, true, "ROR", 2, 5, AddressingMode::ZeroPage, false, &CPU::op_ROR)},
    {0x76,
     OpCode(0x76, true, "ROR", 2, 6, AddressingMode::ZeroPageX, false, &CPU::op_ROR)},
    {0x6E,
     OpCode(0x6E, true, "ROR", 3, 6, AddressingMode::Absolute, false, &CPU::op_ROR)},
    {0x7E,
     OpCode(0x7E, true, "ROR", 3, 7, AddressingMode::AbsoluteX, true, &CPU::op_ROR)},

    // =====================================================
    // Branch Instructions
    // =====================================================
    // 3 cycles if taken
    // -1 cycles if not taken (total 2)
    // +1 cycles if taken and crossing a page (total 4)
    {0x10,
     OpCode(0x10, true, "BPL", 2, 3, AddressingMode::Relative, false, &CPU::op_BPL)},
    {0x30,
     OpCode(0x30, true, "BMI", 2, 3, AddressingMode::Relative, false, &CPU::op_BMI)},
    {0x50,
     OpCode(0x50, true, "BVC", 2, 3, AddressingMode::Relative, false, &CPU::op_BVC)},
    {0x70,
     OpCode(0x70, true, "BVS", 2, 3, AddressingMode::Relative, false, &CPU::op_BVS)},
    {0x90,
     OpCode(0x90, true, "BCC", 2, 3, AddressingMode::Relative, false, &CPU::op_BCC)},
    {0xB0,
     OpCode(0xB0, true, "BCS", 2, 3, AddressingMode::Relative, false, &CPU::op_BCS)},
    {0xD0,
     OpCode(0xD0, true, "BNE", 2, 3, AddressingMode::Relative, false, &CPU::op_BNE)},
    {0xF0,
     OpCode(0xF0, true, "BEQ", 2, 3, AddressingMode::Relative, false, &CPU::op_BEQ)},

    // =====================================================
    // Compare Instructions
    // =====================================================
    // --- CMP
    {0xC9,
     OpCode(0xC9, true, "CMP", 2, 2, AddressingMode::Immediate, false, &CPU::op_CMP)},
    {0xC5,
     OpCode(0xC5, true, "CMP", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_CMP)},
    {0xD5,
     OpCode(0xD5, true, "CMP", 2, 4, AddressingMode::ZeroPageX, false, &CPU::op_CMP)},
    {0xCD,
     OpCode(0xCD, true, "CMP", 3, 4, AddressingMode::Absolute, false, &CPU::op_CMP)},
    {0xDD,
     OpCode(0xDD, true, "CMP", 3, 4, AddressingMode::AbsoluteX, false, &CPU::op_CMP)},
    {0xD9,
     OpCode(0xD9, true, "CMP", 3, 4, AddressingMode::AbsoluteY, false, &CPU::op_CMP)},
    {0xC1,
     OpCode(0xC1, true, "CMP", 2, 6, AddressingMode::IndirectX, false, &CPU::op_CMP)},
    {0xD1,
     OpCode(0xD1, true, "CMP", 2, 5, AddressingMode::IndirectY, false, &CPU::op_CMP)},

    // --- CPX
    {0xE0,
     OpCode(0xE0, true, "CPX", 2, 2, AddressingMode::Immediate, false, &CPU::op_CPX)},
    {0xE4,
     OpCode(0xE4, true, "CPX", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_CPX)},
    {0xEC,
     OpCode(0xEC, true, "CPX", 3, 4, AddressingMode::Absolute, false, &CPU::op_CPX)},

    // --- CPY
    {0xC0,
     OpCode(0xC0, true, "CPY", 2, 2, AddressingMode::Immediate, false, &CPU::op_CPY)},
    {0xC4,
     OpCode(0xC4, true, "CPY", 2, 3, AddressingMode::ZeroPage, false, &CPU::op_CPY)},
    {0xCC,
     OpCode(0xCC, true, "CPY", 3, 4, AddressingMode::Absolute, false, &CPU::op_CPY)},

    // =====================================================
    // Stack and Register Transfer Instructions
    // =====================================================
    // --- Stack Operations
    {0x48,
     OpCode(0x48, true, "PHA", 1, 3, AddressingMode::Implied, false, &CPU::op_PHA)},
    {0x08,
     OpCode(0x08, true, "PHP", 1, 3, AddressingMode::Implied, false, &CPU::op_PHP)},
    {0x68,
     OpCode(0x68, true, "PLA", 1, 4, AddressingMode::Implied, false, &CPU::op_PLA)},
    {0x28,
     OpCode(0x28, true, "PLP", 1, 4, AddressingMode::Implied, false, &CPU::op_PLP)},

    // --- Register Transfers
    {0xAA,
     OpCode(0xAA, true, "TAX", 1, 2, AddressingMode::Implied, false, &CPU::op_TAX)},
    {0xA8,
     OpCode(0xA8, true, "TAY", 1, 2, AddressingMode::Implied, false, &CPU::op_TAY)},
    {0xBA,
     OpCode(0xBA, true, "TSX", 1, 2, AddressingMode::Implied, false, &CPU::op_TSX)},
    {0x8A,
     OpCode(0x8A, true, "TXA", 1, 2, AddressingMode::Implied, false, &CPU::op_TXA)},
    {0x9A,
     OpCode(0x9A, true, "TXS", 1, 2, AddressingMode::Implied, false, &CPU::op_TXS)},
    {0x98,
     OpCode(0x98, true, "TYA", 1, 2, AddressingMode::Implied, false, &CPU::op_TYA)},

    // =====================================================
    // Flag Instructions
    // =====================================================
    {0x18,
     OpCode(0x18, true, "CLC", 1, 2, AddressingMode::Implied, false, &CPU::op_CLC)},
    {0x38,
     OpCode(0x38, true, "SEC", 1, 2, AddressingMode::Implied, false, &CPU::op_SEC)},
    {0x58,
     OpCode(0x58, true, "CLI", 1, 2, AddressingMode::Implied, false, &CPU::op_CLI)},
    {0x78,
     OpCode(0x78, true, "SEI", 1, 2, AddressingMode::Implied, false, &CPU::op_SEI)},
    {0xB8,
     OpCode(0xB8, true, "CLV", 1, 2, AddressingMode::Implied, false, &CPU::op_CLV)},
    {0xD8,
     OpCode(0xD8, true, "CLD", 1, 2, AddressingMode::Implied, false, &CPU::op_CLD)},
    {0xF8,
     OpCode(0xF8, true, "SED", 1, 2, AddressingMode::Implied, false, &CPU::op_SED)},

    // 105 unofficial opcodes
    // =====================================================
    // UNOFFICIAL/ILLEGAL OPCODES
    // =====================================================
    // --- SLO – (ASL then ORA) ---
    {0x07,
     OpCode(0x07, false, "SLO", 2, 5, AddressingMode::ZeroPage, false, &CPU::opi_SLO)},
    {0x17, OpCode(0x17, false, "SLO", 2, 6, AddressingMode::ZeroPageX, false, &CPU::opi_SLO)},
    {0x0F,
     OpCode(0x0F, false, "SLO", 3, 6, AddressingMode::Absolute, false, &CPU::opi_SLO)},
    {0x1F, OpCode(0x1F, false, "SLO", 3, 7, AddressingMode::AbsoluteX, true, &CPU::opi_SLO)},
    {0x1B, OpCode(0x1B, false, "SLO", 3, 7, AddressingMode::AbsoluteY, true, &CPU::opi_SLO)},
    {0x03, OpCode(0x03, false, "SLO", 2, 8, AddressingMode::IndirectX, false, &CPU::opi_SLO)},
    {0x13, OpCode(0x13, false, "SLO", 2, 8, AddressingMode::IndirectY, true, &CPU::opi_SLO)},

    // --- RLA – (ROL then AND) ---
    {0x27,
     OpCode(0x27, false, "RLA", 2, 5, AddressingMode::ZeroPage, false, &CPU::opi_RLA)},
    {0x37, OpCode(0x37, false, "RLA", 2, 6, AddressingMode::ZeroPageX, false, &CPU::opi_RLA)},
    {0x2F,
     OpCode(0x2F, false, "RLA", 3, 6, AddressingMode::Absolute, false, &CPU::opi_RLA)},
    {0x3F, OpCode(0x3F, false, "RLA", 3, 7, AddressingMode::AbsoluteX, true, &CPU::opi_RLA)},
    {0x3B, OpCode(0x3B, false, "RLA", 3, 7, AddressingMode::AbsoluteY, true, &CPU::opi_RLA)},
    {0x23, OpCode(0x23, false, "RLA", 2, 8, AddressingMode::IndirectX, false, &CPU::opi_RLA)},
    {0x33, OpCode(0x33, false, "RLA", 2, 8, AddressingMode::IndirectY, true, &CPU::opi_RLA)},

    // --- SRE – (LSR then EOR) ---
    {0x47,
     OpCode(0x47, false, "SRE", 2, 5, AddressingMode::ZeroPage, false, &CPU::opi_SRE)},
    {0x57, OpCode(0x57, false, "SRE", 2, 6, AddressingMode::ZeroPageX, false, &CPU::opi_SRE)},
    {0x4F,
     OpCode(0x4F, false, "SRE", 3, 6, AddressingMode::Absolute, false, &CPU::opi_SRE)},
    {0x5F, OpCode(0x5F, false, "SRE", 3, 7, AddressingMode::AbsoluteX, true, &CPU::opi_SRE)},
    {0x5B, OpCode(0x5B, false, "SRE", 3, 7, AddressingMode::AbsoluteY, true, &CPU::opi_SRE)},
    {0x43, OpCode(0x43, false, "SRE", 2, 8, AddressingMode::IndirectX, false, &CPU::opi_SRE)},
    {0x53, OpCode(0x53, false, "SRE", 2, 8, AddressingMode::IndirectY, true, &CPU::opi_SRE)},

    // --- RRA – (ROR then ADC) ---
    {0x67,
     OpCode(0x67, false, "RRA", 2, 5, AddressingMode::ZeroPage, false, &CPU::opi_RRA)},
    {0x77, OpCode(0x77, false, "RRA", 2, 6, AddressingMode::ZeroPageX, false, &CPU::opi_RRA)},
    {0x6F,
     OpCode(0x6F, false, "RRA", 3, 6, AddressingMode::Absolute, false, &CPU::opi_RRA)},
    {0x7F, OpCode(0x7F, false, "RRA", 3, 7, AddressingMode::AbsoluteX, true, &CPU::opi_RRA)},
    {0x7B, OpCode(0x7B, false, "RRA", 3, 7, AddressingMode::AbsoluteY, true, &CPU::opi_RRA)},
    {0x63, OpCode(0x63, false, "RRA", 2, 8, AddressingMode::IndirectX, false, &CPU::opi_RRA)},
    {0x73, OpCode(0x73, false, "RRA", 2, 8, AddressingMode::IndirectY, true, &CPU::opi_RRA)},

    // --- LAX – (LDA then LDX simultaneously) ---
    {0xA7,
     OpCode(0xA7, false, "LAX", 2, 3, AddressingMode::ZeroPage, false, &CPU::opi_LAX)},
    {0xB7, OpCode(0xB7, false, "LAX", 2, 4, AddressingMode::ZeroPageY, false, &CPU::opi_LAX)},
    {0xAF,
     OpCode(0xAF, false, "LAX", 3, 4, AddressingMode::Absolute, false, &CPU::opi_LAX)},
    {0xBF, OpCode(0xBF, false, "LAX", 3, 4, AddressingMode::AbsoluteY, false, &CPU::opi_LAX)},  // +1 cycle if page crossed
    {0xA3, OpCode(0xA3, false, "LAX", 2, 6, AddressingMode::IndirectX, false, &CPU::opi_LAX)},
    {0xB3, OpCode(0xB3, false, "LAX", 2, 5, AddressingMode::IndirectY, false, &CPU::opi_LAX)},  // +1 cycle if page crossed

    // --- DCP – (DEC then CMP) ---
    {0xC7,
     OpCode(0xC7, false, "DCP", 2, 5, AddressingMode::ZeroPage, false, &CPU::opi_DCP)},
    {0xD7, OpCode(0xD7, false, "DCP", 2, 6, AddressingMode::ZeroPageX, false, &CPU::opi_DCP)},
    {0xCF,
     OpCode(0xCF, false, "DCP", 3, 6, AddressingMode::Absolute, false, &CPU::opi_DCP)},
    {0xDF, OpCode(0xDF, false, "DCP", 3, 7, AddressingMode::AbsoluteX, true, &CPU::opi_DCP)},
    {0xDB, OpCode(0xDB, false, "DCP", 3, 7, AddressingMode::AbsoluteY, true, &CPU::opi_DCP)},
    {0xC3, OpCode(0xC3, false, "DCP", 2, 8, AddressingMode::IndirectX, false, &CPU::opi_DCP)},
    {0xD3, OpCode(0xD3, false, "DCP", 2, 8, AddressingMode::IndirectY, true, &CPU::opi_DCP)},

    // --- ISC(INS) – (INC then SBC) ---
    {0xE7,
     OpCode(0xE7, false, "ISB", 2, 5, AddressingMode::ZeroPage, false, &CPU::opi_ISC)},
    {0xF7, OpCode(0xF7, false, "ISB", 2, 6, AddressingMode::ZeroPageX, false, &CPU::opi_ISC)},
    {0xEF,
     OpCode(0xEF, false, "ISB", 3, 6, AddressingMode::Absolute, false, &CPU::opi_ISC)},
    {0xFF, OpCode(0xFF, false, "ISB", 3, 7, AddressingMode::AbsoluteX, true, &CPU::opi_ISC)},
    {0xFB, OpCode(0xFB, false, "ISB", 3, 7, AddressingMode::AbsoluteY, true, &CPU::opi_ISC)},
    {0xE3, OpCode(0xE3, false, "ISB", 2, 8, AddressingMode::IndirectX, false, &CPU::opi_ISC)},
    {0xF3, OpCode(0xF3, false, "ISB", 2, 8, AddressingMode::IndirectY, true, &CPU::opi_ISC)},

    // --- SAX – (STA and STX simultaneously) ---
    {0x87,
     OpCode(0x87, false, "SAX", 2, 3, AddressingMode::ZeroPage, false, &CPU::opi_SAX)},
    {0x97, OpCode(0x97, false, "SAX", 2, 4, AddressingMode::ZeroPageY, false, &CPU::opi_SAX)},
    {0x8F,
     OpCode(0x8F, false, "SAX", 3, 4, AddressingMode::Absolute, false, &CPU::opi_SAX)},
    {0x83, OpCode(0x83, false, "SAX", 2, 6, AddressingMode::IndirectX, false, &CPU::opi_SAX)},

    // --- ANC - (AND then update Carry and Negative) ---
    // Here we choose to treat 0x0B and 0x2B as ANC and 0x8B as XAA.
    {0x0B, OpCode(0x0B, false, "ANC", 2, 2, AddressingMode::Immediate, false, &CPU::opi_ANC)},
    {0x2B, OpCode(0x2B, false, "ANC", 2, 2, AddressingMode::Immediate, false, &CPU::opi_ANC)},
    // --- ANE(XAA) - (TXA then AND immediate)
    {0x8B, OpCode(0x8B, false, "ANE", 2, 2, AddressingMode::Immediate, false, &CPU::opi_ANE)},

    // --- ARR – (AND then ROR) ---
    {0x6B, OpCode(0x6B, false, "ARR", 2, 2, AddressingMode::Immediate, false, &CPU::opi_ARR)},

    // --- ALR – (AND then LSR) ---
    {0x4B, OpCode(0x4B, false, "ALR", 2, 2, AddressingMode::Immediate, false, &CPU::opi_ALR)},

    // --- LXA(OAL) - (Highly unstable)
    {0xAB, OpCode(0xAB, false, "LXA", 2, 2, AddressingMode::Immediate, false, &CPU::opi_LXA)},

    // --- SBX(AXS,SAX) – (A & X then subtract) ---
    {0xCB, OpCode(0xCB, false, "SBX", 2, 2, AddressingMode::Immediate, false, &CPU::opi_SBX)},

    // --- Illegal SBC variant – (undocumented SBC) ---
    {0xEB, OpCode(0xEB, false, "SBC", 2, 2, AddressingMode::Immediate, false, &CPU::opi_SBC)},

    // --- LAS (or LAR) – (load A, X, and SP from memory) ---
    {0xBB, OpCode(0xBB, false, "LAS", 3, 4, AddressingMode::AbsoluteY, false, &CPU::opi_LAS)},

    // --- Undocumented Store/Transfer opcodes ---
    // SHA(AHX,AXA) – stores (A & X) into memory under restrictions
    {0x9F, OpCode(0x9F, false, "SHA", 3, 5, AddressingMode::AbsoluteY, true, &CPU::opi_SHA)},
    {0x93, OpCode(0x93, false, "SHA", 2, 6, AddressingMode::IndirectY, true, &CPU::opi_SHA)},
    // SHX(A11,SXA,XAS) – undocumented variant related to X (Absolute,Y)
    {0x9E, OpCode(0x9E, false, "SHX", 3, 5, AddressingMode::AbsoluteY, true, &CPU::opi_SHX)},
    // SHY(SAY) – undocumented variant related to Y (Absolute,X)
    {0x9C, OpCode(0x9C, false, "SHY", 3, 5, AddressingMode::AbsoluteX, true, &CPU::opi_SHY)},

    // SHS (TAS) – stores (A & X) into memory and sets SP (Absolute,Y)
    {0x9B, OpCode(0x9B, false, "TAS", 3, 5, AddressingMode::AbsoluteY, true, &CPU::opi_TAS)},

    // --- Undocumented NOPs – these do nothing but consume cycles ---
    // Implied NOPs:
    {0x1A,
     OpCode(0x1A, false, "NOP", 1, 2, AddressingMode::Implied, false, &CPU::opi_NOP)},
    {0x3A,
     OpCode(0x3A, false, "NOP", 1, 2, AddressingMode::Implied, false, &CPU::opi_NOP)},
    {0x5A,
     OpCode(0x5A, false, "NOP", 1, 2, AddressingMode::Implied, false, &CPU::opi_NOP)},
    {0x7A,
     OpCode(0x7A, false, "NOP", 1, 2, AddressingMode::Implied, false, &CPU::opi_NOP)},
    {0xDA,
     OpCode(0xDA, false, "NOP", 1, 2, AddressingMode::Implied, false, &CPU::opi_NOP)},
    {0xFA,
     OpCode(0xFA, false, "NOP", 1, 2, AddressingMode::Implied, false, &CPU::opi_NOP)},
    // Immediate-mode NOPs:
    {0x80, OpCode(0x80, false, "NOP", 2, 2, AddressingMode::Immediate, false, &CPU::opi_NOP)},
    {0x82, OpCode(0x82, false, "NOP", 2, 2, AddressingMode::Immediate, false, &CPU::opi_NOP)},
    {0x89, OpCode(0x82, false, "NOP", 2, 2, AddressingMode::Immediate, false, &CPU::opi_NOP)},
    {0xC2, OpCode(0xC2, false, "NOP", 2, 2, AddressingMode::Immediate, false, &CPU::opi_NOP)},
    {0xE2, OpCode(0xE2, false, "NOP", 2, 2, AddressingMode::Immediate, false, &CPU::opi_NOP)},
    // Zero Page NOPs:
    {0x04,
     OpCode(0x04, false, "NOP", 2, 3, AddressingMode::ZeroPage, false, &CPU::opi_NOP)},
    {0x44,
     OpCode(0x44, false, "NOP", 2, 3, AddressingMode::ZeroPage, false, &CPU::opi_NOP)},
    {0x64,
     OpCode(0x64, false, "NOP", 2, 3, AddressingMode::ZeroPage, false, &CPU::opi_NOP)},
    // Zero Page,X NOPs:
    {0x14, OpCode(0x14, false, "NOP", 2, 4, AddressingMode::ZeroPageX, false, &CPU::opi_NOP)},
    {0x34, OpCode(0x34, false, "NOP", 2, 4, AddressingMode::ZeroPageX, false, &CPU::opi_NOP)},
    {0x54, OpCode(0x54, false, "NOP", 2, 4, AddressingMode::ZeroPageX, false, &CPU::opi_NOP)},
    {0x74, OpCode(0x74, false, "NOP", 2, 4, AddressingMode::ZeroPageX, false, &CPU::opi_NOP)},
    {0xD4, OpCode(0xD4, false, "NOP", 2, 4, AddressingMode::ZeroPageX, false, &CPU::opi_NOP)},
    {0xF4, OpCode(0xF4, false, "NOP", 2, 4, AddressingMode::ZeroPageX, false, &CPU::opi_NOP)},
    // Absolute NOP:
    {0x0C,
     OpCode(0x0C, false, "NOP", 3, 4, AddressingMode::Absolute, false, &CPU::opi_NOP)},
    // Absolute,X NOPs:
    {0x1C, OpCode(0x1C, false, "NOP", 3, 4, AddressingMode::AbsoluteX, false, &CPU::opi_NOP)},  // +1 cycle if page crossed
    {0x3C, OpCode(0x3C, false, "NOP", 3, 4, AddressingMode::AbsoluteX, false, &CPU::opi_NOP)},  // +1 cycle if page crossed
    {0x5C, OpCode(0x5C, false, "NOP", 3, 4, AddressingMode::AbsoluteX, false, &CPU::opi_NOP)},  // +1 cycle if page crossed
    {0x7C, OpCode(0x7C, false, "NOP", 3, 4, AddressingMode::AbsoluteX, false, &CPU::opi_NOP)},  // +1 cycle if page crossed
    {0xDC, OpCode(0xDC, false, "NOP", 3, 4, AddressingMode::AbsoluteX, false, &CPU::opi_NOP)},  // +1 cycle if page crossed
    {0xFC, OpCode(0xFC, false, "NOP", 3, 4, AddressingMode::AbsoluteX, false, &CPU::opi_NOP)},  // +1 cycle if page crossed

    // --- Undocumented KILs – These instructions freeze the CPU
    // Kill (KIL/JAM)
    {0x02,
     OpCode(0x02, false, "KIL", 1, 2, AddressingMode::Implied, false, &CPU::opi_KIL)},
    {0x12,
     OpCode(0x12, false, "KIL", 1, 2, AddressingMode::Implied, false, &CPU::opi_KIL)},
    {0x22,
     OpCode(0x22, false, "KIL", 1, 2, AddressingMode::Implied, false, &CPU::opi_KIL)},
    {0x32,
     OpCode(0x32, false, "KIL", 1, 2, AddressingMode::Implied, false, &CPU::opi_KIL)},
    {0x42,
     OpCode(0x42, false, "KIL", 1, 2, AddressingMode::Implied, false, &CPU::opi_KIL)},
    {0x52,
     OpCode(0x52, false, "KIL", 1, 2, AddressingMode::Implied, false, &CPU::opi_KIL)},
    {0x62,
     OpCode(0x62, false, "KIL", 1, 2, AddressingMode::Implied, false, &CPU::opi_KIL)},
    {0x72,
     OpCode(0x72, false, "KIL", 1, 2, AddressingMode::Implied, false, &CPU::opi_KIL)},
    {0x92,
     OpCode(0x92, false, "KIL", 1, 2, AddressingMode::Implied, false, &CPU::opi_KIL)},
    {0xB2,
     OpCode(0xB2, false, "KIL", 1, 2, AddressingMode::Implied, false, &CPU::opi_KIL)},
    {0xD2,
     OpCode(0xD2, false, "KIL", 1, 2, AddressingMode::Implied, false, &CPU::opi_KIL)},
    {0xF2,
     OpCode(0xF2, false, "KIL", 1, 2, AddressingMode::Implied, false, &CPU::opi_KIL)},
};