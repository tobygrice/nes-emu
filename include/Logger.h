#ifndef LOGGER_H
#define LOGGER_H

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class Logger {
 public:
 /**
  * Log a single CPU instruction/state line.
  *
  * Parameters:
  *   pc        - Program Counter (16-bit)
  *   opcodes   - Up to 3 opcode bytes for this instruction
  *   disasm    - Disassembly string, e.g. "LDA #$5C" or "STA $0303 = 00"
  *   A, X, Y   - CPU registers
  *   P         - Processor status (flags)
  *   SP        - Stack pointer
  *   ppuX, ppuY- Current PPU X/Y scanline position
  *   cycles    - Cycle count
  * 
  */
  void log(uint16_t pc, const std::vector<uint8_t>& opcodes,
           const std::string& disasm, uint8_t A, uint8_t X, uint8_t Y,
           uint8_t P, uint8_t SP, int ppuX, int ppuY, uint64_t cycles) {
    std::ostringstream oss;

    // We want uppercase hex with zero padding for PC and opcodes.
    oss << std::uppercase << std::setfill('0');

    // Print the 4-digit PC (hex).
    oss << std::hex << std::setw(4) << pc << "  ";

    // Print up to 3 opcode bytes (2-digit hex each). If fewer than 3,
    // pad with spaces so columns line up.
    for (size_t i = 0; i < 3; i++) {
      if (i < opcodes.size()) {
        oss << std::setw(2) << static_cast<int>(opcodes[i]) << " ";
      } else {
        oss << "   ";
      }
    }

    // Leave some space, then print the disassembly in a left-justified field
    // so the registers line up in the same columns every time.
    oss << "  " << std::left << std::setw(28) << disasm;

    // Switch to hex for registers (A, X, Y, P, SP).
    // Each register is 2 hex digits.
    oss << "  A:" << std::setw(2) << static_cast<int>(A)
        << " X:" << std::setw(2) << static_cast<int>(X) << " Y:" << std::setw(2)
        << static_cast<int>(Y) << " P:" << std::setw(2) << static_cast<int>(P)
        << " SP:" << std::setw(2) << static_cast<int>(SP);

    // Switch to decimal for PPU coordinates and cycle count.
    oss << std::dec << std::setfill(' ');

    // PPU: X,Y
    oss << " PPU: " << std::setw(2) << ppuX << "," << std::setw(3) << ppuY;

    // CPU cycle count
    oss << " CYC:" << cycles;

    // Print the final line to stdout.
    std::cout << oss.str() << std::endl;
  }
};

#endif