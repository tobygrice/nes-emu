#include "../include/Logger.h"

std::string Logger::disassembleInstr(CPUState* state) {
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

    if (state->op->isDocumented) {
      out << " ";
    } else {
      out << "*";
    }
    out << std::uppercase << state->op->name << " ";

    // For the addressing modes, build the operand string in nestest style.
    switch (state->op->mode) {
      case AddressingMode::Implied:
        // e.g. "CLC", "INX", no operand needed
        // out << "" (nothing)
        break;

      case AddressingMode::Acc:
        // e.g. "ROL A"
        out << "A";
        break;

      case AddressingMode::Immediate:
        // e.g. "CMP #$AA"
        out << "#$" << hex2(state->opBytes->at(1));
        break;

      case AddressingMode::Relative:
        // e.g. "BNE $D038"
        // The final address is already in addrInfo->address
        out << "$" << hex4(state->addrInfo->address);
        break;

      case AddressingMode::ZeroPage:
        // e.g. "LDA $03 = AB"
        out << "$" << hex2(state->opBytes->at(1));
        out << " = " << hex2(state->valueAtAddr);
        break;

      case AddressingMode::ZeroPageX:
        // e.g. "LDA $03,X = AB"
        out << "$" << hex2(state->opBytes->at(1)) << ",X";
        out << " @ " << hex2(state->addrInfo->address);  // Include computed address
        out << " = " << hex2(state->valueAtAddr);
        break;

      case AddressingMode::ZeroPageY:
        // e.g. "LDA $03,Y = AB"
        out << "$" << hex2(state->opBytes->at(1)) << ",Y";
        out << " @ " << hex2(state->addrInfo->address);  // Include computed address
        out << " = " << hex2(state->valueAtAddr);
        break;

      case AddressingMode::Absolute:
        out << "$" << hex4((state->opBytes->at(2) << 8) | state->opBytes->at(1));
        if (state->op->name != "JSR" &&
            state->op->name != "JMP") {  // Skip = XX for JSR and JMP
          out << " = " << hex2(state->valueAtAddr);
        }
        break;

      case AddressingMode::AbsoluteX:
        out << "$" << hex4((state->opBytes->at(2) << 8) | state->opBytes->at(1)) << ",X";
        out << " @ " << hex4(state->addrInfo->address);  // Include computed address
        out << " = " << hex2(state->valueAtAddr);
        break;

      case AddressingMode::AbsoluteY:
        out << "$" << hex4((state->opBytes->at(2) << 8) | state->opBytes->at(1)) << ",Y";
        out << " @ " << hex4(state->addrInfo->address);  // Include computed address
        out << " = " << hex2(state->valueAtAddr);
        break;

      case AddressingMode::Indirect:
        out << "($" << hex4((state->opBytes->at(2) << 8) | state->opBytes->at(1)) << ")";
        out << " = "
            << hex4(state->addrInfo->address);  // Show final resolved address, no = XX
        break;

      case AddressingMode::IndirectX:
        out << "($" << hex2(state->opBytes->at(1)) << ",X)";
        out << " @ "
            << hex2(state->addrInfo->pointerAddress);  // Intermediate zero-page pointer
        out << " = " << hex4(state->addrInfo->address);  // Final resolved address
        out << " = " << hex2(state->valueAtAddr);       // Value at final address
        break;

      case AddressingMode::IndirectY:
        // Expected format: "LDA ($89),Y = 0300 @ 0300 = 89"
        out << "($" << hex2(state->opBytes->at(1)) << "),Y";
        out << " = " << hex4(state->addrInfo->pointerAddress);  // Show base pointer
        out << " @ " << hex4(state->addrInfo->address);  // Show final computed address
        out << " = " << hex2(state->valueAtAddr);       // Show value at final address
        break;

      default:
        // Fallback if you have other modes not shown
        break;
    }

    return out.str();
  }
}

void Logger::log(CPUState state) {
  if (silenced) return;

  std::string line(94, ' ');  // allocate 94 char string
  // std::string line(73, ' ');  // allocate 73 char string (no ppu + cycle)

  // Field 1: PC at index 0 (4 characters)
  {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(4) << std::setfill('0')
        << state.pc;
    line.replace(0, 4, oss.str());
  }

  // Field 2: Opcode bytes at index 6 (9 characters wide)
  {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0');
    // Print up to 3 opcode bytes separated by spaces.
    for (size_t i = 0; i < state.opBytes->size() && i < 3; i++) {
      oss << std::setw(2) << static_cast<int>(state.opBytes->at(i));
      if (i < 2) {
        oss << " ";
      }
    }
    std::string opStr = oss.str();
    // Pad the opcode field to exactly 9 characters.
    if (opStr.size() < 9) {
      opStr.append(9 - opStr.size(), ' ');
    }
    line.replace(6, 9, opStr);
  }

  // Field 3: Disassembly at index 17 (28 characters wide)
  {
    std::string dis = disassembleInstr(&state);
    if (dis.size() < 31) {
      dis.append(31 - dis.size(), ' ');
    } else if (dis.size() > 31) {
      dis = dis.substr(0, 31);
    }
    line.replace(15, 31, dis);
  }

  // Field 4: Registers at index 49 (25 characters wide)
  // Format: "A:XX X:XX Y:XX P:XX SP:XX"
  {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0')
        << "A:" << std::setw(2) << static_cast<int>(state.A) << " "
        << "X:" << std::setw(2) << static_cast<int>(state.X) << " "
        << "Y:" << std::setw(2) << static_cast<int>(state.Y) << " "
        << "P:" << std::setw(2) << static_cast<int>(state.P) << " "
        << "SP:" << std::setw(2) << static_cast<int>(state.SP);
    std::string regStr = oss.str();
    if (regStr.size() < 25) {
      regStr.append(25 - regStr.size(), ' ');
    } else if (regStr.size() > 25) {
      regStr = regStr.substr(0, 25);
    }
    line.replace(48, 25, regStr);
  }

  // Field 5: PPU at index 75 (11 characters wide)
  // Format: "PPU:XXX,YYY"
  {
    std::ostringstream oss;
    // Use "PPU:" without an extra trailing space
    oss << "PPU:" << std::setw(3) << std::setfill(' ') << std::right << state.ppuX
        << "," << std::setw(3) << std::setfill(' ') << std::right << state.ppuY;
    std::string ppuStr = oss.str();
    if (ppuStr.size() < 11) {
      ppuStr.append(11 - ppuStr.size(), ' ');
    } else if (ppuStr.size() > 11) {
      ppuStr = ppuStr.substr(0, 11);
    }
    line.replace(74, 11, ppuStr);
  }

  // Field 6: Cycles at index 86 (8 characters wide)
  // Format: "CYC:XXXX" (no extra preceding space)
  {
    std::ostringstream oss;
    oss << "CYC:" << state.cycles;
    line.replace(86, 8, oss.str());
  }

  std::cout << line << "\r\n";
}
