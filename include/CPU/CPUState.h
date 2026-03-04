#ifndef CPU_STATE_H
#define CPU_STATE_H

#include <cstdint>
#include <vector>

#include "AddressResolveInfo.h"
#include "OpCode.h"

struct CPUState {
  uint16_t pc;
  const OpCode& op;
  const std::vector<uint8_t>& opBytes;  // updated as instruction executes
  const AddressResolveInfo& addrInfo;   // updated as instruction executes
  const uint8_t& valueAtAddr;
  uint8_t A;
  uint8_t X;
  uint8_t Y;
  uint8_t P;
  uint8_t SP;
  uint16_t ppuX;
  uint16_t ppuY;
  uint64_t cycles;

  CPUState(uint16_t pc, const OpCode& op, const std::vector<uint8_t>& opBytes,
           const AddressResolveInfo& addrInfo, const uint8_t& valueAtAddr,
           uint8_t A,
           uint8_t X, uint8_t Y, uint8_t P, uint8_t SP, int ppuX, int ppuY,
           uint64_t cycles)
      : pc(pc),
        op(op),
        opBytes(opBytes),
        addrInfo(addrInfo),
        valueAtAddr(valueAtAddr),
        A(A),
        X(X),
        Y(Y),
        P(P),
        SP(SP),
        ppuX(ppuX),
        ppuY(ppuY),
        cycles(cycles) {}
};

#endif  // CPU_STATE_H
