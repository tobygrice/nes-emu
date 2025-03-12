#ifndef TESTBUS_H
#define TESTBUS_H

/**
 * A "dumb bus" for use in testing. Does not consider NES memory mapping and
 * allows writing to ROM.
 */

#include <array>
#include <cstdint>

#include "BusInterface.h"

struct TestBus : public BusInterface {
 private:
  std::array<uint8_t, 0x10000> memory = {};
  uint64_t cycles = 0;

 public:
  virtual bool ppuNMI() override { return false; }
  virtual void resetCycles() override { cycles = 0; }
  virtual uint8_t read(uint16_t addr) override {
    cycles++;
    return memory[addr];
  }
  virtual void write(uint16_t addr, uint8_t value) override {
    cycles++;
    memory[addr] = value;
  }
  virtual uint64_t getCycleCount() const override { return cycles; }
  virtual void tick(uint8_t cyc) override { cycles += cyc; }
};

#endif