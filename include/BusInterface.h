#ifndef BUSINTERFACE_H
#define BUSINTERFACE_H

#include <cstdint>

class BusInterface {
 public:
  virtual ~BusInterface() = default;
  virtual uint8_t read(uint16_t addr) = 0;
  virtual void write(uint16_t addr, uint8_t value) = 0;
  virtual uint16_t getPPUScanline() = 0;
  virtual uint16_t getPPUCycle() = 0;
};

#endif