#include "../../include/CPU/CPU.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../../include/Bus.h"

// https://github.com/SingleStepTests/65x02/tree/main/nes6502

void CPU::tick() {
  if (initiatingInterrupt) {
    // interrupt has been triggered
    switch (activeInterrupt) {
      case Interrupt::NMI:
      case Interrupt::IRQ:
        in_NMI_IRQ();
        break;
      case Interrupt::RES:
        in_RES();
        break;
      default:
        initiatingInterrupt = false;
        break;
    }
    return;
  }

  if (cyclesRemainingInCurrentInstr == 0) {
    // new instruction
    logger->log(logState);  // log previous instruction
    delete logState;

    // assumes pc has already been incremented past previous operand bytes
    uint8_t opcode = bus->read(pc);
    currentOpCode = getOpCode(opcode);

    // CAPTURE CPU STATE FOR LOGGING
    // - pass pointer to currentOpBytes, currAddrResCtx, currentValueAtAddress
    // - all other elements of logState are locked to their current state
    // - call logger.log(logState) once currentOpBytes, currAddrResCtx, and
    //   currentValueAtAddress have been computed
    logState = new CPUState(
        pc, *currentOpCode, &currentOpBytes, &currAddrResCtx,
        &currentValueAtAddress, a_register, x_register, y_register, status, sp,
        bus->getPPUCycle(), bus->getPPUScanline(), bus->getCycleCount());

    // increment PC to point at first operand
    pc++;
    cyclesRemainingInCurrentInstr = currentOpCode->cycles;

    // reset values for new instruction
    currentHighByte = 0;
    currentOpBytes.clear();
    currentOpBytes.push_back(opcode);
    currAddrResCtx.reset(currentOpCode->mode);

    // bus->read(pc) means this was a cycle, count cycle and return
    cyclesRemainingInCurrentInstr--;
    return;
  }

  // Relative addressing is exclusively used by branching instructions.
  // On actual 6502 hardware, the target address is not computed until the
  // CPU decides that the branch should be taken. To improve optimisation,
  // absolute address will be computed inside branch handler iff branch is taken
  if ((currAddrResCtx.state != ResolutionState::Done) &&
      (currAddrResCtx.mode != AddressingMode::Relative)) {
    computeAbsoluteAddress();

    if (currAddrResCtx.state != ResolutionState::Done) {
      cyclesRemainingInCurrentInstr--;
      return;  // return if absolute address still not resolved
    }

    if (currAddrResCtx.waitPageCrossed) {
      // page was crossed in address resolution, emulate extra cycle.
      // op->cycles does not include potential page crossings so do not
      // decrement remaining cycles here
      currAddrResCtx.waitPageCrossed = false;
      return;
    }
  }

  // execute instruction
  (this->*(currentOpCode->handler))(currAddrResCtx.address);

  cyclesRemainingInCurrentInstr--;
  return;
}

/**
 * Function to return the address of the operand given the addressing mode.
 *
 * @param mode The addressing mode specified by the opcode
 * @return A 16-bit memory address
 */
