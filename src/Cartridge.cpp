#include "../include/Cartridge.h"

/**
 * Read from PRG ROM, panics if no cartridge is loaded or PRG ROM is empty. Mirrors down address if PRG ROM is 16KiB.
 */
uint8_t Cartridge::read_prg_rom(uint16_t addr) {
    if (empty) {
        throw std::runtime_error("Error: no cartridge loaded.");
    }
    if (prg_rom.empty()) {
        throw std::runtime_error("Error: cartridge PRG ROM is empty.");
    }

    size_t index = static_cast<size_t>(addr - 0x8000);

    // mirror if prg_rom is 16KiB. Will need to modify this if additional mappers are implemented.
    if ((prg_rom.size() == 0x4000) && (index >= 0x4000)) {
        index %= 0x4000;
    }

    if (index >= prg_rom.size()) {
        throw std::out_of_range("PRG ROM read out of range");
    }

    return prg_rom[index];
}

/**
 * Read from CHR ROM, panics if no cartridge is loaded or CHR ROM is empty
 */
uint8_t Cartridge::read_chr_rom(uint16_t addr) {
    if (empty) {
        throw std::runtime_error("Error: attempted to read from CHR ROM with no cartridge loaded.");
    }
    if (chr_rom.empty()) {
        throw std::runtime_error("Error: attempted to read from CHR memory but CHR ROM is empty.");
    }
    return chr_rom[addr % chr_rom.size()];
}

/**
 * Write to CHR RAM, panics if CHR is ROM (or empty)
 */
void Cartridge::write_chr_ram(uint16_t addr, uint8_t value) {
    if (empty) {
        throw std::runtime_error("Error: attempted to write to CHR RAM with no cartridge loaded.");
    }
    if (!chr_is_ram || chr_rom.empty()) {
        throw std::runtime_error("Error: attempted to write to CHR ROM.");
    }
    chr_rom[addr % chr_rom.size()] = value;
}

/**
 * Load a cartridge from an iNES 1.0 ROM dump
 */
void Cartridge::load(const std::vector<uint8_t> &romDump) {
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

    prg_rom_size = romDump[4] * 16384; // PRG ROM size given in 16KiB blocks
    chr_rom_size = romDump[5] * 8192;  // CHR ROM size given in 8KiB blocks

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

    prg_rom.assign(romDump.begin() + prg_rom_start,
                   romDump.begin() + prg_rom_start + prg_rom_size);
    if (chr_rom_size == 0) {
        // iNES uses CHR size 0 to indicate 8 KiB of CHR-RAM
        chr_rom.assign(8192, 0);
        chr_is_ram = true;
    } else {
        chr_rom.assign(romDump.begin() + chr_rom_start,
                       romDump.begin() + chr_rom_start + chr_rom_size);
        chr_is_ram = false;
    }

    empty = false;
}
