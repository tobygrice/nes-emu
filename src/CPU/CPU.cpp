#include "../../include/CPU/CPU.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../../include/BusInterface.h"
#include "../../include/CPU/OpCode.h"

// https://github.com/SingleStepTests/65x02/tree/main/nes6502

/**
 * Constructor to initialise registers with starting values.
 * https://www.nesdev.org/wiki/CPU_power_up_state
 */
CPU::CPU(BusInterface* bus, Logger* logger)
    : a_register(0),       // accumulator starts at 0
      x_register(0),       // X starts at 0
      y_register(0),       // Y starts at 0
      status(0b00100000),  // status register starts with all flags clear
      pc(0x8000),          // cartridge ROM is 0x8000-0xFFFF in NES
      sp(0xFD),            // stack pointer starts at 0xFD (error point 0xFF?)
      bus(bus),            // shared bus object
      logger(logger) {}    // log class

/**
 * Fetches value at given address.
 *
 * @param addr The address from which to retrieve data.
 * @return Single byte of memory at provided addr.
 */
uint8_t CPU::memRead8(uint16_t addr) { return bus->read(addr); }

/**
 * Writes given data at given address.
 *
 * @param addr The address to write the data.
 * @param data The data to be written.
 */
void CPU::memWrite8(uint16_t addr, uint8_t data) { bus->write(addr, data); }

/**
 * Reads 2 bytes of data from the given address, accounting for little endian.
 *
 * @param addr The address from which to retrieve data.
 * @return Two bytes of memory at provided addr.
 */
uint16_t CPU::memRead16(uint16_t addr) {
  uint16_t low = memRead8(addr);       // read low byte
  uint16_t high = memRead8(addr + 1);  // read high byte
  return (high << 8) | low;            // combine bytes (little-endian)
}

/**
 * Writes 2 bytes of data at the given address.
 *
 * @param addr The address to write the data.
 * @param data The 16 bits of data to be written.
 */
void CPU::memWrite16(uint16_t addr, uint16_t data) {
  uint8_t low = data & 0xFF;          // extract low byte
  uint8_t high = (data >> 8) & 0xFF;  // extract high byte
  memWrite8(addr, low);               // write low byte first
  memWrite8(addr + 1, high);          // write high byte second
}

/**
 * Loads provided program into memory, then executes it.
 *
 * @param program The program instruction set.
 */
void CPU::loadAndExecute(const std::vector<uint8_t>& program) {
  loadProgram(program);  // Load program into memory
  resetInterrupt();      // Reset CPU (sets PC to reset vector)
  executeProgram();      // Start execution
}

/**
 * Loads provided program into memory.
 *
 * @param program The program instruction set.
 */
void CPU::loadProgram(const std::vector<uint8_t>& program) {
  for (size_t i = 0; i < program.size(); i++) {
    memWrite8(pc + i, program[i]);
  }

  // Set the reset vector at 0xFFFC to point to the start address (pc).
  memWrite16(0xFFFC, pc);
}

/**
 * NES has a mechanism to indicate where the CPU should start execution.
 * When a new cartridge is inserted, the CPU receives a "Reset interrupt"
 * signal, which instructs it to:
 * - Reset the state (registers and flags)
 * - Set pc to the 16-bit address stored at 0xFFFC
 */
void CPU::resetInterrupt() {
  a_register = 0;
  x_register = 0;
  y_register = 0;
  status &= ~FLAG_DECIMAL;   // clear D flag
  status |= FLAG_INTERRUPT;  // set interrupt flag
  pc = memRead16(0xFFFC);
}

