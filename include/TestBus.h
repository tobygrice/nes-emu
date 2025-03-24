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

 public:
  virtual uint8_t read(uint16_t addr) override {
    return memory[addr];
  }
  virtual void write(uint16_t addr, uint8_t value) override {
    memory[addr] = value;
  }

  inline uint16_t getPPUScanline() override { return 0; }
  inline uint16_t getPPUCycle() override { return 0; }
};

#endif