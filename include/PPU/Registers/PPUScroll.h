#ifndef PPUSCOLL_H
#define PPUSCOLL_H

#include <cstdint>

class PPUScroll {
 public:
  uint8_t scroll_x;
  uint8_t scroll_y;
  bool latch;

  PPUScroll() : scroll_x(0), scroll_y(0), latch(false) {}

  void write(uint8_t data) {
    if (!latch) {
      scroll_x = data;
    } else {
      scroll_y = data;
    }
    latch = !latch;
  }

  // Resets the latch to false.
  void reset_latch() { latch = false; }
};

#endif  // PPUSCOLL_H