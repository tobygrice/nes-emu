#ifndef PPU_H
#define PPU_H

#include <array>
#include <cstdint>
#include <optional>

#include "../Cartridge.h"
#include "../Renderer/Frame.h"
#include "Registers/PPUAddr.h"
#include "Registers/PPUCtrl.h"
#include "Registers/PPUMask.h"
#include "Registers/PPUScroll.h"
#include "Registers/PPUStatus.h"

class PPU {
  private:
    std::optional<Frame> currentFrame = std::nullopt;

    // Background tile fetch state
    uint8_t tileID = 0;
    uint8_t attribute = 0;
    uint8_t patternLow = 0;
    uint8_t patternHigh = 0;

    // Registers
    PPUCtrl ctrl;         // 0x2000
    PPUMask mask;         // 0x2001
    PPUStatus status;     // 0x2002
    PPUScroll scroll;     // 0x2005
    PPUAddr addr;         // 0x2006
    uint8_t data_buf = 0; // 0x2007 read buffer

    uint8_t oam_addr = 0;                // 0x2003
    std::array<uint8_t, 256> oam_data{}; // 0x2004 sprite memory
    std::array<uint8_t, 32> palette_table{};

    // CIRAM backing the mirrored nametable space at $2000-$2FFF.
    std::array<uint8_t, 2048> vram{};
    Cartridge &cart;

    uint16_t cycles = 0;
    int scanline = 0;
    bool oddFrame = false;
    bool nmiInterrupt = false;
    bool suppressVblankThisFrame = false;

    // PPU I/O data bus latch ("open bus"). Lower bits of PPUSTATUS reads come
    // from this latch.
    uint8_t last_written_value = 0;

    // Private helpers
    uint16_t mirrorVRAMAddress(uint16_t addr);
    static uint8_t mirrorPaletteAddress(uint8_t addr);
    void pushBackgroundPixelPair(Frame &frame, uint8_t attributeQuadrant,
                                 uint8_t leftBit,
                                 bool backgroundRenderingEnabled);
    bool spriteZeroPixelOpaque(int screenX, int screenY) const;
    void evaluateSpriteZeroHit(int screenX, int screenY,
                               bool backgroundOpaque);
    void renderSprites(Frame &frame);

  public:
    PPU(const PPU &) = delete;
    PPU &operator=(const PPU &) = delete;
    PPU(PPU &&) = delete;
    PPU &operator=(PPU &&) = delete;

    explicit PPU(Cartridge &cart) : cart(cart) { oam_data.fill(0xFF); }

    std::optional<Frame> tick();

    bool getNMI() const { return nmiInterrupt; }
    uint16_t getScanline() const { return static_cast<uint16_t>(scanline); }
    uint16_t getCycle() const { return cycles; }
    uint8_t lastWrittenValue() const { return last_written_value; }

    uint8_t cpuRead();
    void cpuWrite(uint8_t value);

    // register read/writes
    void write_to_ctrl(uint8_t value);
    void write_to_mask(uint8_t value);
    uint8_t read_status();
    void write_to_oam_addr(uint8_t value);
    void write_to_oam_data(uint8_t value);
    uint8_t read_oam_data();
    void write_to_scroll(uint8_t value);
    void write_to_ppu_addr(uint8_t value);
    void write_oam_dma(const std::array<uint8_t, 256> &data);

    // getters/setters for testing only
    // read/writes without implementing additional expected behaviour
    uint8_t TEST_getvram(uint16_t address) const { return vram[address]; }
    void TEST_setvram(uint16_t address, uint8_t value) {
        vram[address] = value;
    }
    uint8_t TEST_getstatus() const { return status.snapshot(); }
    uint16_t TEST_getaddr() const { return addr.get(); }
    void TEST_set_vblank_status(bool val) { status.set_vblank_status(val); }
};

#endif
