#include "../include/Clock.h"

#include "../include/NES.h"

Clock::Clock(NES& nes)
    : nes(nes), region(NESRegion::None), running(false), lastNMIState(false) {}

void Clock::setRegion(NESRegion region) {
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
void Clock::start() {
  if (region == NESRegion::None) {
    throw std::runtime_error("No region set");
  } else {
    running = true;
    lastNMIState = false;
    gameLoop();
    // cpuThread = std::thread(&Clock::cpuLoop, this);
    // ppuThread = std::thread(&Clock::ppuLoop, this);
  }
}

void Clock::gameLoop() {
  while (running) {
    nes.cpu.tick();
    for (int i = 0; i < 3; i++) {
      std::unique_ptr<Frame> frame = nes.ppu.tick();
      const bool nmiState = nes.bus.ppuNMI();
      if (nmiState && !lastNMIState) {
        nes.cpu.triggerNMI();
      }
      lastNMIState = nmiState;
      if (frame) {
        render(*frame);
      }
    }
  }
}

// Signal the threads to stop and join them
void Clock::stop() {
  running = false;
  if (cpuThread.joinable()) cpuThread.join();
  if (ppuThread.joinable()) ppuThread.join();
  if (rendererThread.joinable()) rendererThread.join();
}

void Clock::render(const Frame& frame) {
  if (!nes.renderer.has_value()) {
    return;
  }
  nes.renderer->get().render(frame);
}


void Clock::cpuLoop() {
  using clock = std::chrono::steady_clock;
  const auto tickDuration = std::chrono::duration_cast<clock::duration>(
      std::chrono::duration<double>(cpuTickInterval));
  auto startTime = clock::now();
  unsigned long long cpuCycleCount = 0;
  while (running) {
    nes.cpu.tick();
    cpuCycleCount++;
    auto nextTick = startTime + cpuCycleCount * tickDuration;
    std::this_thread::sleep_until(nextTick);
  }
}

void Clock::ppuLoop() {
  using clock = std::chrono::steady_clock;
  const auto tickDuration = std::chrono::duration_cast<clock::duration>(
      std::chrono::duration<double>(ppuTickInterval));
  auto startTime = clock::now();
  unsigned long long ppuCycleCount = 0;
  while (running) {
    std::cout << "started ppu tick\n";
    std::unique_ptr<Frame> frame = nes.ppu.tick();
    const bool nmiState = nes.bus.ppuNMI();
    if (nmiState && !lastNMIState) {
      nes.cpu.triggerNMI();
    }
    lastNMIState = nmiState;
    if (frame) {
      render(*frame);
    }

    ppuCycleCount++;
    auto nextTick = startTime + ppuCycleCount * tickDuration;
    std::this_thread::sleep_until(nextTick);
    std::cout << "finished ppu tick\n";
  }
}
