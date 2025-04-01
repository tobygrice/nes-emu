#ifndef PPUMASK_H
#define PPUMASK_H

#include <cstdint>
#include <vector>

class PPUMask {
 private:
  uint8_t bits;

 public:
  static constexpr uint8_t GREYSCALE = 0b00000001;
  static constexpr uint8_t LEFTMOST_8PXL_BACKGROUND = 0b00000010;
  static constexpr uint8_t LEFTMOST_8PXL_SPRITE = 0b00000100;
  static constexpr uint8_t SHOW_BACKGROUND = 0b00001000;
  static constexpr uint8_t SHOW_SPRITES = 0b00010000;
  static constexpr uint8_t EMPHASISE_RED = 0b00100000;
  static constexpr uint8_t EMPHASISE_GREEN = 0b01000000;
  static constexpr uint8_t EMPHASISE_BLUE = 0b10000000;

  // Default constructor initializing the bits to 0.
  PPUMask() : bits(0) {}

  // Returns true if the greyscale flag is set.
  bool is_grayscale() const { return isSet(GREYSCALE); }

  // Returns true if the leftmost 8 pixels of the background should be shown.
  bool leftmost_8pxl_background() const {
    return isSet(LEFTMOST_8PXL_BACKGROUND);
  }

  // Returns true if the leftmost 8 pixels of sprites should be shown.
  bool leftmost_8pxl_sprite() const {
    return isSet(LEFTMOST_8PXL_SPRITE);
  }

  // Returns true if the background should be shown.
  bool show_background() const { return isSet(SHOW_BACKGROUND); }

  // Returns true if the sprites should be shown.
  bool show_sprites() const { return isSet(SHOW_SPRITES); }

  // Returns a vector of Colour values corresponding to the emphasised colors.
  std::vector<Colour> emphasise() const {
    std::vector<Colour> result;
    if (isSet(EMPHASISE_RED)) {
      result.push_back({1,0,0});
    }
    if (isSet(EMPHASISE_GREEN)) {
      result.push_back({0,1,0});
    }
    if (isSet(EMPHASISE_BLUE)) {
      result.push_back({0,0,1});
    }
    return result;
  }

  // Updates the register bits with new data.
  void update(uint8_t data) { bits = data; }

  // Optional helper: checks if a specific flag is set.
  bool isSet(uint8_t flag) const { return (bits & flag) != 0; }
};

#endif  // PPUMASK_H