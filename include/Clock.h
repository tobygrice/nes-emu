#ifndef CLOCK_H
#define CLOCK_H

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "NES.h"

enum class NESRegion { NTSC, PAL };

// NTSC clock speed is 236.25 MHz ÷ 11 by definition
// PAL clock speed is 26.6017125 MHz by definition
const double MASTER_SPEED_NTSC = 1000000 * (236.25 / 11);  // ~21.477 MHz
const double MASTER_SPEED_PAL = 1000000 * 26.6017125;      // ~26.601 MHz

class Clock {
 private:
  NES* nes;
  NESRegion region;
  std::thread cpuThread;
  std::thread ppuThread;
  // std::thread apuThread;
  std::atomic<bool> running;

  long double cpuTickInterval;
  long double ppuTickInterval;

 public:
  Clock(NES* nes, NESRegion region) : nes(nes), region(region), running(false) {
    // calculate CPU/PPU tick intervals (time in seconds between ticks)
    // according to region
    if (region == NESRegion::NTSC) {
      cpuTickInterval = 1.0 / (MASTER_SPEED_NTSC / 12);
      ppuTickInterval = 1.0 / (MASTER_SPEED_NTSC / 4);
    } else {  // PAL
      cpuTickInterval = 1.0 / (MASTER_SPEED_PAL / 16);
      ppuTickInterval = 1.0 / (MASTER_SPEED_PAL / 5);
    }
  }

  // Start the clock threads
  void start() {
    running = true;
    cpuThread = std::thread(&Clock::cpuLoop, this);
    ppuThread = std::thread(&Clock::ppuLoop, this);
  }

  // Signal the threads to stop and join them
  void stop() {
    running = false;
    if (cpuThread.joinable()) cpuThread.join();
    if (ppuThread.joinable()) ppuThread.join();
  }

 private:
  // CPU thread loop calls nes->cpu.tick() at the proper interval
  void cpuLoop() {
    using namespace std::chrono;
    auto nextTick = high_resolution_clock::now();
    while (running) {
      nes->cpu.tick();
      nextTick += duration<double>(cpuTickInterval);
      std::this_thread::sleep_until(nextTick);
    }
  }

  // PPU thread loop calls nes->ppu.tick() at the proper interval
  void ppuLoop() {
    using namespace std::chrono;
    auto nextTick = high_resolution_clock::now();
    while (running) {
      nes->ppu.tick();
      nextTick += duration<double>(ppuTickInterval);
      std::this_thread::sleep_until(nextTick);
    }
  }
};

#endif  // CLOCK_H