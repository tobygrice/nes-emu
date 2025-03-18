#ifndef FRAME_H
#define FRAME_H

#include <cassert>
#include <tuple>
#include <vector>

#include "../Constants.h"
#include "Palette.h"

class Frame {
 public:
  std::vector<uint8_t> data;

  Frame() : data(SCREEN_WIDTH * SCREEN_HEIGHT * 3, 0) {}

  void set_pixel(int x, int y, const Color& rgb) {
    int base = y * SCREEN_WIDTH * 3 + x * 3;
    if (base + 2 < data.size()) {
      data[base + 0] = std::get<0>(rgb);
      data[base + 1] = std::get<1>(rgb);
      data[base + 2] = std::get<2>(rgb);
    }
  }
};

#endif  // FRAME_H