void CPU::executeProgram() {
  executionActive = true;
  std::deque<uint16_t> lastPCs;
  const int maxHistory = 8;    // only remember the last 8 PCs
  const int hitThreshold = 4;  // if the current PC appears this many times in
                               // the history, assume infinite loop

  while (executionActive) {
    // Record the current PC in our history.
    lastPCs.push_back(pc);
    if (lastPCs.size() > maxHistory) {
      lastPCs.pop_front();  // remove the oldest PC to maintain the size
    }

    // Count how many times the current PC appears in the last 10.
    int count = 0;
    for (auto p : lastPCs) {
      if (p == pc) {
        count++;
      }
    }

    if (count >= hitThreshold) {
      std::cerr << "Infinite loop detected at PC 0x" << std::uppercase
                << std::hex << pc << std::dec << " (" << count
                << " hits in the last " << maxHistory
                << " instructions). Exiting execution." << std::endl;
      break;
    }

    uint8_t instructionCycles = executeInstruction();
    bus->tick(instructionCycles);
  }
}

uint8_t CPU::executeInstruction() {
  // 1) capture PC for logging
  uint16_t initPC = pc;

  // 2) fetch opcode from pc
  uint8_t opcode = memRead8(pc);

  // 3) lookup opcode table entry
  const OpCode* op = getOpCode(opcode);  // look up opcode

  // 4) read operand bytes for logger
  std::vector<uint8_t> opBytes;
  for (int i = 0; i < op->bytes; i++) {
    opBytes.push_back(memRead8(pc + i));
  }
  // save current high byte for SHA, SHX, SHY, and TAS
  currentHighByte = opBytes.back();  // little endian

  // 5) increment PC so that getOperandAddress() sees the operand bytes at pc
  pc++;

  // 6) compute final address
  AddressResolveInfo addressInfo =
      getOperandAddress(op->mode, op->ignorePageCrossings);

  // 7) log instruction (nestest logs state before execution)
  uint8_t valueAtFinalAddr = memRead8(addressInfo.address);
  logger->log(initPC, op, &opBytes, &addressInfo, valueAtFinalAddr, a_register,
              x_register, y_register, status, sp,
              0,                    // ppu X
              0,                    // ppu Y
              bus->getCycleCount()  // CPU cycle
  );

  // 8) execute instruction
  (this->*(op->handler))(addressInfo.address);

  // 10) advance PC and compute total cycles
  uint8_t totalCycles = op->cycles;

  if (!pcModified) {
    pc += (op->bytes - 1);  // advance pc past operand bytes

    // DO NOT add extra cycle for page crossed if instruction was a
    // branching/subroutine instruction and the branch wasn't taken
    if (op->mode != AddressingMode::Relative) {
      if (addressInfo.pageCrossed) totalCycles++;
    }
  } else {
    // branch/subroutine taken
    pcModified = false;
    totalCycles++;
    if (addressInfo.pageCrossed) totalCycles++;
  }

  return totalCycles;
}

/**
 * Function to return the address of the operand given the addressing mode.
 *
 * @param mode The addressing mode specified by the opcode
 * @return A 16-bit memory address
 */
