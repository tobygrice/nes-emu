#ifndef ADDRESSRESOLVEINFO_H
#define ADDRESSRESOLVEINFO_H

#include <cstdint>

struct AddressResolveInfo {
  uint16_t address;         // e.g. 0x0400
  uint16_t pointerAddress;  // e.g. 0x00FF (for (zp,X) or (zp),Y)
  bool pointerUsed;  // true if this addressing mode used an indirect pointer
};

#endif