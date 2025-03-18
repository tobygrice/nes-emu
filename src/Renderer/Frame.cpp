/*
Frame Frame::show_tile(const std::vector<uint8_t>& chr_rom, size_t bank,
                     size_t tile_n) {
  assert(bank <= 1);
  Frame frame;
  size_t bank_offset = bank * 0x1000;
  size_t tile_start = bank_offset + tile_n * 16;
  const uint8_t* tile = &chr_rom[tile_start]; 

  for (int y = 0; y < 8; y++) {
    uint8_t upper = tile[y];
    uint8_t lower = tile[y + 8];
    // Process pixels in reverse order (from x = 7 down to 0)
    for (int x = 7; x >= 0; x--) {
      uint8_t value = ((upper & 1) << 1) | (lower & 1);
      upper >>= 1;
      lower >>= 1;
      Color rgb;
      // Map the two-bit value to one of four colors from the system palette.
      // (The palette indices 0x01, 0x23, 0x27, and 0x30 correspond to decimal
      // 1, 35, 39, and 48.)
      switch (value) {
        case 0:
          rgb = SYSTEM_PALETTE[1];
          break;
        case 1:
          rgb = SYSTEM_PALETTE[35];
          break;
        case 2:
          rgb = SYSTEM_PALETTE[39];
          break;
        case 3:
          rgb = SYSTEM_PALETTE[48];
          break;
        default:
          throw std::runtime_error("Invalid pixel value");
      }
      frame.set_pixel(x, y, rgb);
    }
  }
  return frame;
}
*/