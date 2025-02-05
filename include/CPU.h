#ifndef CPU_H
#define CPU_H

#include <array>
#include <cstdint>
#include <vector>

class CPU {
  public:
    // registers http://www.6502.org/users/obelisk/6502/registers.html
    uint8_t a_register;                   // accumulator
    uint8_t x_register;                   // index X
    uint8_t y_register;                   // index Y
    uint8_t status;                       // processor status
    uint16_t pc;                          // program counter
    uint8_t sp;                           // stack pointer
    std::array<uint8_t, 0x10000> memory;  // system memory (2^16 addresses)

    CPU();

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
    void brk();
    void lda(AddressingMode mode);

};

#endif // CPU_H