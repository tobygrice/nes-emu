#ifndef ADDRESSRESOLVEINFO_H
#define ADDRESSRESOLVEINFO_H

#include <cstdint>

struct AddressResolveInfo {
  uint16_t address;         // e.g. 0x0400
  uint16_t pointerAddress;  // e.g. 0x00FF (for (zp,X) or (zp),Y)
  bool pointerUsed;  // true if this addressing mode used an indirect pointer
  bool pageCrossed;  // true if page was crossed (+1 cycle)

  AddressResolveInfo()
      : address(0), pointerAddress(0), pointerUsed(false), pageCrossed(false) {}
};

#endif