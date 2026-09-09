#ifndef PPUADDR_H
#define PPUADDR_H

#include <cstdint>

class PPUAddr {
  private:
    // PPUCTRL, PPUSCROLL and PPUADDR share these rendering registers.
    uint16_t v = 0; // current VRAM address / rendering position (15 bits)
    uint16_t t = 0; // temporary address / scroll position (15 bits)
    uint8_t x = 0;  // fine horizontal scroll
    bool w = false; // second write to either PPUSCROLL or PPUADDR

  public:
    uint16_t get() const { return v; }
    uint16_t temporary() const { return t; }
    uint8_t fine_x() const { return x; }
    void write_ctrl(uint8_t data);
    void write_scroll(uint8_t data);
    void update(uint8_t data);
    void increment(uint8_t inc);
    void increment_x();
    void increment_y();
    void copy_x();
    void copy_y();
    void reset_latch() { w = false; }
};

#endif // PPUADDR_H