void CPU::computeAbsoluteAddress() {
  switch (currentOpCode->mode) {
    case AddressingMode::Implied:
    case AddressingMode::Acc: {
      // accumulator and implicit opcodes do not require an address
      currAddrResCtx.state = ResolutionState::Done;
      break;
    }
    case AddressingMode::Relative: {
      throw std::runtime_error(
          "Relative addressing should be computed by branch instruction.");
      break;
    }
    case AddressingMode::Immediate: {
      // pc currently pointing at next opcode
      currAddrResCtx.address = pc - 1;
      currAddrResCtx.state = ResolutionState::Done;
      break;
    }
    case AddressingMode::ZeroPage: {
      if (currAddrResCtx.state == ResolutionState::Init) {
        readOperand();
        currAddrResCtx.state = ResolutionState::ComputeAddress;
      } else {  // ComputeAddress
        currAddrResCtx.address = currentOpBytes[1];
        currAddrResCtx.state = ResolutionState::Done;
      }
      break;
    }
    case AddressingMode::Absolute: {
      switch (currAddrResCtx.state) {
        case ResolutionState::Init: {
          readOperand();
          currAddrResCtx.state = ResolutionState::ReadOperand;
          break;
        }
        case ResolutionState::ReadOperand: {
          readOperand();
          currAddrResCtx.state = ResolutionState::ComputeAddress;
          // compute address and reset PC in the same cycle for JMP (0x4C) only
          if (currentOpCode->code != 0x4C) break;
        }
        case ResolutionState::ComputeAddress: {
          currAddrResCtx.address =
              assembleBytes(currentOpBytes[2], currentOpBytes[1]);
          currAddrResCtx.state = ResolutionState::Done;
          break;
        }
        default: {
          throw std::runtime_error("Unexpected value in switch statement.");
          break;
        }
      }
      break;
    }
    case AddressingMode::ZeroPageX: {
      if (currAddrResCtx.state == ResolutionState::Init) {
        readOperand();
        currAddrResCtx.state = ResolutionState::ComputeAddress;
      } else {  // ComputeAddress
        uint8_t addr = static_cast<uint8_t>(currentOpBytes[1] + x_register);
        currAddrResCtx.address = static_cast<uint16_t>(addr);
        currAddrResCtx.state = ResolutionState::Done;
      }
      break;
    }
    case AddressingMode::ZeroPageY: {
      if (currAddrResCtx.state == ResolutionState::Init) {
        readOperand();
        currAddrResCtx.state = ResolutionState::ComputeAddress;
      } else {  // ComputeAddress
        uint8_t addr = static_cast<uint8_t>(currentOpBytes[1] + y_register);
        currAddrResCtx.address = static_cast<uint16_t>(addr);
        currAddrResCtx.state = ResolutionState::Done;
      }
      break;
    }
    case AddressingMode::AbsoluteX: {
      switch (currAddrResCtx.state) {
        case ResolutionState::Init: {
          readOperand();
          currAddrResCtx.state = ResolutionState::ReadOperand;
          break;
        }
        case ResolutionState::ReadOperand: {
          readOperand();
          currAddrResCtx.state = ResolutionState::ComputeAddress;
          break;
        }
        case ResolutionState::ComputeAddress: {
          currentHighByte = currentOpBytes[2];  // for SHA, SHX, SHY
          uint16_t base = assembleBytes(currentOpBytes[2], currentOpBytes[1]);
          currAddrResCtx.address = base + x_register;
          if (((base & 0xFF00) != (currAddrResCtx.address & 0xFF00)) &&
              !currentOpCode->ignorePageCrossings) {
            currAddrResCtx.waitPageCrossed = true;
          }
          currAddrResCtx.state = ResolutionState::Done;
          break;
        }
        default: {
          throw std::runtime_error("Unexpected value in switch statement.");
          break;
        }
      }
      break;
    }
    case AddressingMode::AbsoluteY: {
      switch (currAddrResCtx.state) {
        case ResolutionState::Init: {
          readOperand();
          currAddrResCtx.state = ResolutionState::ReadOperand;
          break;
        }
        case ResolutionState::ReadOperand: {
          readOperand();
          currAddrResCtx.state = ResolutionState::ComputeAddress;
          break;
        }
        case ResolutionState::ComputeAddress: {
          currentHighByte = currentOpBytes[2];  // for SHA, SHX, SHY
          uint16_t base = assembleBytes(currentOpBytes[2], currentOpBytes[1]);
          currAddrResCtx.address = base + y_register;
          if (((base & 0xFF00) != (currAddrResCtx.address & 0xFF00)) &&
              !currentOpCode->ignorePageCrossings) {
            currAddrResCtx.waitPageCrossed = true;
          }

          currAddrResCtx.state = ResolutionState::Done;
          break;
        }
        default: {
          throw std::runtime_error("Unexpected value in switch statement.");
          break;
        }
      }
      break;
    }
    case AddressingMode::Indirect: {
      switch (currAddrResCtx.state) {
        case ResolutionState::Init: {
          readOperand();
          currAddrResCtx.state = ResolutionState::ReadOperand;
          break;
        }
        case ResolutionState::ReadOperand: {
          readOperand();
          currAddrResCtx.state = ResolutionState::ReadIndirect_Low;
          break;
        }
        case ResolutionState::ReadIndirect_Low: {
          currAddrResCtx.pointerAddress =
              assembleBytes(currentOpBytes[2], currentOpBytes[1]);
          currAddrResCtx.address = bus->read(currAddrResCtx.pointerAddress);
          currAddrResCtx.state = ResolutionState::ReadIndirect_High;
          break;
        }
        case ResolutionState::ReadIndirect_High: {
          /* https://www.nesdev.org/obelisk-6502-guide/reference.html#JMP
           "An original 6502 has does not correctly fetch the target address
           if the indirect vector falls on a page boundary (e.g. $xxFF where
           xx is any value from $00 to $FF). In this case fetches the LSB from
           $xxFF as expected but takes the MSB from $xx00. This is fixed in
           some later chips like the 65SC02 so for compatibility always ensure
           the indirect vector is not at the end of the page." */
          uint8_t msb;
          if ((currAddrResCtx.pointerAddress & 0x00FF) == 0x00FF) {
            // emulate known bug - wrap to beginning of page
            msb = bus->read(currAddrResCtx.pointerAddress & 0xFF00);
          } else {
            msb = bus->read(currAddrResCtx.pointerAddress + 1);
          }
          // we can compute the address here but still need to emulate one
          // more cycle
          currAddrResCtx.address =
              (static_cast<uint16_t>(msb) << 8) | currAddrResCtx.address;
          currAddrResCtx.state = ResolutionState::ComputeAddress;
          break;
        }
        case ResolutionState::ComputeAddress: {
          currAddrResCtx.state = ResolutionState::Done;
          break;
        }
        default: {
          throw std::runtime_error("Unexpected value in switch statement.");
          break;
        }
      }
      break;
    }
    case AddressingMode::IndirectX: {
      switch (currAddrResCtx.state) {
        case ResolutionState::Init: {
          readOperand();
          currAddrResCtx.state = ResolutionState::ReadIndirect_Low;
          break;
        }
        case ResolutionState::ReadIndirect_Low: {
          currAddrResCtx.pointerAddress = currentOpBytes[1] + x_register;
          currAddrResCtx.pointerUsed = true;
          currAddrResCtx.address = bus->read(currAddrResCtx.pointerAddress);
          currAddrResCtx.state = ResolutionState::ReadIndirect_High;
          break;
        }
        case ResolutionState::ReadIndirect_High: {
          uint8_t high = bus->read(
              static_cast<uint8_t>(currAddrResCtx.pointerAddress + 1));
          currAddrResCtx.address =
              (static_cast<uint16_t>(high) << 8) | currAddrResCtx.address;
          currAddrResCtx.state = ResolutionState::ComputeAddress;
          // we can compute the address here but still need to emulate one
          // more cycle
          break;
        }
        case ResolutionState::ComputeAddress: {
          currAddrResCtx.state = ResolutionState::Done;
          break;
        }
        default: {
          throw std::runtime_error("Unexpected value in switch statement.");
          break;
        }
      }
      break;
    }
    case AddressingMode::IndirectY: {
      switch (currAddrResCtx.state) {
        case ResolutionState::Init: {
          readOperand();
          currAddrResCtx.state = ResolutionState::ReadIndirect_Low;
          break;
        }
        case ResolutionState::ReadIndirect_Low: {
          currAddrResCtx.pointerUsed = true;
          currAddrResCtx.pointerAddress = bus->read(currentOpBytes[1]);
          currAddrResCtx.state = ResolutionState::ReadIndirect_High;
          break;
        }
        case ResolutionState::ReadIndirect_High: {
          currAddrResCtx.pointerAddress |=
              static_cast<uint16_t>(
                  bus->read(static_cast<uint8_t>(currentOpBytes[1] + 1)))
              << 8;
          currAddrResCtx.state = ResolutionState::ComputeAddress;
          break;
        }
        case ResolutionState::ComputeAddress: {
          currAddrResCtx.address = currAddrResCtx.pointerAddress + y_register;
          if (((currAddrResCtx.pointerAddress & 0xFF00) !=
               (currAddrResCtx.address & 0xFF00)) &&
              !currentOpCode->ignorePageCrossings) {
            // MSB of base address and resulting addition address is different
            currAddrResCtx.waitPageCrossed = true;  // +1 cycle for page crossed
          }
          currAddrResCtx.state = ResolutionState::Done;
          break;
        }
        default: {
          throw std::runtime_error("Unexpected value in switch statement.");
          break;
        }
      }
      break;
    }
    default: {
      throw std::runtime_error("Addressing mode not supported");
    }
  }
}

