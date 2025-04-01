#ifndef CLOCK_H
#define CLOCK_H

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

class NES;
class Frame;
enum class NESRegion;

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
  NESRegion region;
  std::thread coreThread;
  std::thread cpuThread;
  std::thread ppuThread;
  std::thread rendererThread;
  // std::thread apuThread;
  std::atomic<bool> running;

  double cpuTickInterval;
  double ppuTickInterval;

 public:
  /**
   * Region MUST be set using setRegion() before starting
   */
  Clock(NES* nes);

  void setRegion(NESRegion region);

  // Start the clock threads
  void start();

  // Signal the threads to stop and join them
  void stop();

 private:
  void gameLoop();
  void cpuLoop();
  void ppuLoop();
  void render(Frame* frame);
};

#endif  // CLOCK_H