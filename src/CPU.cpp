#include "../include/CPU.h"

#include "../include/OpCode.h"

/**
 * Constructor to initialise registers with starting values.
 */
CPU::CPU()
    : a_register(0),  // accumulator starts at 0
      x_register(0),  // X starts at 0
      y_register(0),  // Y starts at 0
      status(0x00),   // status register starts with all flags clear
      pc(0x8000),     // cartridge ROM is 0x8000-0xFFFF in NES
      sp(0xFF) {      // stack pointer starts at 0xFF
  memory.fill(0);     // memory initialised to 0s
}

/**
 * Fetches value at given address.
 *
 * @param addr The address from which to retrieve data.
 * @return Single byte of memory at provided addr.
 */
uint8_t CPU::memRead8(uint16_t addr) const { return memory[addr]; }

/**
 * Writes given data at given address.
 *
 * @param addr The address to write the data.
 * @param data The data to be written.
 */
void CPU::memWrite8(uint16_t addr, uint8_t data) { memory[addr] = data; }

/**
 * Reads 2 bytes of data from the given address, accounting for little endian.
 *
 * @param addr The address from which to retrieve data.
 * @return Two bytes of memory at provided addr.
 */
uint16_t CPU::memRead16(uint16_t addr) const {
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
 * Loads provided program into memory.
 *
 * @param program The program instruction set.
 */
void CPU::loadProgram(const std::vector<uint8_t>& program) {
  std::copy(program.begin(), program.end(), memory.data() + pc);
  memWrite16(0xFFFC, pc);  // set reset address to start at pc
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
  status = 0;
  pc = memRead16(0xFFFC);
}

/**
 * Function to return the address of the operand given the addressing mode.
 *
 * @param mode The addressing mode specified by the opcode
 * @return A 16-bit memory address
 */
uint16_t CPU::getOperandAddress(AddressingMode mode) {
  switch (mode) {
    case AddressingMode::NoneAddressing:
    case AddressingMode::Accumulator:
      return 0;  // accumulator and implicit opcodes do not require an address

    case AddressingMode::Relative:
      return pc + 1 + static_cast<int8_t>(memRead8(pc));

    case AddressingMode::Immediate:
      return pc;

    case AddressingMode::ZeroPage:
      return static_cast<uint16_t>(memRead8(pc));

    case AddressingMode::Absolute:
      return memRead16(pc);

    case AddressingMode::ZeroPage_X: {
      uint8_t pos = memRead8(pc);
      // Wrap-around addition on 8-bit values:
      uint8_t addr = static_cast<uint8_t>(pos + x_register);
      return static_cast<uint16_t>(addr);
    }

    case AddressingMode::ZeroPage_Y: {
      uint8_t pos = memRead8(pc);
      uint8_t addr = static_cast<uint8_t>(pos + y_register);
      return static_cast<uint16_t>(addr);
    }

    case AddressingMode::Absolute_X: {
      uint16_t base = memRead16(pc);
      uint16_t addr = base + x_register;
      if ((base & 0xFF00) != (addr & 0xFF00)) {
        // MSB of base address and resulting addition address is different
        this->cycleCount += 1;  // +1 cycle for page crossed
      }
      return addr;
    }

    case AddressingMode::Absolute_Y: {
      uint16_t base = memRead16(pc);
      uint16_t addr = base + y_register;
      if ((base & 0xFF00) != (addr & 0xFF00)) {
        // MSB of base address and resulting addition address is different
        this->cycleCount += 1;  // +1 cycle for page crossed
      }
      return addr;
    }

    case AddressingMode::Indirect_X: {
      uint8_t base = memRead8(pc);
      uint8_t ptr =
          static_cast<uint8_t>(base + x_register);  // wrapping add on 8-bit
      uint8_t low = memRead8(static_cast<uint16_t>(ptr));
      uint8_t high =
          memRead8(static_cast<uint16_t>(static_cast<uint8_t>(ptr + 1)));
      return (static_cast<uint16_t>(high) << 8) | static_cast<uint16_t>(low);
    }

    case AddressingMode::Indirect_Y: {
      uint8_t base = memRead8(pc);
      uint8_t low = memRead8(static_cast<uint16_t>(base));
      uint8_t high =
          memRead8(static_cast<uint16_t>(static_cast<uint8_t>(base + 1)));
      uint16_t deref_base =
          (static_cast<uint16_t>(high) << 8) | static_cast<uint16_t>(low);
      uint16_t addr = deref_base + y_register;
      if ((deref_base & 0xFF00) != (addr & 0xFF00)) {
        this->cycleCount += 1;  // Extra cycle
      }
      return addr;
    }
    default:
      throw std::runtime_error("Addressing mode not supported");
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
void CPU::branch(uint16_t addr) {
  // relative address was already computed by getOperandAddress, set pc to addr

  // +1 cycle if page crossed (high byte changed)
  if (((pc + 1) & 0xFF00) != (addr & 0xFF00)) {
    cycleCount += 1;
  }

  pc = addr;          // update pc
  pcModified = true;  // set flag for execution loop
  cycleCount += 1;    // +1 cycle for branch taken
}

void CPU::op_ADC(uint16_t addr) {
  uint8_t value = memRead8(addr);
  uint8_t carry =
      (status & 0x01);  // extract the carry flag value from status register
  uint16_t result = a_register + value + carry;  // compute result

  // set carry flag (C) if result > 255
  if (result > 0xFF) {
    status |= FLAG_CARRY;
  } else {
    status &= ~FLAG_CARRY;  // else clear carry flag
  }

  // set overflow flag (V) if a signed overflow occurs (adding 2 positive or 2
  // negative numbers results in a different-signed result)
  bool overflow = (~(a_register ^ value) & (a_register ^ result) & 0b10000000);
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
void CPU::op_ASL_ACC(uint16_t /* addr ignored */) {
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
  status &= ~(FLAG_NEGATIVE | FLAG_OVERFLOW | FLAG_ZERO); // clear N,V,Z flags
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
void CPU::op_BRK(uint16_t /* always implicit */) {
  // push pc-1 onto the stack
  uint16_t returnAddress = pc - 1;

  // Push returnAddress onto the stack, high byte first.
  memWrite8(0x0100 + sp--, (returnAddress >> 8) & 0xFF);
  memWrite8(0x0100 + sp--, returnAddress & 0xFF);

  // Push the status register with the Break flag set.
  // Ensure we do not inadvertently push the Negative flag.
  uint8_t statusToPush = (status & ~0x80) | FLAG_BREAK;
  memWrite8(0x0100 + sp--, statusToPush);

  // Set the Interrupt Disable flag (bit 2).
  status |= FLAG_INTERRUPT;

  // Fetch the new program counter from the interrupt vector.
  pc = memRead16(0xFFFE);
  pcModified = true;
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
void CPU::op_CLC(uint16_t addr) { /* TO-DO */ }
void CPU::op_CLD(uint16_t addr) { /* TO-DO */ }
void CPU::op_CLI(uint16_t addr) { /* TO-DO */ }
void CPU::op_CLV(uint16_t addr) { /* TO-DO */ }
void CPU::op_CMP(uint16_t addr) { /* TO-DO */ }
void CPU::op_CPX(uint16_t addr) {
  // C set if X >= M
  // Z set if X == M
  // N set if X < M
  uint8_t result = x_register - memRead8(addr);
  // clear negative, zero, and carry flags
  status &= ~(FLAG_NEGATIVE | FLAG_ZERO | FLAG_CARRY);

  if (result & 0x80) {
    status |= FLAG_NEGATIVE;
  } else if (result == 0) {
    status |= FLAG_ZERO;
    status |= FLAG_CARRY;
  } else {
    status |= FLAG_CARRY;
  }
}
void CPU::op_CPY(uint16_t addr) { /* TO-DO */ }
void CPU::op_DEC(uint16_t addr) {
  uint8_t value = memRead8(addr);
  value -= 1;
  memWrite8(addr, value);
  updateZeroAndNegativeFlags(value);
}
void CPU::op_DEX(uint16_t /* implied */) {
  x_register -= 1;
  updateZeroAndNegativeFlags(x_register);
}
void CPU::op_DEY(uint16_t /* implied */) {
  y_register -= 1;
  updateZeroAndNegativeFlags(y_register);
}
void CPU::op_EOR(uint16_t addr) { /* TO-DO */ }
void CPU::op_INC(uint16_t addr) { /* TO-DO */ }
void CPU::op_INX(uint16_t addr) { /* TO-DO */ }
void CPU::op_INY(uint16_t addr) { /* TO-DO */ }
void CPU::op_JMP(uint16_t addr) { /* TO-DO */ }
void CPU::op_JSR(uint16_t addr) { /* TO-DO */ }
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
void CPU::op_LSR(uint16_t addr) { /* TO-DO */ }
void CPU::op_NOP(uint16_t addr) { /* TO-DO */ }
void CPU::op_ORA(uint16_t addr) { /* TO-DO */ }
void CPU::op_PHA(uint16_t addr) { /* TO-DO */ }
void CPU::op_PHP(uint16_t addr) { /* TO-DO */ }
void CPU::op_PLA(uint16_t addr) { /* TO-DO */ }
void CPU::op_PLP(uint16_t addr) { /* TO-DO */ }
void CPU::op_ROL(uint16_t addr) { /* TO-DO */ }
void CPU::op_ROR(uint16_t addr) { /* TO-DO */ }
void CPU::op_RTI(uint16_t addr) { /* TO-DO */ }
void CPU::op_RTS(uint16_t addr) { /* TO-DO */ }
void CPU::op_SBC(uint16_t addr) { /* TO-DO */ }
void CPU::op_SEC(uint16_t addr) { /* TO-DO */ }
void CPU::op_SED(uint16_t addr) { /* TO-DO */ }
void CPU::op_SEI(uint16_t addr) { /* TO-DO */ }
void CPU::op_STA(uint16_t addr) { memWrite8(addr, a_register); }
void CPU::op_STX(uint16_t addr) { memWrite8(addr, x_register); }
void CPU::op_STY(uint16_t addr) { memWrite8(addr, y_register); }
void CPU::op_TAX(uint16_t addr) { /* TO-DO */ }
void CPU::op_TAY(uint16_t addr) { /* TO-DO */ }
void CPU::op_TSX(uint16_t addr) { /* TO-DO */ }
void CPU::op_TXA(uint16_t addr) { /* TO-DO */ }
void CPU::op_TXS(uint16_t addr) { /* TO-DO */ }
void CPU::op_TYA(uint16_t addr) { /* TO-DO */ }

void CPU::executeProgram() {
  while (true) {
    uint8_t opcode = memRead8(pc);  // fetch opcode

    const OpCode* op = getOpCode(opcode);  // look up opcode
    if (!op) {
      throw std::runtime_error("Unknown opcode: " + std::to_string(opcode));
    }

    pc++;  // increment PC to point to first operand byte

    uint16_t addr = getOperandAddress(op->mode);
    (this->*(op->handler))(addr);  // execute appropriate handler function

    cycleCount += op->cycles;  // increment cycle count

    if (opcode == 0x00) return;  // exit if BRK

    // advance PC to consume operand bytes
    if (!pcModified) {
      pc += (op->bytes - 1);
    } else {
      pcModified = false;
    }
  }
}