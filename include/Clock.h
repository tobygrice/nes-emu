#ifndef CLOCK_H
#define CLOCK_H

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "NES.h"

const double TARGET_SPEED = 0.1;  // game speed to target (1 = full speed 60fps)

// NTSC clock speed is 236.25 MHz ÷ 11 by definition
// PAL clock speed is 26.6017125 MHz by definition
const double MASTER_SPEED_NTSC =
    TARGET_SPEED * 1000000 * (236.25 / 11);  // ~21.477 MHz
const double MASTER_SPEED_PAL =
    TARGET_SPEED * 1000000 * 26.6017125;  // ~26.601 MHz

class Clock {
 private:
  NES* nes;
  NESRegion region = NESRegion::None;
  std::thread cpuThread;
  std::thread ppuThread;
  // std::thread apuThread;
  std::atomic<bool> running;

  double cpuTickInterval;
  double ppuTickInterval;

 public:
  /** 
   * Region MUST be set using setRegion() before starting
   */
  Clock(NES* nes) : nes(nes), running(false) {}

  void setRegion(NESRegion region) {
    this->region = region;
    // calculate CPU/PPU tick intervals (time in seconds between ticks)
    // according to region:
    if (region == NESRegion::NTSC) {
      this->cpuTickInterval = 1.0 / (MASTER_SPEED_NTSC / 12);
      this->ppuTickInterval = 1.0 / (MASTER_SPEED_NTSC / 4);
    } else if (region == NESRegion::PAL) {  // PAL
      this->cpuTickInterval = 1.0 / (MASTER_SPEED_PAL / 16);
      this->ppuTickInterval = 1.0 / (MASTER_SPEED_PAL / 5);
    }
  }

  // Start the clock threads
  void start() {
    if (region == NESRegion::None) {
      throw std::runtime_error("No region set");
    } else {
      running = true;
      cpuThread = std::thread(&Clock::cpuLoop, this);
      ppuThread = std::thread(&Clock::ppuLoop, this);
    }
  }

  // Signal the threads to stop and join them
  void stop() {
    running = false;
    if (cpuThread.joinable()) cpuThread.join();
    if (ppuThread.joinable()) ppuThread.join();
  }

 private:
  void cpuLoop() {
    using clock = std::chrono::steady_clock;
    const auto tickDuration = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<double>(cpuTickInterval));
    auto startTime = clock::now();
    unsigned long long cpuCycleCount = 0;
    while (running) {
      nes->cpu.tick();
      cpuCycleCount++;
      auto nextTick = startTime + cpuCycleCount * tickDuration;
      std::this_thread::sleep_until(nextTick);
    }

    /* option 2: wait for fixed interval after processing (does not consider
    tick processing time) auto nextTick = std::chrono::steady_clock::now();
    while (running) {
      nes->cpu.tick();
      nextTick +=
          std::chrono::duration_cast<std::chrono::steady_clock::duration>(
              std::chrono::duration<double>(cpuTickInterval));
      std::this_thread::sleep_until(nextTick);
    }
      */
  }

  void ppuLoop() {
    using clock = std::chrono::steady_clock;
    const auto tickDuration = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<double>(ppuTickInterval));
    auto startTime = clock::now();
    unsigned long long ppuCycleCount = 0;
    while (running) {
      // nes->ppu.tick();
      ppuCycleCount++;
      auto nextTick = startTime + ppuCycleCount * tickDuration;
      std::this_thread::sleep_until(nextTick);
    }
  }
};

#endif  // CLOCK_H