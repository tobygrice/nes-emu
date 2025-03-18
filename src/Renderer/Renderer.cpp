#include "../../include/Renderer/Renderer.h"

#include <stdexcept>

void Renderer::render(const PPU& ppu, Frame& frame) {
  // ============== 1. RENDER BACKGROUND ==============
  {
    // get background pattern bank address (0x1000 if flag set, else 0)
    uint16_t bank = ppu.ctrl.bknd_pattern_addr();

    // nametable size = 32x30 = 960 bytes
    for (int i = 0; i < 960; i++) {
      // read tile address from VRAM (first nametable starts at index 0 in vram)
      uint16_t tile_address = static_cast<uint16_t>(ppu.vram[i]);
      int tile_column = i % 32;  // determine x,y coordinates of tile in frame
      int tile_row = i / 32;
      // get ptr to tile byte i in chr_rom
      const uint8_t* tile =
          ppu.cart->get_chr_rom_addr() + bank + tile_address * 16;
      auto palette = bg_palette(ppu, tile_column, tile_row);

      // each tile is 8x8 pixels, each pixel is 2 bits
      // 1st 8 bytes: LSB of each pixel
      // 2nd 8 bytes: MSB of each pixel
      // these two bits encode the pixel's colour
      for (int y = 0; y < 8; y++) {
        // outer loop y -> vertical index (row)
        uint8_t upper = tile[y];      // LSB of pixel
        uint8_t lower = tile[y + 8];  // MSB of pixel

        // process each bit of row in reverse order
        for (int x = 7; x >= 0; x--) {
          // inner loop x -> horizontal index (column)
          // value is 2 bits: (MSB pixel)(LSB pixel)
          uint8_t value = ((lower & 1) << 1) | (upper & 1);
          upper >>= 1;  // shift right for next loop iteration
          lower >>= 1;

          // use pixel value to determine rgb colour
          Color rgb;
          if (value == 0b00) {
            // 0 -> default value
            rgb = SYSTEM_PALETTE[ppu.palette_table[0]];
          } else {
            rgb = SYSTEM_PALETTE[palette[value]];
          }

          // set pixel in frame
          frame.set_pixel(tile_column * 8 + x, tile_row * 8 + y, rgb);
        }
      }
    }
  }

  // =============== 2. RENDER SPRITES ================
  {
    // NES OAM (Object Attribute Memory) Sprite Structure
    // Each sprite uses 4 bytes in OAM:
    // Byte | Name        | Bits | Description
    // -----|-------------|------|---------------------------------
    //  0   | Y Position  |  8   | Y-coordinate (+1 offset)
    //  1   | Tile Index  |  8   | Tile number in CHR ROM
    //  2   | Attributes  |  8   | Palette, flipping, priority
    //  3   | X Position  |  8   | X-coordinate
    //
    // NES supports 64 sprites total (256 bytes of OAM).

    // lower the index -> higher the priority:
    // process in reverse order so higher-priority sprites are drawn on top:
    for (int i = static_cast<int>(ppu.oam_data.size()) - 4; i >= 0; i -= 4) {
      int tile_row = ppu.oam_data[i];
      uint16_t tile_address = static_cast<uint16_t>(ppu.oam_data[i + 1]);
      uint8_t attributes = ppu.oam_data[i + 2];
      int tile_column = ppu.oam_data[i + 3];

      // Sprite Attribute Byte (OAM Byte 2)
      //  Bit | Function               | Description
      // -----|------------------------|-----------------------------
      //  7   | Flip Vertically (V)    | 1 = Flip sprite vertically
      //  6   | Flip Horizontally (H)  | 1 = Flip sprite horizontally
      //  5   | Priority (P)           | 0 = In front, 1 = Behind background
      //  4-2 | Unused                 | Always 0
      //  1-0 | Palette Index          | 0b00 to 0b11 (Selects 1 of 4 palettes)
      bool flip_vertical = (attributes & 0b10000000) != 0;
      bool flip_horizontal = (attributes & 0b01000000) != 0;
      bool priority = (attributes & 0b00100000) != 0;
      uint8_t palette_index = attributes & 0b11;

      auto spr_palette = sprite_palette(ppu, palette_index);
      // get sprite pattern bank address (0x1000 if flag set, else 0)
      uint16_t spr_bank = ppu.ctrl.sprite_pattern_addr();
      const uint8_t* tile =
          ppu.cart->get_chr_rom_addr() + spr_bank + tile_address * 16;

      // tiles are stored same as bg tiles, two bits for 1 pixel
      for (int y = 0; y < 8; y++) {
        // outer loop y -> vertical index (row)
        uint8_t upper = tile[y];      // LSB of pixel
        uint8_t lower = tile[y + 8];  // MSB of pixel

        // process each bit of row in reverse order
        for (int x = 7; x >= 0; x--) {
          // inner loop x -> horizontal index (column)
          // value is 2 bits: (MSB pixel)(LSB pixel)
          uint8_t value = ((lower & 1) << 1) | (upper & 1);
          upper >>= 1;  // shift right for next loop iteration
          lower >>= 1;

          // determine colour:
          if (value == 0) continue;  // skip transparent pixels
          Color rgb = SYSTEM_PALETTE[spr_palette[value]];

          // compute final sprite pixel position after flipping
          int x_pos, y_pos;
          if (!flip_horizontal && !flip_vertical) {
            x_pos = tile_column + x;
            y_pos = tile_row + y;
          } else if (flip_horizontal && !flip_vertical) {
            x_pos = tile_column + (7 - x);
            y_pos = tile_row + y;
          } else if (!flip_horizontal && flip_vertical) {
            x_pos = tile_column + x;
            y_pos = tile_row + (7 - y);
          } else {  // flip_horizontal && flip_vertical
            x_pos = tile_column + (7 - x);
            y_pos = tile_row + (7 - y);
          }
          frame.set_pixel(x_pos, y_pos, rgb);
        }
      }
    }
  }
}

// ======================== HELPER FUNCTIONS ======================== //
std::array<uint8_t, 4> Renderer::bg_palette(const PPU& ppu, int tile_column,
                                            int tile_row) {
  int attr_table_index = (tile_row / 4) * 8 + (tile_column / 4);
  uint8_t attr_byte = ppu.vram[0x3C0 + attr_table_index];

  uint8_t palette_index = 0;
  int sub_x = (tile_column % 4) / 2;
  int sub_y = (tile_row % 4) / 2;
  if (sub_x == 0 && sub_y == 0) {
    palette_index = attr_byte & 0b11;
  } else if (sub_x == 1 && sub_y == 0) {
    palette_index = (attr_byte >> 2) & 0b11;
  } else if (sub_x == 0 && sub_y == 1) {
    palette_index = (attr_byte >> 4) & 0b11;
  } else if (sub_x == 1 && sub_y == 1) {
    palette_index = (attr_byte >> 6) & 0b11;
  }
  int palette_start = 1 + palette_index * 4;
  return {ppu.palette_table[0], ppu.palette_table[palette_start],
          ppu.palette_table[palette_start + 1],
          ppu.palette_table[palette_start + 2]};
}

std::array<uint8_t, 4> Renderer::sprite_palette(const PPU& ppu,
                                                uint8_t palette_index) {
  int start = 0x11 + palette_index * 4;
  return {0, ppu.palette_table[start], ppu.palette_table[start + 1],
          ppu.palette_table[start + 2]};
}