AddressResolveInfo CPU::getOperandAddress(AddressingMode mode,
                                          bool ignorePageCrossings) {
  AddressResolveInfo info;
  info.address = 0;
  info.pointerAddress = 0;
  info.pointerUsed = false;
  info.pageCrossed = false;

  switch (mode) {
    case AddressingMode::Implied:
    case AddressingMode::Acc:
      // accumulator and implicit opcodes do not require an address
      info.address = 0;
      break;
    case AddressingMode::Relative:
      // error point: uint8_t?
      info.address = pc + 1 + static_cast<int8_t>(memRead8(pc));
      if ((((pc + 1) & 0xFF00) != (info.address & 0xFF00)) &&
          !ignorePageCrossings) {
        info.pageCrossed = true;
      }
      break;
    case AddressingMode::Immediate:
      info.address = pc;
      break;
    case AddressingMode::ZeroPage:
      info.address = static_cast<uint16_t>(memRead8(pc));
      break;
    case AddressingMode::Absolute:
      info.address = memRead16(pc);
      break;
    case AddressingMode::ZeroPageX: {
      // wrap-around addition on 8-bit values:
      uint8_t addr = static_cast<uint8_t>(memRead8(pc) + x_register);
      info.address = static_cast<uint16_t>(addr);
      break;
    }
    case AddressingMode::ZeroPageY: {
      uint8_t addr = static_cast<uint8_t>(memRead8(pc) + y_register);
      info.address = static_cast<uint16_t>(addr);
      break;
    }
    case AddressingMode::AbsoluteX: {
      uint16_t base = memRead16(pc);
      info.address = base + x_register;
      if (((base & 0xFF00) != (info.address & 0xFF00)) &&
          !ignorePageCrossings) {
        // MSB of base address and resulting addition address is different
        info.pageCrossed = true;  // +1 cycle for page crossed
      }
      break;
    }
    case AddressingMode::AbsoluteY: {
      uint16_t base = memRead16(pc);
      info.address = base + y_register;
      if (((base & 0xFF00) != (info.address & 0xFF00)) &&
          !ignorePageCrossings) {
        // MSB of base address and resulting addition address is different
        info.pageCrossed = true;  // +1 cycle for page crossed
      }
      break;
    }
    case AddressingMode::Indirect: {
      // For JMP ($xxxx): the 16-bit pointer is fetched from PC.
      uint16_t pointer = memRead16(pc);
      uint8_t lsb = memRead8(pointer);
      uint8_t msb;
      /* https://www.nesdev.org/obelisk-6502-guide/reference.html#JMP
         "An original 6502 has does not correctly fetch the target address if
         the indirect vector falls on a page boundary (e.g. $xxFF where xx is
         any value from $00 to $FF). In this case fetches the LSB from $xxFF as
         expected but takes the MSB from $xx00. This is fixed in some later
         chips like the 65SC02 so for compatibility always ensure the indirect
         vector is not at the end of the page." */
      if ((pointer & 0x00FF) == 0x00FF) {
        // emulate bug - wrap to beginning of page
        msb = memRead8(pointer & 0xFF00);
      } else {
        msb = memRead8(pointer + 1);
      }
      info.address = (static_cast<uint16_t>(msb) << 8) | lsb;
      break;
    }

    case AddressingMode::IndirectX: {
      uint8_t ptr = memRead8(pc) + x_register;  // wrapping add on 8-bit
      info.pointerUsed = true;
      info.pointerAddress = static_cast<uint16_t>(ptr);

      uint8_t low = memRead8(info.pointerAddress);
      uint8_t high =
          memRead8(static_cast<uint16_t>(static_cast<uint8_t>(ptr + 1)));
      info.address =
          (static_cast<uint16_t>(high) << 8) | static_cast<uint16_t>(low);
      break;
    }

    case AddressingMode::IndirectY: {
      uint8_t base = memRead8(pc);
      uint8_t low = memRead8(static_cast<uint16_t>(base));
      uint8_t high = memRead8(static_cast<uint8_t>(base + 1));
      uint16_t deref_base = (static_cast<uint16_t>(high) << 8) | low;
      info.address = deref_base + y_register;
      info.pointerAddress = deref_base;
      info.pointerUsed = true;
      if (((deref_base & 0xFF00) != (info.address & 0xFF00)) &&
          !ignorePageCrossings) {
        // MSB of base address and resulting addition address is different
        info.pageCrossed = true;  // +1 cycle for page crossed
      }
      break;
    }
    default:
      throw std::runtime_error("Addressing mode not supported");
  }
  return info;
}