/**
 * https://www.nesdev.org/wiki/CPU_interrupts
 * #  address R/W description
 *--- ------- --- -----------------------------------------------
 * 1    PC     R  fetch opcode and discard it - $00 (BRK) is forced
 * 2    PC     R  read next instruction byte (discard same as above)
 * 3  $0100,S  W  push PCH on stack, decrement S
 * 4  $0100,S  W  push PCL on stack, decrement S
 **** At this point, the signal status determines which interrupt vector is used
 * 5  $0100,S  W  push P on stack (with B flag *clear*), decrement S
 * 6   A       R  fetch PCL (A = FFFE for IRQ, A = FFFA for NMI), set I flag
 * 7   A       R  fetch PCH (A = FFFF for IRQ, A = FFFB for NMI)
 */
void CPU::in_NMI_IRQ() {
  cyclesRemainingInCurrentInterrupt--;
  switch (cyclesRemainingInCurrentInterrupt) {
    case 6: {
      bus->read(pc);  // fetch opcode and discard
      break;          // burn cycle
    }
    case 5: {
      bus->read(pc + 1);  // fetch operand and discard
      break;
    }
    case 4: {
      push((pc >> 8) & 0xFF);
      break;
    }
    case 3: {
      push(static_cast<uint8_t>(pc & 0xFF));
      break;
    }
    case 2: {
      // push(status | FLAG_BREAK);
      push(status & ~FLAG_BREAK);
      break;
    }
    case 1: {
      status |= FLAG_INTERRUPT;  // set the interrupt flag
      uint16_t address = (activeInterrupt == Interrupt::NMI) ? 0xFFFA : 0xFFFE;
      pc = bus->read(address);
      break;
    }
    case 0: {
      uint16_t address = (activeInterrupt == Interrupt::NMI) ? 0xFFFB : 0xFFFF;
      pc |= (static_cast<uint16_t>(bus->read(address)) << 8);
      cyclesRemainingInCurrentInterrupt = 7;  // reset for next interrupt
      initiatingInterrupt = false;
      break;
    }
  }
}

void CPU::in_RES() {
  cyclesRemainingInCurrentInterrupt--;
  switch (cyclesRemainingInCurrentInterrupt) {
    case 6:
      // dummy opcode read
      // reset registers
      a_register = 0;
      x_register = 0;
      y_register = 0;
      status = 0b00100000;
      sp = 0xFF;
      break;
    case 5:
      break;  // dummy operand read
    case 4:
      sp--;  // dummy stack push
      break;
    case 3:
      sp--;  // dummy stack push
      break;
    case 2:
      break;
    case 1:
      status &= ~FLAG_DECIMAL;   // clear D flag
      status |= FLAG_INTERRUPT;  // set the interrupt flag
      pc = bus->read(0xFFFC);
      break;
    case 0:
      pc |= (static_cast<uint16_t>(bus->read(0xFFFD)) << 8);
      cyclesRemainingInCurrentInterrupt = 7;  // reset for next interrupt
      initiatingInterrupt = false;
      break;
  }
}

/**
 * Updates the zero and negative flags based on a given result.
 *
 * @param result Zero flag is set if result is 0, negative flag is set if
 * MSB of result is 1.
 */
