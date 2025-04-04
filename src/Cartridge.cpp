#include "../include/Cartridge.h"

#include <iomanip>  // for std::setw and std::setfill
#include <iostream>

// error point: not checking addr out of bounds -> seg fault
// should not be an issue iff provided rom adheres to expectactions
uint8_t Cartridge::read_prg_rom(uint16_t addr) {
  if (empty) {
    throw std::runtime_error("Error: no cartridge loaded.");
  }

  addr -= 0x8000;

  // mirror if prg_rom is 16KiB:
  if ((prg_rom.size() == 0x4000) && (addr >= 0x4000)) {
    addr %= 0x4000;
  }

  return prg_rom[addr];
}

// error point: not checking addr out of bounds -> seg fault
// should not be an issue iff provided rom adheres to expectactions
uint8_t Cartridge::read_chr_rom(uint16_t addr) {
  if (empty) {
    throw std::runtime_error("Error: no cartridge loaded.");
  }
  return chr_rom[addr];
}

void Cartridge::load(const std::vector<uint8_t>& romDump) {
  // validate iNES header
  // 0-3 | Constant "NES" ($4E $45 $53 $1A - ASCII "NES" followed by EOF char)
  if (romDump.size() < 16 || romDump[0] != 'N' || romDump[1] != 'E' ||
      romDump[2] != 'S' || romDump[3] != 0x1A) {
    throw std::invalid_argument("File is not in iNES file format");
  }

  uint8_t ines_ver = (romDump[7] >> 2) & 0b00000011;
  if (ines_ver != 0) {
    throw std::invalid_argument("NES2.0 format is not supported yet.");
  }

  mapper = (romDump[7] & 0b11110000) | (romDump[6] >> 4);

  // FLAGS 9
  // 76543210
  // ||||||||
  // |||||||+- TV system (0: NTSC; 1: PAL)
  // +++++++-- Reserved, set to zero
  region = romDump[9] == 0 ? NESRegion::NTSC : NESRegion::PAL;

  if (mapper != 0x00) {
    throw std::runtime_error("mapper not implemented");
  }

  prg_rom_size = romDump[4] * 16384;  // PRG ROM size given in 16KiB blocks
  chr_rom_size = romDump[5] * 8192;   // CHR ROM size given in 8KiB blocks

  // determine mirroring mode:
  if (romDump[6] & 0b00001000) {
    // bit 3 of flags 6 is set [alt layout]
    mirroring = MirroringMode::FourScreen;
  } else {
    // bit 0 of flags 6 set ? vertical mirroring, else horizontal
    mirroring = (romDump[6] & 0b1) ? MirroringMode::Vertical
                                   : MirroringMode::Horizontal;
  }

  bool skip_trainer = (romDump[6] & 0b00000100) != 0;

  size_t prg_rom_start = 16 + (skip_trainer ? 512 : 0);
  size_t chr_rom_start = prg_rom_start + prg_rom_size;

  if (romDump.size() < chr_rom_start + chr_rom_size) {
    throw std::invalid_argument("Invalid ROM file: insufficient data");
  }

  prg_rom.assign(romDump.begin() + prg_rom_start,
                 romDump.begin() + prg_rom_start + prg_rom_size);
  chr_rom.assign(romDump.begin() + chr_rom_start,
                 romDump.begin() + chr_rom_start + chr_rom_size);

  empty = false;
}