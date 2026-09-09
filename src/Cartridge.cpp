#include "../include/Cartridge.h"

#include <algorithm>
#include <stdexcept>
#include <string>

/**
 * Read from PRG ROM, panics if no cartridge is loaded or PRG ROM is empty.
 * Mirrors down address if PRG ROM is 16KiB.
 */
uint8_t Cartridge::read_prg_rom(uint16_t addr) const {
    if (empty) {
        throw std::runtime_error("Error: no cartridge loaded.");
    }
    if (prg_rom.empty()) {
        throw std::runtime_error("Error: cartridge PRG ROM is empty.");
    }

    if (addr < 0x8000) {
        throw std::out_of_range("PRG ROM address must be in $8000-$FFFF");
    }
    size_t index = static_cast<size_t>(addr - 0x8000);

    // mirror if prg_rom is 16KiB. Will need to modify this if additional
    // mappers are implemented.
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
uint8_t Cartridge::read_chr_rom(uint16_t addr) const {
    if (empty) {
        throw std::runtime_error(
            "Error: attempted to read from CHR ROM with no cartridge loaded.");
    }
    if (chr_rom.empty()) {
        throw std::runtime_error(
            "Error: attempted to read from CHR memory but CHR ROM is empty.");
    }
    return chr_rom[addr % chr_rom.size()];
}

/**
 * Writes affect CHR RAM only; cartridges with CHR ROM ignore them.
 */
void Cartridge::write_chr_ram(uint16_t addr, uint8_t value) {
    if (empty) {
        throw std::runtime_error(
            "Error: attempted to write to CHR RAM with no cartridge loaded.");
    }
    if (!chr_is_ram) {
        return;
    }
    if (chr_rom.empty()) {
        throw std::runtime_error("Error: attempted to write to empty CHR RAM.");
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

    const uint8_t loadedMapper = (romDump[7] & 0xF0) | (romDump[6] >> 4);

    // FLAGS 9
    // 76543210
    // ||||||||
    // |||||||+- TV system (0: NTSC; 1: PAL)
    // +++++++-- Reserved, set to zero
    const NESRegion loadedRegion = (romDump[9] & 1) ? NESRegion::PAL
                                                  : NESRegion::NTSC;

    if (loadedMapper != 0) {
        throw std::invalid_argument("Unsupported mapper " +
                                    std::to_string(loadedMapper) +
                                    "; only mapper 0 (NROM) is supported");
    }

    if (romDump[4] != 1 && romDump[4] != 2) {
        throw std::invalid_argument("NROM requires 16 or 32 KiB of PRG ROM");
    }
    if (romDump[5] > 1) {
        throw std::invalid_argument("NROM supports only 8 KiB of CHR ROM or RAM");
    }
    const size_t prg_rom_size = romDump[4] * size_t{16384};
    const size_t chr_rom_size = romDump[5] * size_t{8192};
    MirroringMode loadedMirroring;

    // determine mirroring mode:
    if (romDump[6] & 0b00001000) {
        // bit 3 of flags 6 is set [alt layout]
        loadedMirroring = MirroringMode::FourScreen;
    } else {
        // bit 0 of flags 6 set ? vertical mirroring, else horizontal
        loadedMirroring = (romDump[6] & 1) ? MirroringMode::Vertical
                                           : MirroringMode::Horizontal;
    }

    bool skip_trainer = (romDump[6] & 0b00000100) != 0;

    size_t prg_rom_start = 16 + (skip_trainer ? 512 : 0);
    size_t chr_rom_start = prg_rom_start + prg_rom_size;
    size_t rom_payload_size = prg_rom_size + chr_rom_size;

    if (romDump.size() < prg_rom_start + rom_payload_size) {
        throw std::invalid_argument("Invalid ROM file: insufficient data");
    }

    // Build replacement storage before changing the current cartridge. A bad
    // or truncated replacement ROM must leave the loaded cartridge intact.
    std::vector<uint8_t> loadedPrg(romDump.begin() + prg_rom_start,
                                   romDump.begin() + chr_rom_start);
    std::vector<uint8_t> loadedChr;
    if (chr_rom_size == 0) {
        // iNES uses CHR size 0 to indicate 8 KiB of CHR-RAM
        loadedChr.assign(8192, 0);
    } else {
        loadedChr.assign(romDump.begin() + chr_rom_start,
                         romDump.begin() + chr_rom_start + chr_rom_size);
    }

    prg_rom.swap(loadedPrg);
    chr_rom.swap(loadedChr);
    prg_ram.fill(0);
    if (skip_trainer) {
        std::copy_n(romDump.begin() + 16, 512, prg_ram.begin() + 0x1000);
    }
    mapper = loadedMapper;
    region = loadedRegion;
    mirroring = loadedMirroring;
    chr_is_ram = chr_rom_size == 0;
    empty = false;
}
