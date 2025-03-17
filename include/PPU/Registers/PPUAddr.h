#ifndef PPUADDR_H
#define PPUADDR_H

#include <cstdint>

class PPUAddr {
 private:
  uint8_t high;  // high byte of the address
  uint8_t low;   // low byte of the address
  bool hi_ptr;   // true if next update should write to the high byte

  void set(uint16_t data);  // sets addr register, applies mirroring

 public:
  PPUAddr();

  uint16_t get() const;         // return combined high/low bytes
  void update(uint8_t data);    // updates high or low byte per hi_ptr flag
  void increment(uint8_t inc);  // adds `inc` to address, applies mirroring
  void reset_latch();            // next write is to high byte
};

#endif  // PPUADDR_H