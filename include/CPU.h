#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <vector>

class CPU {
  public:
    // registers http://www.6502.org/users/obelisk/6502/registers.html
    uint8_t a_register; // accumulator
    uint8_t x_register; // index X
    uint8_t y_register; // index Y
    uint8_t status;     // processor status
    uint16_t pc;        // program counter
    uint8_t sp;         // stack pointer

    /**
     * Constructor to initialise registers with starting values.
     */
    CPU();

    /** 
     * Updates the zero and negative flags based on a given result.
     * 
     * @param result zero flag is set if result is 0, negative flag is set if 
     * MSB of result is 1.
     */ 
    void updateZeroAndNegativeFlags(uint8_t result);

    /**
     * Core method to execute a set of program instructions
     */
    void executeProgram(const std::vector<uint8_t>& instructions);

};

#endif // CPU_H