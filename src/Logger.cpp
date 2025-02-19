#include "../include/Logger.h"

std::string Logger::disassembleInstr(uint16_t pc, const OpCode* op,
                             std::vector<uint8_t>* opBytes,
                             AddressResolveInfo& addrInfo,
                             uint8_t valueAtAddr) {
  {
    // A small helper to print 2-digit hex (uppercase) with leading zeros.
    auto hex2 = [&](uint8_t v) {
      std::ostringstream oss;
      oss << std::uppercase << std::setw(2) << std::setfill('0') << std::hex
          << (int)v;
      return oss.str();
    };

    // A small helper to print 4-digit hex (uppercase) with leading zeros.
    auto hex4 = [&](uint16_t v) {
      std::ostringstream oss;
      oss << std::uppercase << std::setw(4) << std::setfill('0') << std::hex
          << (int)v;
      return oss.str();
    };

    // Start building the disassembly string: "name "
    std::ostringstream out;
    out << std::uppercase << op->name << " ";

    // For the addressing modes, build the operand string in nestest style.
    switch (op->mode) {
      case AddressingMode::Implied:
        // e.g. "CLC", "INX", no operand needed
        // out << "" (nothing)
        break;

      case AddressingMode::Accumulator:
        // e.g. "ROL A"
        out << "A";
        break;

      case AddressingMode::Immediate:
        // e.g. "CMP #$AA"
        out << "#$" << hex2((*opBytes)[1]);
        break;

      case AddressingMode::Relative:
        // e.g. "BNE $D038"
        // The final address is already in addrInfo.address
        out << "$" << hex4(addrInfo.address);
        break;

      case AddressingMode::ZeroPage:
        // e.g. "LDA $03 = AB"
        out << "$" << hex2((*opBytes)[1]);
        out << " = " << hex2(valueAtAddr);
        break;

      case AddressingMode::ZeroPage_X:
        // e.g. "LDA $03,X = AB"
        out << "$" << hex2((*opBytes)[1]) << ",X";
        out << " = " << hex2(valueAtAddr);
        break;

      case AddressingMode::ZeroPage_Y:
        // e.g. "LDA $03,Y = AB"
        out << "$" << hex2((*opBytes)[1]) << ",Y";
        out << " = " << hex2(valueAtAddr);
        break;

      case AddressingMode::Absolute:
        // e.g. "LDA $0300 = AB"
        out << "$" << hex4(((*opBytes)[2] << 8) | (*opBytes)[1]);
        out << " = " << hex2(valueAtAddr);
        break;

      case AddressingMode::Absolute_X:
        // e.g. "LDA $0300,X = AB"
        out << "$" << hex4(((*opBytes)[2] << 8) | (*opBytes)[1]) << ",X";
        out << " = " << hex2(valueAtAddr);
        break;

      case AddressingMode::Absolute_Y:
        // e.g. "LDA $0300,Y = AB"
        out << "$" << hex4(((*opBytes)[2] << 8) | (*opBytes)[1]) << ",Y";
        out << " = " << hex2(valueAtAddr);
        break;

      case AddressingMode::Indirect:
        // e.g. "JMP ($0300) = 1234"
        // nestest typically shows the final pointer: "($0300) = FCE2"
        out << "($" << hex4(((*opBytes)[2] << 8) | (*opBytes)[1]) << ")";
        out << " = " << hex4(addrInfo.address);
        break;

      case AddressingMode::Indirect_X:
        // e.g. "STA ($FF,X) @ FF = 0400 = 5D"
        out << "($" << hex2((*opBytes)[1]) << ",X)";
        out << " @ " << hex2(addrInfo.pointerAddress);
        out << " = " << hex4(addrInfo.address);
        out << " = " << hex2(valueAtAddr);
        break;

      case AddressingMode::Indirect_Y:
        // e.g. "STA ($FF),Y @ FF = 0400 = 5D"
        out << "($" << hex2((*opBytes)[1]) << "),Y";
        out << " @ " << hex2(addrInfo.pointerAddress);
        out << " = " << hex4(addrInfo.address);
        out << " = " << hex2(valueAtAddr);
        break;

      default:
        // Fallback if you have other modes not shown
        break;
    }

    return out.str();
  }
}

void Logger::log(uint16_t pc, const OpCode* op, std::vector<uint8_t>* opBytes,
         AddressResolveInfo* addrInfo, uint8_t valueAtAddr, uint8_t A,
         uint8_t X, uint8_t Y, uint8_t P, uint8_t SP, int ppuX, int ppuY,
         uint64_t cycles) {
  std::ostringstream oss;

  // We want uppercase hex with zero padding for PC and opBytes.
  oss << std::uppercase << std::setfill('0');

  // Print the 4-digit PC (hex).
  oss << std::hex << std::setw(4) << pc << "  ";

  // Print up to 3 opcode bytes (2-digit hex each). If fewer than 3,
  // pad with spaces so columns line up.
  for (size_t i = 0; i < 3; i++) {
    if (i < opBytes->size()) {
      oss << std::setw(2) << static_cast<int>((*opBytes)[i]) << " ";
    } else {
      oss << "   ";
    }
  }

  // Leave some space, then print the disassembly in a left-justified field
  // so the registers line up in the same columns every time.
  std::string disassembly =
      disassembleInstr(pc, op, opBytes, *addrInfo, valueAtAddr);
  oss << "  " << std::left << std::setw(28) << disassembly;

  // Switch to hex for registers (A, X, Y, P, SP).
  // Each register is 2 hex digits.
  oss << "  A:" << std::setw(2) << static_cast<int>(A) << " X:" << std::setw(2)
      << static_cast<int>(X) << " Y:" << std::setw(2) << static_cast<int>(Y)
      << " P:" << std::setw(2) << static_cast<int>(P) << " SP:" << std::setw(2)
      << static_cast<int>(SP);

  // Switch to decimal for PPU coordinates and cycle count.
  oss << std::dec << std::setfill(' ');

  // PPU: X,Y
  oss << " PPU: " << std::setw(2) << ppuX << "," << std::setw(3) << ppuY;

  // CPU cycle count
  oss << " CYC:" << cycles;

  // Print the final line to stdout.
  std::cout << oss.str() << std::endl;
}