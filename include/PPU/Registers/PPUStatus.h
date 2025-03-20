#ifndef PPUSTATUS_H
#define PPUSTATUS_H

#include <cstdint>
#include <stdexcept>

class PPUStatus {
 private:
  uint8_t bits;

  // helper: sets or clears the specified bit based on status.
  inline void set(uint8_t mask, bool status) {
    if (status) {
      bits |= mask;
    } else {
      bits &= ~mask;
    }
  }

 public:
  // Bit definitions corresponding to the Rust bitflags.
  static constexpr uint8_t SPRITE_OVERFLOW = 0b00100000;
  static constexpr uint8_t SPRITE_ZERO_HIT = 0b01000000;
  static constexpr uint8_t VBLANK_STARTED =  0b10000000;

  PPUStatus() : bits(0) {}

  // Sets or clears the VBLANK_STARTED flag.
  void set_vblank_status(bool status) { set(VBLANK_STARTED, status); }

  // Sets or clears the SPRITE_ZERO_HIT flag.
  void set_sprite_zero_hit(bool status) { set(SPRITE_ZERO_HIT, status); }

  // Sets or clears the SPRITE_OVERFLOW flag.
  void set_sprite_overflow(bool status) { set(SPRITE_OVERFLOW, status); }

  // Returns true if VBLANK_STARTED flag is set.
  bool is_in_vblank() const { return (bits & VBLANK_STARTED) != 0; }

  // Returns the underlying bits.
  uint8_t snapshot() const { return bits; }
};

#endif // PPUSTATUS_H