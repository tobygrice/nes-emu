#ifndef RENDERER_H
#define RENDERER_H

#include <array>
#include <cstdint>

#include "Frame.h"
#include "PPU.h"
#include "Palette.h"

class Renderer {
 public:
  void render(const PPU& ppu, Frame& frame);

 private:
  // Helper: compute the background palette for a tile given its column and row.
  std::array<uint8_t, 4> bg_palette(const PPU& ppu, int tile_column,
                                    int tile_row);
  // Helper: compute the sprite palette based on the attribute index.
  std::array<uint8_t, 4> sprite_palette(const PPU& ppu, uint8_t palette_idx);
};

#endif  // RENDERER_H