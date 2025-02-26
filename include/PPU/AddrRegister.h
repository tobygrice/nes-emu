#ifndef ADDRREGISTER_H
#define ADDRREGISTER_H

#include <cstdint>

class AddrRegister {
 private:
  void set(uint16_t data); // sets addr register applies mirroring

  uint8_t high;  // high byte of the address
  uint8_t low;   // low byte of the address
  bool hi_ptr;   // true if next update should write to the high byte

 public:
  AddrRegister();

  // Updates the register with the given data byte.
  // If hi_ptr is true, the high byte is updated; otherwise, the low byte is.
  void update(uint8_t data);

  // Increments the address register by 'inc', handling wrapping of the low
  // byte.
  void increment(uint8_t inc);

  // Resets the write latch so that the next write updates the high byte.
  void resetLatch();

  // Returns the combined 16-bit address (high byte first, then low byte).
  uint16_t get() const;
};

#endif  // ADDRREGISTER_H