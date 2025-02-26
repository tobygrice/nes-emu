#include "../../include/PPU/AddrRegister.h"

AddrRegister::AddrRegister() : high(0), low(0), hi_ptr(true) {}

// combine high/low bytes into standard uint16_t object
uint16_t AddrRegister::get() const {
  return (static_cast<uint16_t>(high) << 8) | low;
}

// sets high/low bytes from a 16bit address
void AddrRegister::set(uint16_t data) {
  high = static_cast<uint8_t>(data >> 8);
  low = static_cast<uint8_t>(data & 0x00FF);
}

void AddrRegister::update(uint8_t data) {
  if (hi_ptr) {
    high = data;
  } else {
    low = data;
  }

  // toggle hi_ptr indicator
  hi_ptr = !hi_ptr;

  // mirror if address exceeds 0x3FFF
  if (get() > 0x3FFF) {
    set(get() & 0x3FFF);
  }
}

void AddrRegister::increment(uint8_t inc) {
  uint8_t oldLow = low;
  // Wrapping addition is inherent with unsigned arithmetic.
  low = static_cast<uint8_t>(low + inc);
  if (oldLow > low) {  // If low byte wrapped around, increment the high byte.
    high = static_cast<uint8_t>(high + 1);
  }

  if (get() > 0x3fff) {
    set(get() & 0x3fff);
  }
}

void AddrRegister::resetLatch() { hi_ptr = true; }
