#ifndef RENDERER_H
#define RENDERER_H

#include <array>
#include <cstdint>

#include "../PPU/PPU.h"
#include "Frame.h"
#include "Palette.h"

class Renderer {
 private:
  // Helper: compute the background palette for a tile given its column and row.
  std::array<uint8_t, 4> bg_palette(const PPU& ppu, int tile_column,
                                    int tile_row);
  // Helper: compute the sprite palette based on the attribute index.
  std::array<uint8_t, 4> sprite_palette(const PPU& ppu, uint8_t palette_idx);

 public:
  void render(const PPU& ppu, Frame& frame);
};

#endif  // RENDERER_H