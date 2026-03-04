#include "../include/Cartridge.h"

namespace {

constexpr uint32_t CRC32_POLY = 0xEDB88320u;
constexpr uint32_t PACMAN_MIRROR_OVERRIDE_CRC32 = 0x9E4E9CC2u;

uint32_t crc32(const uint8_t* data, size_t size) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < size; i++) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; bit++) {
      const uint32_t lsb = crc & 1u;
      crc >>= 1;
      if (lsb) {
        crc ^= CRC32_POLY;
      }
    }
  }
  return ~crc;
}

}  // namespace

uint8_t Cartridge::read_prg_rom(uint16_t addr) {
  if (empty) {
    throw std::runtime_error("Error: no cartridge loaded.");
  }
  if (prg_rom.empty()) {
    throw std::runtime_error("Error: cartridge PRG ROM is empty.");
  }

  size_t index = static_cast<size_t>(addr - 0x8000);

  // mirror if prg_rom is 16KiB:
  if ((prg_rom.size() == 0x4000) && (index >= 0x4000)) {
    index %= 0x4000;
  }

  if (index >= prg_rom.size()) {
    throw std::out_of_range("PRG ROM read out of range");
  }
  return prg_rom[index];
}

uint8_t Cartridge::read_chr_rom(uint16_t addr) {
  if (empty) {
    throw std::runtime_error("Error: no cartridge loaded.");
  }
  if (chr_rom.empty()) {
    throw std::runtime_error("Error: CHR memory is empty.");
  }
  return chr_rom[addr % chr_rom.size()];
}

void Cartridge::write_chr_rom(uint16_t addr, uint8_t value) {
  if (empty) {
    throw std::runtime_error("Error: no cartridge loaded.");
  }
  if (!chr_is_ram || chr_rom.empty()) {
    return;
  }
  chr_rom[addr % chr_rom.size()] = value;
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
  size_t rom_payload_size = prg_rom_size + chr_rom_size;

  if (romDump.size() < prg_rom_start + rom_payload_size) {
    throw std::invalid_argument("Invalid ROM file: insufficient data");
  }

  // Compatibility quirk: this mapper-0 Pacman dump has the mirroring bit set
  // opposite to how the game data is laid out.
  const uint32_t payload_crc = crc32(romDump.data() + prg_rom_start,
                                     rom_payload_size);
  if (mapper == 0x00 && mirroring == MirroringMode::Horizontal &&
      payload_crc == PACMAN_MIRROR_OVERRIDE_CRC32) {
    mirroring = MirroringMode::Vertical;
  }

  prg_rom.assign(romDump.begin() + prg_rom_start,
                 romDump.begin() + prg_rom_start + prg_rom_size);
  if (chr_rom_size == 0) {
    // iNES uses CHR size 0 to indicate 8 KiB of CHR-RAM.
    chr_rom.assign(8192, 0);
    chr_is_ram = true;
  } else {
    chr_rom.assign(romDump.begin() + chr_rom_start,
                   romDump.begin() + chr_rom_start + chr_rom_size);
    chr_is_ram = false;
  }

  empty = false;
}