void CPU::updateZeroAndNegativeFlags(uint8_t result) {
  // zero flag is bit 1
  if (result == 0) {
    status |= FLAG_ZERO;  // set zero flag if result is 0
  } else {
    status &= ~FLAG_ZERO;  // else clear zero flag
  }

  // negative flag is bit 7
  if (result & 0b10000000) {
    status |= FLAG_NEGATIVE;  // set negative flag if bit 7 of result is 1
  } else {
    status &= ~FLAG_NEGATIVE;  // else clear negative flag
  }
}

void CPU::branch() {
  if (currAddrResCtx.state == ResolutionState::Init) {
    readOperand();
    currAddrResCtx.state = ResolutionState::ComputeAddress;
  } else if (currAddrResCtx.state == ResolutionState::ComputeAddress) {
    currAddrResCtx.address = pc + static_cast<int8_t>(currentOpBytes[1]);
    if (((pc & 0xFF00) != (currAddrResCtx.address & 0xFF00)) &&
        !currentOpCode->ignorePageCrossings) {
      // increment remaining cycles to emulate extra cycle
      cyclesRemainingInCurrentInstr++;
    }
    pc = currAddrResCtx.address;  // update pc
    currAddrResCtx.state = ResolutionState::Done;
  }
  // else ResolutionState::Done, do nothing (emulate cycle)
}

