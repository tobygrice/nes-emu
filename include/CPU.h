#ifndef CPU_H
#define CPU_H

#include "OpCode.h"

#include <array>
#include <cstdint>
#include <vector>

class CPU {
  private:
    // registers http://www.6502.org/users/obelisk/6502/registers.html
    uint8_t a_register;                   // accumulator
    uint8_t x_register;                   // index X
    uint8_t y_register;                   // index Y
    uint8_t status;                       // processor status
    uint16_t pc;                          // program counter
    uint8_t sp;                           // stack pointer
    std::array<uint8_t, 0x10000> memory;  // system memory (2^16 addresses)

  public:
    CPU();

    uint8_t getA() { return a_register; };
    uint8_t getX() { return x_register; };
    uint8_t getY() { return y_register; };
    uint8_t getStatus() { return status; };
    uint16_t getPC() { return pc; };
    uint8_t getSP() { return sp; };
    void setA(uint8_t value) { a_register = value; };
    void setX(uint8_t value) { x_register = value; };
    void setY(uint8_t value) { y_register = value; };
    void setStatus(uint8_t value) { status = value; };
    void setPC(uint16_t value) { pc = value; };
    void setSP(uint8_t value) { sp = value; };

    void updateZeroAndNegativeFlags(uint8_t result);

    // memory handling
    uint8_t memRead8(uint16_t addr) const;
    uint16_t memRead16(uint16_t addr) const;
    void memWrite8(uint16_t addr, uint8_t data);
    void memWrite16(uint16_t addr, uint16_t data);

    // program loading and execution
    void resetInterrupt();
    void loadAndExecute(const std::vector<uint8_t>& program);
    void loadProgram(const std::vector<uint8_t>& program);
    void executeProgram();

    // addressing mode handling
    uint16_t getOperandAddress(AddressingMode mode) const;

    // instruction implementations
    void op_ADC(AddressingMode mode);
    void op_AND(AddressingMode mode);
    void op_ASL(AddressingMode mode);
    void op_BCC(AddressingMode mode);
    void op_BCS(AddressingMode mode);
    void op_BEQ(AddressingMode mode);
    void op_BIT(AddressingMode mode);
    void op_BMI(AddressingMode mode);
    void op_BNE(AddressingMode mode);
    void op_BPL(AddressingMode mode);
    void op_BRK(AddressingMode mode);
    void op_BVC(AddressingMode mode);
    void op_BVS(AddressingMode mode);
    void op_CLC(AddressingMode mode);
    void op_CLD(AddressingMode mode);
    void op_CLI(AddressingMode mode);
    void op_CLV(AddressingMode mode);
    void op_CMP(AddressingMode mode);
    void op_CPX(AddressingMode mode);
    void op_CPY(AddressingMode mode);
    void op_DEC(AddressingMode mode);
    void op_DEX(AddressingMode mode);
    void op_DEY(AddressingMode mode);
    void op_EOR(AddressingMode mode);
    void op_INC(AddressingMode mode);
    void op_INX(AddressingMode mode);
    void op_INY(AddressingMode mode);
    void op_JMP(AddressingMode mode);
    void op_JSR(AddressingMode mode);
    void op_LDA(AddressingMode mode);
    void op_LDX(AddressingMode mode);
    void op_LDY(AddressingMode mode);
    void op_LSR(AddressingMode mode);
    void op_NOP(AddressingMode mode);
    void op_ORA(AddressingMode mode);
    void op_PHA(AddressingMode mode);
    void op_PHP(AddressingMode mode);
    void op_PLA(AddressingMode mode);
    void op_PLP(AddressingMode mode);
    void op_ROL(AddressingMode mode);
    void op_ROR(AddressingMode mode);
    void op_RTI(AddressingMode mode);
    void op_RTS(AddressingMode mode);
    void op_SBC(AddressingMode mode);
    void op_SEC(AddressingMode mode);
    void op_SED(AddressingMode mode);
    void op_SEI(AddressingMode mode);
    void op_STA(AddressingMode mode);
    void op_STX(AddressingMode mode);
    void op_STY(AddressingMode mode);
    void op_TAX(AddressingMode mode);
    void op_TAY(AddressingMode mode);
    void op_TSX(AddressingMode mode);
    void op_TXA(AddressingMode mode);
    void op_TXS(AddressingMode mode);
    void op_TYA(AddressingMode mode);

};

#endif // CPU_H