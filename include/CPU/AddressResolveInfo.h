#ifndef ADDRESSRESOLVEINFO_H
#define ADDRESSRESOLVEINFO_H

#include <cstdint>
#include "OpCode.h" // AddressingMode class

enum ResolutionState {
  Init,  // will read operand 1 in this phase if necessary
  ReadOperand,
  ReadIndirect_Low,
  ReadIndirect_High,
  ComputeAddress,
  Done,
  Branch1,
  Branch2
};

struct AddressResolveInfo {
  AddressingMode mode;
  ResolutionState state;
  uint16_t address;         // e.g. 0x0400
  uint16_t pointerAddress;  // e.g. 0x00FF (for (zp,X) or (zp),Y)
  bool pointerUsed;  // true if this addressing mode used an indirect pointer
  bool waitPageCrossed;  // true if page was crossed (+1 cycle)

  AddressResolveInfo()
      : mode(),
        state(ResolutionState::Init),
        address(0),
        pointerAddress(0),
        pointerUsed(false),
        waitPageCrossed(false) {}

  void reset(AddressingMode m) {
    mode = m;
    state = ResolutionState::Init;
    address = 0;
    pointerAddress = 0;
    pointerUsed = false;
    waitPageCrossed = false;
  }
};

#endif