void CPU::op_ADC(uint16_t addr) {
  op_ADC_CORE(bus->read(addr));
  // all processing of adc_core is done in the same cycle as this read
}
void CPU::op_ADC_CORE(uint8_t operand) {
  // allows SBC to use ADC logic
  uint8_t carry = (status & 0x01);  // extract carry flag from status register
  uint16_t result = a_register + operand + carry;  // compute result

  // set carry flag (C) if result > 255
  if (result > 0xFF) {
    status |= FLAG_CARRY;
  } else {
    status &= ~FLAG_CARRY;  // else clear carry flag
  }

  // set overflow flag (V) if a signed overflow occurs (adding 2 positive or 2
  // negative numbers results in a different-signed result)
  bool overflow =
      (~(a_register ^ operand) & (a_register ^ result) & 0b10000000);
  if (overflow) {
    status |= FLAG_OVERFLOW;  // set overflow flag (bit 6)
  } else {
    status &= ~FLAG_OVERFLOW;  // clear overflow flag
  }

  a_register = static_cast<uint8_t>(result);  // store result in A reg

  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_AND(uint16_t addr) {
  a_register &= bus->read(addr);
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_ASL(uint16_t addr) {
  switch (cyclesRemainingInCurrentInstr) {
    case 3:
      readBuffer = bus->read(addr);
      break;
    case 2:
      // emulate dummy read cycle
      status = (status & ~FLAG_CARRY) | ((readBuffer & 0x80) ? 0x01 : 0);
      readBuffer <<= 1;  // shift value left
      break;
    case 1:
      bus->write(addr, readBuffer);
      updateZeroAndNegativeFlags(readBuffer);
      break;
  }
}
void CPU::op_ASL_ACC(uint16_t /* implied */) {
  // store bit 7 before shift in carry flag
  status = (status & ~FLAG_CARRY) | ((a_register & 0x80) ? 0x01 : 0);
  a_register <<= 1;  // shift accumulator left
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_BCC(uint16_t /* calculated iff branch taken */) {
  if (!(status & FLAG_CARRY)) {
    branch();  // branch if carry flag clear
  } else {
    // branch not taken, set remaining cycles to 1 (will be decremented to zero
    // immediately by tick function after this function exits)
    cyclesRemainingInCurrentInstr = 1;
  }
}
void CPU::op_BCS(uint16_t /* calculated iff branch taken */) {
  if (status & FLAG_CARRY) {
    branch();  // branch if carry flag set
  } else {
    // branch not taken, set remaining cycles to 1 (will be decremented to zero
    // immediately by tick function after this function exits)
    cyclesRemainingInCurrentInstr = 1;
  }
}
void CPU::op_BEQ(uint16_t /* calculated iff branch taken */) {
  if (status & FLAG_ZERO) {
    branch();  // branch if zero flag is set
  } else {
    // branch not taken, set remaining cycles to 1 (will be decremented to zero
    // immediately by tick function after this function exits)
    cyclesRemainingInCurrentInstr = 1;
  }
}
void CPU::op_BIT(uint16_t addr) {
  // - bits 7 and 6 of operand are transfered to bit 7 and 6 of SR (N,V);
  // - the zero-flag is set according to the result of the operand AND the
  // - accumulator (set, if the result is zero, unset otherwise).
  status &= ~(FLAG_NEGATIVE | FLAG_OVERFLOW | FLAG_ZERO);  // clear N,V,Z flags
  readBuffer = bus->read(addr);
  status |= readBuffer & (FLAG_NEGATIVE | FLAG_OVERFLOW);
  if ((readBuffer & a_register) == 0) {
    status |= FLAG_ZERO;
  }
}
void CPU::op_BMI(uint16_t /* calculated iff branch taken */) {
  if (status & FLAG_NEGATIVE) {
    branch();  // branch if negative flag is set
  } else {
    // branch not taken, set remaining cycles to 1 (will be decremented to zero
    // immediately by tick function after this function exits)
    cyclesRemainingInCurrentInstr = 1;
  }
}
void CPU::op_BNE(uint16_t /* calculated iff branch taken */) {
  if (!(status & FLAG_ZERO)) {
    branch();  // branch if zero flag is clear
  } else {
    // branch not taken, set remaining cycles to 1 (will be decremented to zero
    // immediately by tick function after this function exits)
    cyclesRemainingInCurrentInstr = 1;
  }
}
void CPU::op_BPL(uint16_t /* calculated iff branch taken */) {
  if (!(status & FLAG_NEGATIVE)) {
    branch();  // branch if negative flag is clear
  } else {
    // branch not taken, set remaining cycles to 1 (will be decremented to zero
    // immediately by tick function after this function exits)
    cyclesRemainingInCurrentInstr = 1;
  }
}
/**
 *  #  address R/W description
 * --- ------- --- -----------------------------------------------
 *  1    PC     R  fetch opcode, increment PC
 *  2    PC     R  read next instruction byte (and throw it away),
 *                 increment PC
 *  3  $0100,S  W  push PCH on stack, decrement S
 *  4  $0100,S  W  push PCL on stack, decrement S
 * *** ATP, the signal status determines which interrupt vector is used ***
 *  5  $0100,S  W  push P on stack (with B flag set), decrement S
 *  6   $FFFE   R  fetch PCL, set I flag
 *  7   $FFFF   R  fetch PCH
 */
void CPU::op_BRK(uint16_t /* implied */) {
  // cycle 1 already completed by tick function
  switch (cyclesRemainingInCurrentInstr) {
    case 6:
      bus->read(pc);  // operand dummy read
      pc++;
      break;
    case 5:
      push((pc >> 8) & 0xFF);  // push PCH
      break;
    case 4:
      push(pc & 0xFF);  // push PCL
      break;
    case 3:
      // push P on stack (with B flag set), decrement S
      push(status | FLAG_BREAK);
      break;
    case 2:
      // fetch PCL, set I flag
      pc = bus->read(0xFFFE);
      status |= FLAG_INTERRUPT;  // set the interrupt flag
      break;
    case 1:
      // fetch PCH
      pc |= (static_cast<uint16_t>(bus->read(0xFFFF)) << 8);
      break;
  }
}
void CPU::op_BVC(uint16_t /* calculated iff branch taken */) {
  if (!(status & FLAG_OVERFLOW)) {
    branch();  // branch if overflow flag is clear
  } else {
    // branch not taken, set remaining cycles to 1 (will be decremented to zero
    // immediately by tick function after this function exits)
    cyclesRemainingInCurrentInstr = 1;
  }
}
void CPU::op_BVS(uint16_t /* calculated iff branch taken */) {
  if (status & FLAG_OVERFLOW) {
    branch();  // branch if overflow flag is set
  } else {
    // branch not taken, set remaining cycles to 1 (will be decremented to zero
    // immediately by tick function after this function exits)
    cyclesRemainingInCurrentInstr = 1;
  }
}
void CPU::op_CLC(uint16_t /* implied */) { status &= ~FLAG_CARRY; }
void CPU::op_CLD(uint16_t /* implied */) { status &= ~FLAG_DECIMAL; }
void CPU::op_CLI(uint16_t /* implied */) { status &= ~FLAG_INTERRUPT; }
void CPU::op_CLV(uint16_t /* implied */) { status &= ~FLAG_OVERFLOW; }
void CPU::op_CMP(uint16_t addr) {
  // C set if A >= M
  // Z set if A == M
  // N set if A < M
  // clear negative, zero, and carry flags
  status &= ~(FLAG_NEGATIVE | FLAG_ZERO | FLAG_CARRY);
  readBuffer = bus->read(addr);
  uint8_t result = a_register - readBuffer;

  if (a_register == readBuffer) status |= FLAG_ZERO;   // set zero if Y == M
  if (a_register >= readBuffer) status |= FLAG_CARRY;  // set carry if Y >= M
  if (result & 0x80) status |= FLAG_NEGATIVE;  // set neg if result is negative
}
void CPU::op_CPX(uint16_t addr) {
  // C set if X >= M
  // Z set if X == M
  // N set if X < M
  // clear negative, zero, and carry flags
  status &= ~(FLAG_NEGATIVE | FLAG_ZERO | FLAG_CARRY);
  readBuffer = bus->read(addr);
  uint8_t result = x_register - readBuffer;

  if (x_register == readBuffer) status |= FLAG_ZERO;   // set zero if Y == M
  if (x_register >= readBuffer) status |= FLAG_CARRY;  // set carry if Y >= M
  if (result & 0x80) status |= FLAG_NEGATIVE;  // set neg if result is negative
}
void CPU::op_CPY(uint16_t addr) {
  // C set if Y >= M
  // Z set if Y == M
  // N set if Y < M
  // clear negative, zero, and carry flags
  status &= ~(FLAG_NEGATIVE | FLAG_ZERO | FLAG_CARRY);
  readBuffer = bus->read(addr);
  uint8_t result = y_register - readBuffer;

  if (y_register == readBuffer) status |= FLAG_ZERO;   // set zero if Y == M
  if (y_register >= readBuffer) status |= FLAG_CARRY;  // set carry if Y >= M
  if (result & 0x80) status |= FLAG_NEGATIVE;  // set neg if result is negative
}
void CPU::op_DEC(uint16_t addr) {
  switch (cyclesRemainingInCurrentInstr) {
    case 3:
      readBuffer = bus->read(addr);
      break;
    case 2:
      // emulate additional dummy read cycle
      readBuffer--;
      break;
    case 1:
      bus->write(addr, readBuffer);
      updateZeroAndNegativeFlags(readBuffer);
      break;
  }
}
void CPU::op_DEX(uint16_t /* implied */) {
  x_register--;
  updateZeroAndNegativeFlags(x_register);
}
void CPU::op_DEY(uint16_t /* implied */) {
  y_register--;
  updateZeroAndNegativeFlags(y_register);
}
void CPU::op_EOR(uint16_t addr) {
  a_register ^= bus->read(addr);
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_INC(uint16_t addr) {
  switch (cyclesRemainingInCurrentInstr) {
    case 3:
      readBuffer = bus->read(addr);
      break;
    case 2:
      readBuffer++;
      break;
    case 1:
      bus->write(addr, readBuffer);
      updateZeroAndNegativeFlags(readBuffer);
      break;
  }
}
void CPU::op_INX(uint16_t /* implied */) {
  x_register++;
  updateZeroAndNegativeFlags(x_register);
}
void CPU::op_INY(uint16_t /* implied */) {
  y_register++;
  updateZeroAndNegativeFlags(y_register);
}
void CPU::op_JMP(uint16_t addr) { pc = addr; }
void CPU::op_JSR(uint16_t addr) {
  switch (cyclesRemainingInCurrentInstr) {
    case 3:
      pc++;  // pc + 1 = the address minus one of the next instruction
      push((pc >> 8) & 0xFF);  // push PCH
      break;
    case 2:
      push(pc & 0xFF);  // push LSB
    case 1:
      pc = addr;  // error point: no idea why this requires an extra cycle
  }
}
void CPU::op_LDA(uint16_t addr) {
  a_register = bus->read(addr);
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_LDX(uint16_t addr) {
  x_register = bus->read(addr);
  ;
  updateZeroAndNegativeFlags(x_register);
}
void CPU::op_LDY(uint16_t addr) {
  y_register = bus->read(addr);
  updateZeroAndNegativeFlags(y_register);
}
void CPU::op_LSR(uint16_t addr) {
  switch (cyclesRemainingInCurrentInstr) {
    case 3:
      readBuffer = bus->read(addr);
      break;
    case 2:
      // on actual hardware, dummy write unshifted value in this cycle
      // bus->write(addr, readBuffer);
      status = (status & ~FLAG_CARRY) | ((readBuffer & 0x01) ? FLAG_CARRY : 0);
      readBuffer >>= 1;  // shift value right
      break;
    case 1:
      bus->write(addr, readBuffer);  // write new value back to memory
      updateZeroAndNegativeFlags(readBuffer);
      break;
  }
}
void CPU::op_LSR_ACC(uint16_t /* implied */) {
  // store bit 0 before shift in carry flag
  status = (status & ~FLAG_CARRY) | ((a_register & 0x01) ? FLAG_CARRY : 0);
  a_register >>= 1;  // shift A register right
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_NOP(uint16_t /* implied */) { return; }
void CPU::op_ORA(uint16_t addr) {
  a_register |= bus->read(addr);
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_PHA(uint16_t /* implied */) {
  if (cyclesRemainingInCurrentInstr == 2)
    return;  // dummy read to pc happens here
  else
    push(a_register);
}
void CPU::op_PHP(uint16_t /* implied */) {
  if (cyclesRemainingInCurrentInstr == 2)
    return;  // dummy read to pc happens here
  else
    push(status | FLAG_BREAK | FLAG_CONSTANT);
}
void CPU::op_PLA(uint16_t /* implied */) {
  switch (cyclesRemainingInCurrentInstr) {
    case 3:
      // dummy read to pc
      break;
    case 2:
      // dummy read to (0x100 + sp - 1)
      break;
    case 1:
      a_register = pop();
      updateZeroAndNegativeFlags(a_register);
  }
}
void CPU::op_PLP(uint16_t /* implied */) {
  switch (cyclesRemainingInCurrentInstr) {
    case 3:
      // dummy read to pc
      break;
    case 2:
      // dummy read to (0x100 + sp - 1)
      break;
    case 1:
      status = (pop() | FLAG_CONSTANT) & ~FLAG_BREAK;
      updateZeroAndNegativeFlags(a_register);
  }
}
void CPU::op_ROL(uint16_t addr) {
  switch (cyclesRemainingInCurrentInstr) {
    case 3: {
      readBuffer = bus->read(addr);
      break;
    }
    case 2: {
      // dummy write same value back to addr here
      uint8_t result = (readBuffer << 1) | (status & FLAG_CARRY ? 1 : 0);
      if (readBuffer & 0x80) {
        status |= FLAG_CARRY;  // bit 7 of value is set, set carry flag
      } else {
        status &= ~FLAG_CARRY;  // else clear carry flag
      }
      readBuffer = result;
      break;
    }
    case 1: {
      bus->write(addr, readBuffer);
      updateZeroAndNegativeFlags(readBuffer);
      break;
    }
  }
}
void CPU::op_ROL_ACC(uint16_t /* implied */) {
  // shift accumulator left and set LSB to carry bit
  uint8_t result = (a_register << 1) | (status & FLAG_CARRY ? 1 : 0);

  if (a_register & 0x80) {
    status |= FLAG_CARRY;  // bit 7 of value is set, set carry flag
  } else {
    status &= ~FLAG_CARRY;  // else clear carry flag
  }

  a_register = result;
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_ROR(uint16_t addr) {
  switch (cyclesRemainingInCurrentInstr) {
    case 3: {
      readBuffer = bus->read(addr);
      break;
    }
    case 2: {
      // dummy write same value back to addr here
      uint8_t result = (readBuffer >> 1) | (status & FLAG_CARRY ? 0x80 : 0);
      if (readBuffer & 0x01) {
        status |= FLAG_CARRY;  // bit 0 of value is set, set carry flag
      } else {
        status &= ~FLAG_CARRY;  // else clear carry flag
      }
      readBuffer = result;
      break;
    }
    case 1: {
      bus->write(addr, readBuffer);
      updateZeroAndNegativeFlags(readBuffer);
      break;
    }
  }
}
void CPU::op_ROR_ACC(uint16_t /* implied */) {
  // shift accumulator right and set MSB to carry bit
  uint8_t result = (a_register >> 1) | (status & FLAG_CARRY ? 0x80 : 0);

  if (a_register & 0x01) {
    status |= FLAG_CARRY;  // bit 0 of value is set, set carry flag
  } else {
    status &= ~FLAG_CARRY;  // else clear carry flag
  }

  a_register = result;
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_RTI(uint16_t /* implied */) {
  switch (cyclesRemainingInCurrentInstr) {
    case 5:
      activeInterrupt = Interrupt::NONE;
      // dummy read to operand
      break;
    case 4:
      // dummy read to 0x100 + sp - 1
      break;
    case 3:
      status = (pop() | FLAG_CONSTANT) & ~FLAG_BREAK;
      break;
    case 2:
      pc = pop();
      break;
    case 1:
      pc |= static_cast<uint16_t>(pop()) << 8;
      // error point: may need to inc pc?
      break;
  }
}
void CPU::op_RTS(uint16_t /* implied */) {
  switch (cyclesRemainingInCurrentInstr) {
    case 5:
      // dummy read to operand
      break;
    case 4:
      // dummy read to 0x100 + sp - 1
      break;
    case 3:
      // same as RTI but status reg not set
      break;
    case 2:
      pc = pop();
      break;
    case 1:
      pc |= static_cast<uint16_t>(pop()) << 8;
      // error point: may need to inc pc?
      break;
  }
}
void CPU::op_SBC(uint16_t addr) {
  // SBC:
  // A = A – M – (1 – C)
  //   = A + (~M) + C
  op_ADC_CORE(~(bus->read(addr)));
  // op_ADC_CORE is only one cycle so this is fine,
  // the read is not duplicated
}
void CPU::op_SEC(uint16_t /* implied */) { status |= FLAG_CARRY; }
void CPU::op_SED(uint16_t /* implied */) { status |= FLAG_DECIMAL; }
void CPU::op_SEI(uint16_t /* implied */) { status |= FLAG_INTERRUPT; }
void CPU::op_STA(uint16_t addr) { bus->write(addr, a_register); }
void CPU::op_STX(uint16_t addr) { bus->write(addr, x_register); }
void CPU::op_STY(uint16_t addr) { bus->write(addr, y_register); }
void CPU::op_TAX(uint16_t /* implied */) {
  x_register = a_register;
  updateZeroAndNegativeFlags(x_register);
}
void CPU::op_TAY(uint16_t /* implied */) {
  y_register = a_register;
  updateZeroAndNegativeFlags(y_register);
}
void CPU::op_TSX(uint16_t /* implied */) {
  x_register = sp;
  updateZeroAndNegativeFlags(x_register);
}
void CPU::op_TXA(uint16_t /* implied */) {
  a_register = x_register;
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_TXS(uint16_t /* implied */) { sp = x_register; }
void CPU::op_TYA(uint16_t /* implied */) {
  a_register = y_register;
  updateZeroAndNegativeFlags(a_register);
}

// =====================================================
// UNDOCUMENTED/ILLEGAL INSTRUCTIONS
// =====================================================
// https://www.masswerk.at/nowgobang/2021/6502-illegal-opcodes (mnemonics)
// http://www.ffd2.com/fridge/docs/6502-NMOS.extra.opcodes (descriptions)
// https://www.oxyron.de/html/opcodes02.html (status register impact)

// void CPU::opi_ALR(uint16_t addr) {
//   /* ANDs the contents of the A register with an immediate value and then
//   LSRs
//    * the result. */
//   op_AND(addr);
//   op_LSR_ACC(0);  // implied addressing
// }
// void CPU::opi_ANC(uint16_t addr) {
//   /* ANC ANDs the contents of the A register with an immediate value and then
//    * moves bit 7 of A into the Carry flag.  This opcode works basically
//    * identically to AND #immed. except that the Carry flag is set to the same
//    * state that the Negative flag is set to. */
//   op_AND(addr);
//   if (status & FLAG_NEGATIVE) {
//     status |= FLAG_CARRY;
//   } else {
//     status &= ~FLAG_CARRY;
//   }
// }
// void CPU::opi_ANE(uint16_t addr) {
//   /* aka XAA: transfers the contents of the X register to the A register and
//    * then ANDs the A register with an immediate value. Highly unstable. */
//   op_TXA(0);
//   op_AND(addr);
// }
// void CPU::opi_ARR(uint16_t addr) {
//   /* ANDs the contents of the A register with an immediate value and then
//   RORs
//    * the result. The carry flag is set to the value of bit 6 of the result.
//         •	The overflow flag is set to the XOR of bits 6 and 5 of the
//    result.*/
//   op_AND(addr);
//   op_ROR_ACC(0);

//   if (a_register & 0x40) {
//     status |= FLAG_CARRY;
//   } else {
//     status &= ~FLAG_CARRY;
//   }

//   bool newOverflow = (((a_register >> 6) & 1) ^ ((a_register >> 5) & 1)) !=
//   0; if (newOverflow) {
//     status |= FLAG_OVERFLOW;
//   } else {
//     status &= ~FLAG_OVERFLOW;
//   }
// }
// void CPU::opi_DCP(uint16_t addr) {
//   /* aka DCM: DECs the contents of a memory location and then CMPs the result
//    * with the A register. */
//   op_DEC(addr);
//   op_CMP(addr);
// }
// void CPU::opi_ISC(uint16_t addr) {
//   /* aka INS: INCs the contents of a memory location and then SBCs the result
//    * from the A register.*/
//   op_INC(addr);
//   op_SBC(addr);
// }
// void CPU::opi_LAS(uint16_t addr) {
//   /* ANDs the contents of a memory location with the contents of the stack
//    * pointer register and stores the result in the accumulator, the X
//    * register, and the stack pointer.  Affected flags: N Z.*/
//   sp &= memRead8(addr);
//   a_register = sp;
//   x_register = sp;
//   updateZeroAndNegativeFlags(sp);
// }
// void CPU::opi_LAX(uint16_t addr) {
//   /* This opcode loads both the accumulator and the X register with the
//    * contents of a memory location. */
//   op_LDA(addr);
//   op_LDX(addr);
// }
// void CPU::opi_LXA(uint16_t addr) {
//   /* aka OAL: ORs the A register with #$EE, ANDs the result with an immediate
//    * value, and then stores the result in both A and X. Highly unstable. */
//   a_register |= 0xEE;
//   op_AND(addr);
//   op_TAX(0);
// }
// void CPU::opi_RLA(uint16_t addr) {
//   /* ROLs the contents of a memory location and then ANDs the result with the
//    * accumulator. */
//   op_ROL(addr);
//   op_AND(addr);
// }
// void CPU::opi_RRA(uint16_t addr) {
//   /* RORs the contents of a memory location and then ADCs the result with the
//    * accumulator. */
//   op_ROR(addr);
//   op_ADC(addr);
// }
// void CPU::opi_SAX(uint16_t addr) {
//   /* aka AXS+AAX: ANDs the contents of the A and X registers (without
//   changing
//    * the contents of either register) and stores the result in memory. Does
//    * not affect any flags in the processor status register.*/
//   memWrite8(addr, a_register & x_register);
// }
// void CPU::opi_SBX(uint16_t addr) {
//   /* aka AXS+SAX: ANDs the contents of the A and X registers (leaving the
//    * contents of A intact), subtracts an immediate value, and then stores the
//    * result in X. A few points might be made about the action of subtracting
//    * an immediate value. It actually works just like the CMP instruction,
//    * except that CMP does not store the result of the subtraction it performs
//    * in any register. This subtract operation is not affected by the state of
//    * the Carry flag, though it does affect the Carry flag. It does not affect
//    * the Overflow flag. */
//   x_register = (a_register & x_register) - memRead8(addr);
//   // set carry flag (C) if result > 255
//   if (x_register > 0xFF) {
//     status |= FLAG_CARRY;
//   } else {
//     status &= ~FLAG_CARRY;  // else clear carry flag
//   }
//   updateZeroAndNegativeFlags(x_register);
// }
// void CPU::opi_SHA(uint16_t addr) {
//   /* Stores A AND X AND (high-byte of addr. + 1) at addr. Unstable. */
//   uint8_t high_plus_one = currentHighByte + 1;
//   memWrite8(addr, (a_register & x_register) & high_plus_one);
// }
// void CPU::opi_SHX(uint16_t addr) {
//   /* aka A11,SXA,XAS: Stores X AND (high-byte of addr. + 1) at addr.
//   Unstable.
//    */
//   uint8_t high_plus_one = currentHighByte + 1;
//   memWrite8(addr, x_register & high_plus_one);
// }
// void CPU::opi_SHY(uint16_t addr) {
//   /* aka SAY: Stores Y AND (high-byte of addr. + 1) at addr. Unstable. */
//   uint8_t high_plus_one = currentHighByte + 1;
//   memWrite8(addr, y_register & high_plus_one);
// }
// void CPU::opi_SLO(uint16_t addr) {
//   /* This opcode ASLs the contents of a memory location and then ORs the
//   result with the accumulator. */
//   op_ASL(addr);
//   op_ORA(addr);
// }
// void CPU::opi_SRE(uint16_t addr) {
//   /* aka LSE: LSRs the contents of a memory location and then EORs the result
//    * with the accumulator. */
//   op_LSR(addr);
//   op_EOR(addr);
// }
// void CPU::opi_TAS(uint16_t addr) {
//   /* ANDs the contents of the A and X registers (without changing the
//   contents
//    * of either register) and transfers the result to the stack pointer. It
//    * then ANDs that result with the contents of the high byte of the target
//    * address of the operand +1 and stores that final result in memory. */
//   uint8_t high_plus_one = currentHighByte + 1;
//   sp = a_register & x_register;
//   memWrite8(addr, sp & high_plus_one);
// }
// void CPU::opi_SBC(uint16_t addr) { op_SBC(addr); }
// void CPU::opi_NOP(uint16_t addr) { return; }
// void CPU::opi_KIL(uint16_t addr) { executionActive = false; }
