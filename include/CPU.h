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

    uint64_t cycleCount = 0; // global cycle counter
    bool pcModified = false; // flag set by handler if it has modified the pc

  public:
    CPU();

    uint8_t getA() { return a_register; };
    uint8_t getX() { return x_register; };
    uint8_t getY() { return y_register; };
    uint8_t getStatus() { return status; };
    uint16_t getPC() { return pc; };
    uint8_t getSP() { return sp; };
    uint8_t getCycleCount() { return cycleCount; };
    void setA(uint8_t value) { a_register = value; };
    void setX(uint8_t value) { x_register = value; };
    void setY(uint8_t value) { y_register = value; };
    void setStatus(uint8_t value) { status = value; };
    void setPC(uint16_t value) { pc = value; };
    void setSP(uint8_t value) { sp = value; };

    static constexpr uint8_t FLAG_CARRY     = 0b00000001; // C
    static constexpr uint8_t FLAG_ZERO      = 0b00000010; // Z
    static constexpr uint8_t FLAG_INTERRUPT = 0b00000100; // I
    static constexpr uint8_t FLAG_DECIMAL   = 0b00001000; // D
    static constexpr uint8_t FLAG_BREAK     = 0b00010000; // B
                                                          // 1 (always 1)
    static constexpr uint8_t FLAG_OVERLOW   = 0b01000000; // V
    static constexpr uint8_t FLAG_NEGATIVE  = 0b10000000; // N

    // helpers
    void updateZeroAndNegativeFlags(uint8_t result);
    void branch(uint16_t addr);

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
    uint16_t getOperandAddress(AddressingMode mode);

    // instruction implementations
    void op_ADC(uint16_t addr);
    void op_AND(uint16_t addr);
    void op_ASL(uint16_t addr);
    void op_ASL_ACC(uint16_t /* implied */);
    void op_BCC(uint16_t addr);
    void op_BCS(uint16_t addr);
    void op_BEQ(uint16_t addr);
    void op_BIT(uint16_t addr);
    void op_BMI(uint16_t addr);
    void op_BNE(uint16_t addr);
    void op_BPL(uint16_t addr);
    void op_BRK(uint16_t /* none addressing */);
    void op_BVC(uint16_t addr);
    void op_BVS(uint16_t addr);
    void op_CLC(uint16_t addr);
    void op_CLD(uint16_t addr);
    void op_CLI(uint16_t addr);
    void op_CLV(uint16_t addr);
    void op_CMP(uint16_t addr);
    void op_CPX(uint16_t addr);
    void op_CPY(uint16_t addr);
    void op_DEC(uint16_t addr);
    void op_DEX(uint16_t /* implied */);
    void op_DEY(uint16_t /* implied */);
    void op_EOR(uint16_t addr);
    void op_INC(uint16_t addr);
    void op_INX(uint16_t addr);
    void op_INY(uint16_t addr);
    void op_JMP(uint16_t addr);
    void op_JSR(uint16_t addr);
    void op_LDA(uint16_t addr);
    void op_LDX(uint16_t addr);
    void op_LDY(uint16_t addr);
    void op_LSR(uint16_t addr);
    void op_NOP(uint16_t addr);
    void op_ORA(uint16_t addr);
    void op_PHA(uint16_t addr);
    void op_PHP(uint16_t addr);
    void op_PLA(uint16_t addr);
    void op_PLP(uint16_t addr);
    void op_ROL(uint16_t addr);
    void op_ROR(uint16_t addr);
    void op_RTI(uint16_t addr);
    void op_RTS(uint16_t addr);
    void op_SBC(uint16_t addr);
    void op_SEC(uint16_t addr);
    void op_SED(uint16_t addr);
    void op_SEI(uint16_t addr);
    void op_STA(uint16_t addr);
    void op_STX(uint16_t addr);
    void op_STY(uint16_t addr);
    void op_TAX(uint16_t addr);
    void op_TAY(uint16_t addr);
    void op_TSX(uint16_t addr);
    void op_TXA(uint16_t addr);
    void op_TXS(uint16_t addr);
    void op_TYA(uint16_t addr);

};

#endif // CPU_H