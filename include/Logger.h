#ifndef LOGGER_H
#define LOGGER_H

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "AddressResolveInfo.h"
#include "OpCode.h"

class Logger {
 private:
  bool silenced;
 public:
  /**
   * Log a single CPU instruction/state line. Written with a LLM.
   *
   * Parameters:
   *   pc          - Program Counter (16-bit)
   *   op          - OpCode* containing information about the operation
   *   opBytes     - Up to 3 opcode bytes for this instruction
   *   addrInfo    - Struct containing info required to build disassembly
   *   valueAtAddr - Required for disassembly
   *   A, X, Y     - CPU registers
   *   P           - Processor status (flags)
   *   SP          - Stack pointer
   *   ppuX, ppuY  - Current PPU X/Y scanline position
   *   cycles      - Cycle count
   *
   */

  void mute() { silenced = true; };
  void unmute() { silenced = false; };

  std::string disassembleInstr(uint16_t pc, const OpCode* op,
                               std::vector<uint8_t>* opBytes,
                               AddressResolveInfo& addrInfo,
                               uint8_t valueAtAddr);

  void log(uint16_t pc, const OpCode* op, std::vector<uint8_t>* opBytes,
           AddressResolveInfo* addrInfo, uint8_t valueAtAddr, uint8_t A,
           uint8_t X, uint8_t Y, uint8_t P, uint8_t SP, int ppuX, int ppuY,
           uint64_t cycles);
};

#endif