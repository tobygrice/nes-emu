#ifndef CPU_H
#define CPU_H

#include <array>
#include <cstdint>
#include <vector>

#include "../Bus.h"
#include "../Logger.h"
#include "AddressResolveInfo.h"
#include "OpCode.h"

enum Interrupt { NONE, RES, NMI, IRQ };

class CPU {
  friend class OpCode;

 public:
  CPU(BusInterface* bus, Logger* logger)
      : a_register(0),       // accumulator starts at 0
        x_register(0),       // X starts at 0
        y_register(0),       // Y starts at 0
        status(0b00100000),  // status register starts with all flags clear
        pc(0x8000),          // cartridge ROM is 0x8000-0xFFFF in NES
        sp(0xFF),            // stack pointer starts at 0xFD (error point 0xFF?)
        bus(bus),            // handles all read/writes
        logger(logger),      // log class
        currAddrResCtx() {}

  void tick();

  void triggerRES() {
    initiatingInterrupt = true;
    activeInterrupt = Interrupt::RES;
  }
  void triggerNMI() {
    initiatingInterrupt = true;
    activeInterrupt = Interrupt::NMI;
  }
  void triggerIRQ() {
    initiatingInterrupt = true;
    activeInterrupt = Interrupt::IRQ;
  }

  Interrupt checkInterrupt() { return activeInterrupt; }

  uint8_t TEST_getA() { return a_register; };
  uint8_t TEST_getX() { return x_register; };
  uint8_t TEST_getY() { return y_register; };
  uint8_t TEST_getStatus() { return status; };
  uint16_t TEST_getPC() { return pc; };
  uint8_t TEST_getSP() { return sp; };
  uint8_t TEST_getCyclesRemainingInCurrentInstr() {
    return cyclesRemainingInCurrentInstr;
  };
  void TEST_setA(uint8_t value) { a_register = value; };
  void TEST_setX(uint8_t value) { x_register = value; };
  void TEST_setY(uint8_t value) { y_register = value; };
  void TEST_setStatus(uint8_t value) { status = value; };
  void TEST_setPC(uint16_t value) { pc = value; };
  void TEST_setSP(uint8_t value) { sp = value; };

  static constexpr uint8_t FLAG_CARRY = 0b00000001;      // C
  static constexpr uint8_t FLAG_ZERO = 0b00000010;       // Z
  static constexpr uint8_t FLAG_INTERRUPT = 0b00000100;  // I
  static constexpr uint8_t FLAG_DECIMAL = 0b00001000;    // D
  static constexpr uint8_t FLAG_BREAK = 0b00010000;      // B
  static constexpr uint8_t FLAG_CONSTANT = 0b00100000;   // 1 (constant)
  static constexpr uint8_t FLAG_OVERFLOW = 0b01000000;   // V
  static constexpr uint8_t FLAG_NEGATIVE = 0b10000000;   // N

 private:
  // registers http://www.6502.org/users/obelisk/6502/registers.html
  uint8_t a_register;  // accumulator
  uint8_t x_register;  // index X
  uint8_t y_register;  // index Y
  uint8_t status;      // processor status (p)
  uint16_t pc;         // program counter
  uint8_t sp;          // stack pointer
  BusInterface* bus;   // bus
  Logger* logger;      // logger

  const OpCode* currentOpCode = nullptr;
  uint8_t readBuffer;
  std::vector<uint8_t> currentOpBytes;
  uint8_t cyclesRemainingInCurrentInstr = 0;
  uint8_t cyclesRemainingInCurrentInterrupt = 7;
  AddressResolveInfo currAddrResCtx;  // current address resolution context
  uint8_t currentValueAtAddress = 0;
  CPUState* logState = nullptr;

  bool initiatingInterrupt = false;
  Interrupt activeInterrupt = Interrupt::NONE;

  // variable to hold the high byte of operand *before* dereferencing
  // only used by illegal opcodes SHA, SHX, SHY, and TAS
  uint8_t currentHighByte = 0;

  uint64_t cycleCount = 0;

  // helpers
  void branch();
  void updateZeroAndNegativeFlags(uint8_t result);

  /**
   * Pushes value to the stack. Consumes 1 cycle.
   */
  inline void push(uint8_t value) {
    bus->write(0x0100 + sp, value);
    sp--;
  }

  /**
   * Pops value from the stack. Consumes 1 cycle.
   */
  inline uint8_t pop() {
    sp++;
    return bus->read(0x100 + sp);
  }

