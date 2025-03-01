#include "../../include/PPU/PPU.h"

uint8_t PPU::read_data() {
  // Retrieve the current VRAM address
  uint16_t addr_val = addr.get();
  // Increment the VRAM address (side-effect)
  increment_vram_addr();

  // Branch based on the address range.
  if (addr_val <= 0x1FFF) {
    // CHR-ROM read
    uint8_t result = internal_buffer;
    internal_buffer =
        bus->read(addr_val);  // error point: may need to add/subtract a value
                              // to account for offset in bus

  } else if (addr_val <= 0x2FFF) {
    // TODO: Implement RAM read.
  } else if (addr_val <= 0x3EFF) {
    // This address range should not be used.
    throw std::runtime_error(
        "Address space 0x3000..0x3EFF is not expected to be used, "
        "requested: " +
        std::to_string(addr_val));
  } else if (addr_val <= 0x3FFF) {
    // Read from the palette table.
    return palette_table[addr_val - 0x3F00];
  } else {
    // Catch any unexpected mirrored address access.
    throw std::runtime_error("Unexpected access to mirrored space: " +
                             std::to_string(addr_val));
  }
}