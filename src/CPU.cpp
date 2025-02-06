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
      pc(0x0000),     // program counter starts at 0
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
  // cartridge ROM is 0x8000-0xFFFF in NES
  std::copy(program.begin(), program.end(), memory.data() + 0x8000);
  memWrite16(0xFFFC, 0x8000);  // set reset address to start at 0x8000
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
uint16_t CPU::getOperandAddress(AddressingMode mode) const {
  switch (mode) {
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
      return base + x_register;  // 16-bit addition
    }

    case AddressingMode::Absolute_Y: {
      uint16_t base = memRead16(pc);
      return base + y_register;
    }

    case AddressingMode::Indirect_X: {
      uint8_t base = memRead8(pc);
      uint8_t ptr = static_cast<uint8_t>(
          base + x_register);  // wrapping addition on 8-bit
      uint8_t lo = memRead8(static_cast<uint16_t>(ptr));
      uint8_t hi =
          memRead8(static_cast<uint16_t>(static_cast<uint8_t>(ptr + 1)));
      return (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    }

    case AddressingMode::Indirect_Y: {
      uint8_t base = memRead8(pc);
      uint8_t lo = memRead8(static_cast<uint16_t>(base));
      uint8_t hi =
          memRead8(static_cast<uint16_t>(static_cast<uint8_t>(base + 1)));
      uint16_t deref_base =
          (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
      return deref_base + y_register;
    }

    case AddressingMode::NoneAddressing:
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
    status |= 0b00000010;  // set zero flag if result is 0
  } else {
    status &= 0b11111101;  // else clear zero flag
  }

  // negative flag is bit 7
  if (result & 0b10000000) {
    status |= 0b10000000;  // set negative flag if bit 7 of result is 1
  } else {
    status &= 0b01111111;  // else clear negative flag
  }
}

void CPU::op_ADC(AddressingMode mode) {
  uint16_t addr = getOperandAddress(mode);
  uint8_t value = memRead8(addr); 
  uint8_t carry = (status & 0x01);  // extract the carry flag value from status register
  uint16_t result = a_register + value + carry;  // compute result

  // set carry flag (C) if result > 255
  if (result > 0xFF) {
    status |= 0b00000001;
  } else {
    status &= 0b11111110;  // else clear carry flag
  }

  // set overflow flag (V) if a signed overflow occurs (adding 2 positive or 2 negative 
  // numbers results in a different-signed result)
  bool overflow = (~(a_register ^ value) & (a_register ^ result) & 0b10000000);
  if (overflow) {
    status |= 0b01000000;  // set overflow flag (bit 6)
  } else {
    status &= 0b10111111;  // clear overflow flag
  }

  a_register = static_cast<uint8_t>(result); // store result in A reg

  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_AND(AddressingMode mode) {
  
}
void CPU::op_ASL(AddressingMode mode) { /* TO-DO */ }
void CPU::op_BCC(AddressingMode mode) { /* TO-DO */ }
void CPU::op_BCS(AddressingMode mode) { /* TO-DO */ }
void CPU::op_BEQ(AddressingMode mode) { /* TO-DO */ }
void CPU::op_BIT(AddressingMode mode) { /* TO-DO */ }
void CPU::op_BMI(AddressingMode mode) { /* TO-DO */ }
void CPU::op_BNE(AddressingMode mode) { /* TO-DO */ }
void CPU::op_BPL(AddressingMode mode) { /* TO-DO */ }
void CPU::op_BRK(AddressingMode /* always implicit */) {
  // According to many references, when BRK is executed the CPU pushes (PC - 1)
  // onto the stack. If the PC has been advanced by 2 (i.e., PC = original PC + 2),
  // then (PC - 1) is the return address.
  uint16_t returnAddress = pc - 1;

  // Push returnAddress onto the stack, high byte first.
  memWrite8(0x0100 + sp--, (returnAddress >> 8) & 0xFF);
  memWrite8(0x0100 + sp--, returnAddress & 0xFF);

  // Push the status register with the Break flag set.
  // Ensure we do not inadvertently push the Negative flag.
  uint8_t statusToPush = (status & ~0x80) | 0b00010000;
  memWrite8(0x0100 + sp--, statusToPush);

  // Set the Interrupt Disable flag (bit 2).
  status |= 0b00000100;

  // Fetch the new program counter from the interrupt vector.
  pc = memRead16(0xFFFE);
}
void CPU::op_BVC(AddressingMode mode) { /* TO-DO */ }
void CPU::op_BVS(AddressingMode mode) { /* TO-DO */ }
void CPU::op_CLC(AddressingMode mode) { /* TO-DO */ }
void CPU::op_CLD(AddressingMode mode) { /* TO-DO */ }
void CPU::op_CLI(AddressingMode mode) { /* TO-DO */ }
void CPU::op_CLV(AddressingMode mode) { /* TO-DO */ }
void CPU::op_CMP(AddressingMode mode) { /* TO-DO */ }
void CPU::op_CPX(AddressingMode mode) { /* TO-DO */ }
void CPU::op_CPY(AddressingMode mode) { /* TO-DO */ }
void CPU::op_DEC(AddressingMode mode) { /* TO-DO */ }
void CPU::op_DEX(AddressingMode mode) { /* TO-DO */ }
void CPU::op_DEY(AddressingMode mode) { /* TO-DO */ }
void CPU::op_EOR(AddressingMode mode) { /* TO-DO */ }
void CPU::op_INC(AddressingMode mode) { /* TO-DO */ }
void CPU::op_INX(AddressingMode mode) { /* TO-DO */ }
void CPU::op_INY(AddressingMode mode) { /* TO-DO */ }
void CPU::op_JMP(AddressingMode mode) { /* TO-DO */ }
void CPU::op_JSR(AddressingMode mode) { /* TO-DO */ }
void CPU::op_LDA(AddressingMode mode) {
  uint16_t addr = getOperandAddress(mode);
  uint8_t value = memRead8(addr);
  a_register = value;
  updateZeroAndNegativeFlags(a_register);
}
void CPU::op_LDX(AddressingMode mode) {
  uint16_t addr = getOperandAddress(mode);
  uint8_t value = memRead8(addr);
  x_register = value;
  updateZeroAndNegativeFlags(x_register);
}
void CPU::op_LDY(AddressingMode mode) {
  uint16_t addr = getOperandAddress(mode);
  uint8_t value = memRead8(addr);
  y_register = value;
  updateZeroAndNegativeFlags(y_register);
}
void CPU::op_LSR(AddressingMode mode) { /* TO-DO */ }
void CPU::op_NOP(AddressingMode mode) { /* TO-DO */ }
void CPU::op_ORA(AddressingMode mode) { /* TO-DO */ }
void CPU::op_PHA(AddressingMode mode) { /* TO-DO */ }
void CPU::op_PHP(AddressingMode mode) { /* TO-DO */ }
void CPU::op_PLA(AddressingMode mode) { /* TO-DO */ }
void CPU::op_PLP(AddressingMode mode) { /* TO-DO */ }
void CPU::op_ROL(AddressingMode mode) { /* TO-DO */ }
void CPU::op_ROR(AddressingMode mode) { /* TO-DO */ }
void CPU::op_RTI(AddressingMode mode) { /* TO-DO */ }
void CPU::op_RTS(AddressingMode mode) { /* TO-DO */ }
void CPU::op_SBC(AddressingMode mode) { /* TO-DO */ }
void CPU::op_SEC(AddressingMode mode) { /* TO-DO */ }
void CPU::op_SED(AddressingMode mode) { /* TO-DO */ }
void CPU::op_SEI(AddressingMode mode) { /* TO-DO */ }
void CPU::op_STA(AddressingMode mode) { /* TO-DO */ }
void CPU::op_STX(AddressingMode mode) { /* TO-DO */ }
void CPU::op_STY(AddressingMode mode) { /* TO-DO */ }
void CPU::op_TAX(AddressingMode mode) { /* TO-DO */ }
void CPU::op_TAY(AddressingMode mode) { /* TO-DO */ }
void CPU::op_TSX(AddressingMode mode) { /* TO-DO */ }
void CPU::op_TXA(AddressingMode mode) { /* TO-DO */ }
void CPU::op_TXS(AddressingMode mode) { /* TO-DO */ }
void CPU::op_TYA(AddressingMode mode) { /* TO-DO */ }

void CPU::executeProgram() {
  while (true) {
    uint8_t opcode = memRead8(pc);  // fetch opcode

    const OpCode* op = getOpCode(opcode);  // look up opcode
    if (!op) {
      throw std::runtime_error("Unknown opcode: " + std::to_string(opcode));
    }

    pc++;  // increment PC to point to first operand byte

    (this->*(op->handler))(op->mode);  // execute appropriate handler function

    // advance PC to consume operand bytes (except for BRK, which replaces PC)
    if (opcode != 0x00) {
      pc += (op->bytes - 1);
    } else {
      return;
    }
  }
}