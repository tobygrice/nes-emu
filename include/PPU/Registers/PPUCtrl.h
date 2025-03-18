#ifndef PPUCTRL_H
#define PPUCTRL_H

#include <cstdint>
#include <stdexcept>

// register classes are C++ clones of the ebook definitions
// - didn't want to take any risks at this low level
// https://github.com/bugzmanov/nes_ebook/blob/master/code/ch6.4/src/ppu/registers/

class PPUCtrl {
 private:
  uint8_t bits;

 public:
  // Bit definitions (from bit 7 to bit 0)
  // 7  bit  0
  // ---- ----
  // VPHB SINN
  // |||| ||||
  // |||| ||++- Base nametable address
  // |||| ||    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
  // |||| |+--- VRAM address increment per CPU read/write of PPUDATA
  // |||| |     (0: add 1, going across; 1: add 32, going down)
  // |||| +---- Sprite pattern table address for 8x8 sprites
  // ||||       (0: $0000; 1: $1000; ignored in 8x16 mode)
  // |||+------ Background pattern table address (0: $0000; 1: $1000)
  // ||+------- Sprite size (0: 8x8 pixels; 1: 8x16 pixels)
  // |+-------- PPU master/slave select
  // |          (0: read backdrop from EXT pins; 1: output color on EXT pins)
  // +--------- Generate an NMI at the start of the vertical blanking interval
  // (0: off; 1: on)
  static constexpr uint8_t NAMETABLE1 = 0b00000001;
  static constexpr uint8_t NAMETABLE2 = 0b00000010;
  static constexpr uint8_t VRAM_ADD_INCREMENT = 0b00000100;
  static constexpr uint8_t SPRITE_PATTERN_ADDR = 0b00001000;
  static constexpr uint8_t BACKGROUND_PATTERN_ADDR = 0b00010000;
  static constexpr uint8_t SPRITE_SIZE = 0b00100000;
  static constexpr uint8_t MASTER_SLAVE_SELECT = 0b01000000;
  static constexpr uint8_t GENERATE_NMI = 0b10000000;

  // Default constructor (initializes bits to 0)
  PPUCtrl() : bits(0) {}

  // Returns the base nametable address based on the lower two bits
  uint16_t nametable_addr() const {
    switch (bits & 0b11) {
      case 0b00:
        return 0x2000;
      case 0b01:
        return 0x2400;
      case 0b10:
        return 0x2800;
      default:  // 0b11
        return 0x2C00;
    }
  }

  // Returns the VRAM address increment: 1 if the VRAM_ADD_INCREMENT flag is not
  // set, 32 otherwise.
  uint8_t vram_addr_increment() const {
    return !isSet(VRAM_ADD_INCREMENT) ? 1 : 32;
  }

  // Returns the sprite pattern table address: 0 if the flag is not set, 0x1000
  // if it is.
  uint16_t sprite_pattern_addr() const {
    return !isSet(SPRITE_PATTERN_ADDR) ? 0 : 0x1000;
  }

  // Returns the background pattern table address: 0 if the flag is not set,
  // 0x1000 if it is.
  uint16_t bknd_pattern_addr() const {
    return !isSet(BACKGROUND_PATTERN_ADDR) ? 0 : 0x1000;
  }

  // Returns the sprite size: 8 if the SPRITE_SIZE flag is not set, 16 if it is.
  uint8_t sprite_size() const { return !isSet(SPRITE_SIZE) ? 8 : 16; }

  // Returns the PPU master/slave select.
  uint8_t master_slave_select() const { return !isSet(MASTER_SLAVE_SELECT) ? 0 : 1; }

  // Returns true if an NMI should be generated at the start of vertical blank.
  bool generate_vblank_nmi() const { return isSet(GENERATE_NMI); }

  // Updates the register's value.
  void update(uint8_t data) { bits = data; }

  // Optional helper: checks if a specific flag is set.
  bool isSet(uint8_t flag) const { return (bits & flag) != 0; }
};

#endif  // PPUCTRL_H