  // addressing mode handling
  void computeAbsoluteAddress();

  inline uint16_t assembleBytes(uint8_t high, uint8_t low) {
    return (static_cast<uint16_t>(high) << 8) | static_cast<uint16_t>(low);
  }

  inline void readOperand() {
    currentOpBytes.push_back(bus->read(pc));
    pc++;
  }

  // interrupts:
  void in_RES();
  void in_NMI_IRQ();

  // instruction implementations - 56 instructions, 151 opcodes
  void op_ADC(uint16_t addr);
  void op_ADC_CORE(uint8_t operand);  // allows SBC to use ADC logic
  void op_AND(uint16_t addr);
  void op_ASL(uint16_t addr);
  void op_ASL_ACC(uint16_t /* implied */);
  void op_BCC(uint16_t addr);
  void op_BCS(uint16_t addr);
  void op_BEQ(uint16_t addr);
  void op_BIT(uint16_t addr);
  void op_BMI(uint16_t addr);
  void op_BNE(uint16_t addr);
  void op_BPL(uint16_t addr);
  void op_BRK(uint16_t /* none addressing */);
  void op_BVC(uint16_t addr);
  void op_BVS(uint16_t addr);
  void op_CLC(uint16_t addr);
  void op_CLD(uint16_t addr);
  void op_CLI(uint16_t addr);
  void op_CLV(uint16_t addr);
  void op_CMP(uint16_t addr);
  void op_CPX(uint16_t addr);
  void op_CPY(uint16_t addr);
  void op_DEC(uint16_t addr);
  void op_DEX(uint16_t /* implied */);
  void op_DEY(uint16_t /* implied */);
  void op_EOR(uint16_t addr);
  void op_INC(uint16_t addr);
  void op_INX(uint16_t addr);
  void op_INY(uint16_t addr);
  void op_JMP(uint16_t addr);
  void op_JSR(uint16_t addr);
  void op_LDA(uint16_t addr);
  void op_LDX(uint16_t addr);
  void op_LDY(uint16_t addr);
  void op_LSR(uint16_t addr);
  void op_LSR_ACC(uint16_t /* implied */);
  void op_NOP(uint16_t addr);
  void op_ORA(uint16_t addr);
  void op_PHA(uint16_t addr);
  void op_PHP(uint16_t addr);
  void op_PLA(uint16_t addr);
  void op_PLP(uint16_t addr);
  void op_ROL(uint16_t addr);
  void op_ROL_ACC(uint16_t /* implied */);
  void op_ROR(uint16_t addr);
  void op_ROR_ACC(uint16_t /* implied */);
  void op_RTI(uint16_t addr);
  void op_RTS(uint16_t addr);
  void op_SBC(uint16_t addr);
  void op_SEC(uint16_t addr);
  void op_SED(uint16_t addr);
  void op_SEI(uint16_t addr);
  void op_STA(uint16_t addr);
  void op_STX(uint16_t addr);
  void op_STY(uint16_t addr);
  void op_TAX(uint16_t addr);
  void op_TAY(uint16_t addr);
  void op_TSX(uint16_t addr);
  void op_TXA(uint16_t addr);
  void op_TXS(uint16_t addr);
  void op_TYA(uint16_t addr);

  // remaining 21 instructions / 105 unofficial opcodes:
  void opi_ALR(uint16_t addr);
  void opi_ANC(uint16_t addr);
  void opi_ANC2(uint16_t addr);
  void opi_ANE(uint16_t addr);
  void opi_ARR(uint16_t addr);
  void opi_DCP(uint16_t addr);
  void opi_ISC(uint16_t addr);
  void opi_LAS(uint16_t addr);
  void opi_LAX(uint16_t addr);
  void opi_LXA(uint16_t addr);
  void opi_RLA(uint16_t addr);
  void opi_RRA(uint16_t addr);
  void opi_SAX(uint16_t addr);
  void opi_SBX(uint16_t addr);
  void opi_SHA(uint16_t addr);
  void opi_SHX(uint16_t addr);
  void opi_SHY(uint16_t addr);
  void opi_SLO(uint16_t addr);
  void opi_SRE(uint16_t addr);
  void opi_TAS(uint16_t addr);
  void opi_SBC(uint16_t addr);
  void opi_NOP(uint16_t addr);
  void opi_KIL(uint16_t addr);
};

#endif  // CPU_H