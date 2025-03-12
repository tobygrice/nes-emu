#ifndef BUSINTERFACE_H
#define BUSINTERFACE_H

#include <cstdint>

class BusInterface {
 public:
  virtual ~BusInterface() = default;
  virtual uint8_t read(uint16_t addr) = 0;
  virtual void write(uint16_t addr, uint8_t value) = 0;
  virtual void tick(uint8_t cycles) = 0;
  virtual uint64_t getCycleCount() const = 0;
  virtual void resetCycles() = 0;

  virtual bool ppuNMI() = 0;
};

#endif