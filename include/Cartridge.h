/**
 * Provides support for iNES 1.0 format ROMs.
 *
 * iNES Header Format (16 bytes) - https://www.nesdev.org/wiki/INES
 * Bytes   | Description
 * ---------------------------------------------
 * 0-3     | Constant "NES" ($4E $45 $53 $1A - ASCII "NES" followed by EOF char)
 * 4       | Size of PRG ROM in 16 KB units
 * 5       | Size of CHR ROM in 8 KB units (0 means board uses CHR RAM)
 * 6       | Flags 6 – Mapper, mirroring, battery, trainer
 * 7       | Flags 7 – Mapper, VS/Playchoice, NES 2.0
 * 8       | Flags 8 – PRG-RAM size (rarely used)
 * 9       | Flags 9 – TV system (rarely used)
 * 10      | Flags 10 – TV system, PRG-RAM presence (unofficial)
 * 11-15   | Unused padding (should be zero, but may contain ripper data)
 *
 * FLAGS 6
 *  76543210
 *  ||||||||
 *  |||||||+- Nametable arrangement: 0: vertical arrangement ("horizontal
 *  |||||||                             mirrored") (CIRAM A10 = PPU A11)
 *  |||||||                          1: horizontal arrangement ("vertically
 *  |||||||                             mirrored") (CIRAM A10 = PPU A10)
 *  ||||||+-- 1: Cartridge contains battery-backed PRG RAM ($6000-7FFF) or other
 *  ||||||       persistent memory
 *  |||||+--- 1: 512-byte trainer at $7000-$71FF (stored before PRG data)
 *  ||||+---- 1: Alternative nametable layout
 *  ++++----- Lower nybble of mapper number
 *
 * FLAGS 7
 *  76543210
 *  ||||||||
 *  |||||||+- VS Unisystem
 *  ||||||+-- PlayChoice-10 (8 KB of Hint Screen data stored after CHR data)
 *  ||||++--- If equal to 2, flags 8-15 are in NES 2.0 format
 *  ++++----- Upper nybble of mapper number
 */

#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <string>
#include <vector>

// enum class for addressing modes
enum class MirroringMode { Vertical, Horizontal, FourScreen };
enum class NESRegion { NTSC, PAL, None };

class Cartridge {
 private:
  bool empty;
  std::vector<uint8_t> prg_rom;
  std::vector<uint8_t> chr_rom;
  MirroringMode mirroring;
  NESRegion region;
  uint8_t mapper;
  size_t prg_rom_size;
  size_t chr_rom_size;

 public:
  Cartridge()
      : empty(true),
        prg_rom{},
        chr_rom{},
        mirroring(MirroringMode::Horizontal),
        region(NESRegion::None),
        mapper(),
        prg_rom_size(0),
        chr_rom_size(0) {}

  Cartridge(const std::vector<uint8_t>& raw) : Cartridge() { load(raw); }

  void load(const std::vector<uint8_t>& raw);
  uint8_t read_prg_rom(uint16_t addr);
  uint8_t read_chr_rom(uint16_t addr);

  bool isEmpty() { return empty; }
  MirroringMode getMirroring() { return mirroring; }
  void setMirroring(MirroringMode m) { this->mirroring = m; }
  const uint8_t* get_chr_rom_addr() {
    if (empty) {
      throw std::runtime_error("Error: no cartridge loaded.");
    }
    return chr_rom.data();
  }

  NESRegion getRegion() { return region; }

};

#endif