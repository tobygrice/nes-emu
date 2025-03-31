#ifndef FRAME_H
#define FRAME_H

#include <cassert>
#include <tuple>
#include <vector>

#include "../Constants.h"

using Colour = std::tuple<uint8_t, uint8_t, uint8_t>;

class Frame {
 public:
  std::vector<Colour> data;
  int currentPixel;

  Frame() : data(SCREEN_WIDTH * SCREEN_HEIGHT, {0, 0, 0}), currentPixel(0) {}

  // PPU determines colour and sets pixel
  void set_pixel(const Colour& colour) {
    // int index = (y * SCREEN_WIDTH) + x;
    // TO-DO: remove out-of-range check once stable for performance
    if (currentPixel >= data.size()) {
      throw std::runtime_error("Attempt to set Frame pixel out of range");
    }
    data.at(currentPixel) = colour;
    currentPixel++;
  }
};

#endif  // FRAME_H
