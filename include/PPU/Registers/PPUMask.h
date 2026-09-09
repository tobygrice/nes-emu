#ifndef PPUMASK_H
#define PPUMASK_H

#include <cstdint>

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

    // default constructor initializing the bits to 0
    PPUMask() : bits(0) {}

    // returns true if the greyscale flag is set
    bool is_grayscale() const { return isSet(GREYSCALE); }

    // returns true if the leftmost 8 pixels of the background should be shown
    bool leftmost_8pxl_background() const {
        return isSet(LEFTMOST_8PXL_BACKGROUND);
    }

    // returns true if the leftmost 8 pixels of sprites should be shown
    bool leftmost_8pxl_sprite() const { return isSet(LEFTMOST_8PXL_SPRITE); }

    // returns true if the background should be shown
    bool show_background() const { return isSet(SHOW_BACKGROUND); }

    // returns true if the sprites should be shown
    bool show_sprites() const { return isSet(SHOW_SPRITES); }

    // updates the register bits with new data
    void update(uint8_t data) { bits = data; }

    // optional helper: checks if a specific flag is set
    bool isSet(uint8_t flag) const { return (bits & flag) != 0; }
};

#endif // PPUMASK_H