void CPU::push(uint8_t value) {
  memWrite8(0x0100 + sp, value);
  sp--;
}
uint8_t CPU::pop() {
  sp++;
  return memRead8(0x100 + sp);
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
void CPU::branch(uint16_t addr) {
  // relative address was already computed by getOperandAddress, set pc to addr
  pc = addr;          // update pc
  pcModified = true;  // set flag for execution loop
}

void CPU::op_ADC(uint16_t addr) { op_ADC_CORE(memRead8(addr)); }
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
  uint8_t value = memRead8(addr);
  a_register &= value;
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_ASL(uint16_t addr) {
  uint8_t value = memRead8(addr);
  // store bit 7 before shift in carry flag
  status = (status & ~FLAG_CARRY) | ((value & 0x80) ? 0x01 : 0);
  value <<= 1;             // shift value left
  memWrite8(addr, value);  // write new value back to memory
  updateZeroAndNegativeFlags(value);
}
void CPU::op_ASL_ACC(uint16_t /* implied */) {
  // store bit 7 before shift in carry flag
  status = (status & ~FLAG_CARRY) | ((a_register & 0x80) ? 0x01 : 0);
  a_register <<= 1;  // shift accumulator left
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_BCC(uint16_t addr) {
  if (!(status & FLAG_CARRY)) {
    branch(addr);  // branch if carry flag clear
  }
}
void CPU::op_BCS(uint16_t addr) {
  if (status & FLAG_CARRY) {
    branch(addr);  // branch if carry flag set
  }
}
void CPU::op_BEQ(uint16_t addr) {
  if (status & FLAG_ZERO) {
    branch(addr);  // branch if zero flag is set
  }
}
void CPU::op_BIT(uint16_t addr) {
  // - bits 7 and 6 of operand are transfered to bit 7 and 6 of SR (N,V);
  // - the zero-flag is set according to the result of the operand AND the
  // - accumulator (set, if the result is zero, unset otherwise).
  status &= ~(FLAG_NEGATIVE | FLAG_OVERFLOW | FLAG_ZERO);  // clear N,V,Z flags
  uint8_t value = memRead8(addr);
  status |= value & (FLAG_NEGATIVE | FLAG_OVERFLOW);
  if ((value & a_register) == 0) {
    status |= FLAG_ZERO;
  }
}
void CPU::op_BMI(uint16_t addr) {
  if (status & FLAG_NEGATIVE) {
    branch(addr);  // branch if negative flag is set
  }
}
void CPU::op_BNE(uint16_t addr) {
  if (!(status & FLAG_ZERO)) {
    branch(addr);  // branch if zero flag is clear
  }
}
void CPU::op_BPL(uint16_t addr) {
  if (!(status & FLAG_NEGATIVE)) {
    branch(addr);  // branch if negative flag is clear
  }
}
void CPU::op_BRK(uint16_t /* implied */) {
  pc++;  // provide an extra byte of spacing for a break mark

  // push high byte first, then low byte
  push((pc >> 8) & 0xFF);  // push MSB
  push(pc & 0xFF);         // push LSB

  // push the status register with the break flag set.
  push(status | FLAG_BREAK);

  status |= FLAG_INTERRUPT;  // set the interrupt flag

  // fetch the new pc from the interrupt vector
  pc = memRead16(0xFFFE);
  pcModified = true;  // set flag for execution loop

  executionActive = false;  // for now, terminate on BRK
}
void CPU::op_BVC(uint16_t addr) {
  if (!(status & FLAG_OVERFLOW)) {
    branch(addr);  // branch if overflow flag is clear
  }
}
void CPU::op_BVS(uint16_t addr) {
  if (status & FLAG_OVERFLOW) {
    branch(addr);  // branch if overflow flag is set
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
  uint8_t value = memRead8(addr);
  uint8_t result = a_register - value;

  if (a_register == value) status |= FLAG_ZERO;   // set zero if Y == M
  if (a_register >= value) status |= FLAG_CARRY;  // set carry if Y >= M
  if (result & 0x80) status |= FLAG_NEGATIVE;  // set neg if result is negative
}
void CPU::op_CPX(uint16_t addr) {
  // C set if X >= M
  // Z set if X == M
  // N set if X < M
  // clear negative, zero, and carry flags
  status &= ~(FLAG_NEGATIVE | FLAG_ZERO | FLAG_CARRY);
  uint8_t value = memRead8(addr);
  uint8_t result = x_register - value;

  if (x_register == value) status |= FLAG_ZERO;   // set zero if Y == M
  if (x_register >= value) status |= FLAG_CARRY;  // set carry if Y >= M
  if (result & 0x80) status |= FLAG_NEGATIVE;  // set neg if result is negative
}
void CPU::op_CPY(uint16_t addr) {
  // C set if Y >= M
  // Z set if Y == M
  // N set if Y < M
  // clear negative, zero, and carry flags
  status &= ~(FLAG_NEGATIVE | FLAG_ZERO | FLAG_CARRY);
  uint8_t value = memRead8(addr);
  uint8_t result = y_register - value;

  if (y_register == value) status |= FLAG_ZERO;   // set zero if Y == M
  if (y_register >= value) status |= FLAG_CARRY;  // set carry if Y >= M
  if (result & 0x80) status |= FLAG_NEGATIVE;  // set neg if result is negative
}
void CPU::op_DEC(uint16_t addr) {
  uint8_t value = memRead8(addr);
  value--;
  memWrite8(addr, value);
  updateZeroAndNegativeFlags(value);
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
  a_register ^= memRead8(addr);
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_INC(uint16_t addr) {
  uint8_t mem = memRead8(addr);
  mem++;
  memWrite8(addr, mem);
  updateZeroAndNegativeFlags(mem);
}
void CPU::op_INX(uint16_t /* implied */) {
  x_register++;
  updateZeroAndNegativeFlags(x_register);
}
void CPU::op_INY(uint16_t /* implied */) {
  y_register++;
  updateZeroAndNegativeFlags(y_register);
}
void CPU::op_JMP(uint16_t addr) {
  pc = addr;
  pcModified = true;  // set flag for execution loop
}
void CPU::op_JSR(uint16_t addr) {
  pc++;  // pc + 1 = the address minus one of the next instruction

  // push high byte first, then low byte
  push((pc >> 8) & 0xFF);  // push MSB
  push(pc & 0xFF);         // push LSB

  pc = addr;
  pcModified = true;  // set flag for execution loop
}
void CPU::op_LDA(uint16_t addr) {
  uint8_t value = memRead8(addr);
  a_register = value;
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_LDX(uint16_t addr) {
  uint8_t value = memRead8(addr);
  x_register = value;
  updateZeroAndNegativeFlags(x_register);
}
void CPU::op_LDY(uint16_t addr) {
  uint8_t value = memRead8(addr);
  y_register = value;
  updateZeroAndNegativeFlags(y_register);
}
void CPU::op_LSR(uint16_t addr) {
  uint8_t value = memRead8(addr);
  // store bit 0 before shift in carry flag
  status = (status & ~FLAG_CARRY) | ((value & 0x01) ? FLAG_CARRY : 0);
  value >>= 1;             // shift value right
  memWrite8(addr, value);  // write new value back to memory
  updateZeroAndNegativeFlags(value);
}
void CPU::op_LSR_ACC(uint16_t /* implied */) {
  // store bit 0 before shift in carry flag
  status = (status & ~FLAG_CARRY) | ((a_register & 0x01) ? FLAG_CARRY : 0);
  a_register >>= 1;  // shift A register right
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_NOP(uint16_t /* implied */) { return; }
void CPU::op_ORA(uint16_t addr) {
  a_register |= memRead8(addr);
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_PHA(uint16_t /* implied */) { push(a_register); }
void CPU::op_PHP(uint16_t /* implied */) {
  push(status | FLAG_BREAK | FLAG_CONSTANT);
}
void CPU::op_PLA(uint16_t /* implied */) {
  a_register = pop();
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_PLP(uint16_t /* implied */) {
  status = (pop() | FLAG_CONSTANT) & ~FLAG_BREAK;
}
void CPU::op_ROL(uint16_t addr) {
  uint8_t value = memRead8(addr);
  // shift value left and set LSB to carry bit
  uint8_t result = (value << 1) | (status & FLAG_CARRY ? 1 : 0);

  if (value & 0x80) {
    status |= FLAG_CARRY;  // bit 7 of value is set, set carry flag
  } else {
    status &= ~FLAG_CARRY;  // else clear carry flag
  }

  memWrite8(addr, result);
  updateZeroAndNegativeFlags(result);
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
  uint8_t value = memRead8(addr);
  // shift value right and set MSB to carry bit
  uint8_t result = (value >> 1) | (status & FLAG_CARRY ? 0x80 : 0);

  if (value & 0x01) {
    status |= FLAG_CARRY;  // bit 0 of value is set, set carry flag
  } else {
    status &= ~FLAG_CARRY;  // else clear carry flag
  }

  memWrite8(addr, result);
  updateZeroAndNegativeFlags(result);
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
  status = (pop() | FLAG_CONSTANT) & ~FLAG_BREAK;
  // op_RTS(0);  // error point: may be incorrect to add 1 to PC
  uint8_t pc_l = pop();
  uint8_t pc_h = pop();
  pc = ((pc_h << 8) | pc_l);
  pcModified = true;  // set flag for execution loop
}
void CPU::op_RTS(uint16_t /* implied */) {
  uint8_t pc_l = pop();
  uint8_t pc_h = pop();
  pc = ((pc_h << 8) | pc_l) + 1;
  pcModified = true;  // set flag for execution loop
}
void CPU::op_SBC(uint16_t addr) {
  // SBC:
  // A = A – M – (1 – C)
  //   = A + (~M) + C
  op_ADC_CORE(~memRead8(addr));
}
void CPU::op_SEC(uint16_t /* implied */) { status |= FLAG_CARRY; }
void CPU::op_SED(uint16_t /* implied */) { status |= FLAG_DECIMAL; }
void CPU::op_SEI(uint16_t /* implied */) { status |= FLAG_INTERRUPT; }
void CPU::op_STA(uint16_t addr) { memWrite8(addr, a_register); }
void CPU::op_STX(uint16_t addr) { memWrite8(addr, x_register); }
void CPU::op_STY(uint16_t addr) { memWrite8(addr, y_register); }
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

void CPU::opi_ALR(uint16_t addr) {
  /* ANDs the contents of the A register with an immediate value and then LSRs
   * the result. */
  op_AND(addr);
  op_LSR_ACC(0);  // implied addressing
}
void CPU::opi_ANC(uint16_t addr) {
  /* ANC ANDs the contents of the A register with an immediate value and then
   * moves bit 7 of A into the Carry flag.  This opcode works basically
   * identically to AND #immed. except that the Carry flag is set to the same
   * state that the Negative flag is set to. */
  op_AND(addr);
  if (status & FLAG_NEGATIVE) {
    status |= FLAG_CARRY;
  } else {
    status &= ~FLAG_CARRY;
  }
}
void CPU::opi_ANE(uint16_t addr) {
  /* aka XAA: transfers the contents of the X register to the A register and
   * then ANDs the A register with an immediate value. Highly unstable. */
  op_TXA(0);
  op_AND(addr);
}
void CPU::opi_ARR(uint16_t addr) {
  /* ANDs the contents of the A register with an immediate value and then RORs
   * the result. The carry flag is set to the value of bit 6 of the result.
        •	The overflow flag is set to the XOR of bits 6 and 5 of the
   result.*/
  op_AND(addr);
  op_ROR_ACC(0);

  if (a_register & 0x40) {
    status |= FLAG_CARRY;
  } else {
    status &= ~FLAG_CARRY;
  }

  bool newOverflow = (((a_register >> 6) & 1) ^ ((a_register >> 5) & 1)) != 0;
  if (newOverflow) {
    status |= FLAG_OVERFLOW;
  } else {
    status &= ~FLAG_OVERFLOW;
  }
}
void CPU::opi_DCP(uint16_t addr) {
  /* aka DCM: DECs the contents of a memory location and then CMPs the result
   * with the A register. */
  op_DEC(addr);
  op_CMP(addr);
}
void CPU::opi_ISC(uint16_t addr) {
  /* aka INS: INCs the contents of a memory location and then SBCs the result
   * from the A register.*/
  op_INC(addr);
  op_SBC(addr);
}
void CPU::opi_LAS(uint16_t addr) {
  /* ANDs the contents of a memory location with the contents of the stack
   * pointer register and stores the result in the accumulator, the X register,
   * and the stack pointer.  Affected flags: N Z.*/
  sp &= memRead8(addr);
  a_register = sp;
  x_register = sp;
  updateZeroAndNegativeFlags(sp);
}
void CPU::opi_LAX(uint16_t addr) {
  /* This opcode loads both the accumulator and the X register with the contents
   * of a memory location. */
  op_LDA(addr);
  op_LDX(addr);
}
void CPU::opi_LXA(uint16_t addr) {
  /* aka OAL: ORs the A register with #$EE, ANDs the result with an immediate
   * value, and then stores the result in both A and X. Highly unstable. */
  a_register |= 0xEE;
  op_AND(addr);
  op_TAX(0);
}
void CPU::opi_RLA(uint16_t addr) {
  /* ROLs the contents of a memory location and then ANDs the result with the
   * accumulator. */
  op_ROL(addr);
  op_AND(addr);
}
void CPU::opi_RRA(uint16_t addr) {
  /* RORs the contents of a memory location and then ADCs the result with the
   * accumulator. */
  op_ROR(addr);
  op_ADC(addr);
}
void CPU::opi_SAX(uint16_t addr) {
  /* aka AXS+AAX: ANDs the contents of the A and X registers (without changing
   * the contents of either register) and stores the result in memory. Does not
   * affect any flags in the processor status register.*/
  memWrite8(addr, a_register & x_register);
}
void CPU::opi_SBX(uint16_t addr) {
  /* aka AXS+SAX: ANDs the contents of the A and X registers (leaving the
   * contents of A intact), subtracts an immediate value, and then stores the
   * result in X. A few points might be made about the action of subtracting an
   * immediate value. It actually works just like the CMP instruction, except
   * that CMP does not store the result of the subtraction it performs in any
   * register. This subtract operation is not affected by the state of the Carry
   * flag, though it does affect the Carry flag. It does not affect the Overflow
   * flag. */
  x_register = (a_register & x_register) - memRead8(addr);
  // set carry flag (C) if result > 255
  if (x_register > 0xFF) {
    status |= FLAG_CARRY;
  } else {
    status &= ~FLAG_CARRY;  // else clear carry flag
  }
  updateZeroAndNegativeFlags(x_register);
}
void CPU::opi_SHA(uint16_t addr) {
  /* Stores A AND X AND (high-byte of addr. + 1) at addr. Unstable. */
  uint8_t high_plus_one = currentHighByte + 1;
  memWrite8(addr, (a_register & x_register) & high_plus_one);
}
void CPU::opi_SHX(uint16_t addr) {
  /* aka A11,SXA,XAS: Stores X AND (high-byte of addr. + 1) at addr. Unstable.
   */
  uint8_t high_plus_one = currentHighByte + 1;
  memWrite8(addr, x_register & high_plus_one);
}
void CPU::opi_SHY(uint16_t addr) {
  /* aka SAY: Stores Y AND (high-byte of addr. + 1) at addr. Unstable. */
  uint8_t high_plus_one = currentHighByte + 1;
  memWrite8(addr, y_register & high_plus_one);
}
void CPU::opi_SLO(uint16_t addr) {
  /* This opcode ASLs the contents of a memory location and then ORs the result
  with the accumulator. */
  op_ASL(addr);
  op_ORA(addr);
}
void CPU::opi_SRE(uint16_t addr) {
  /* aka LSE: LSRs the contents of a memory location and then EORs the result
   * with the accumulator. */
  op_LSR(addr);
  op_EOR(addr);
}
void CPU::opi_TAS(uint16_t addr) {
  /* ANDs the contents of the A and X registers (without changing the contents
   * of either register) and transfers the result to the stack pointer. It then
   * ANDs that result with the contents of the high byte of the target address
   * of the operand +1 and stores that final result in memory. */
  uint8_t high_plus_one = currentHighByte + 1;
  sp = a_register & x_register;
  memWrite8(addr, sp & high_plus_one);
}
void CPU::opi_SBC(uint16_t addr) { op_SBC(addr); }
void CPU::opi_NOP(uint16_t addr) { return; }
void CPU::opi_KIL(uint16_t addr) { executionActive = false; }
