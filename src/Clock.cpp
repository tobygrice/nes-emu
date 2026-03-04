#include "../include/Clock.h"

#include <SDL3/SDL.h>
#include <cstdint>

#include "../include/NES.h"

Clock::Clock(NES& nes)
    : nes(nes),
      region(NESRegion::None),
      running(false),
      lastNMIState(false),
      pendingNMIEdge(false) {}

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
    pendingNMIEdge = false;
    gameLoop();
    // cpuThread = std::thread(&Clock::cpuLoop, this);
    // ppuThread = std::thread(&Clock::ppuLoop, this);
  }
}

void Clock::gameLoop() {
  using clock = std::chrono::steady_clock;
  constexpr double NTSC_CPU_CYCLES_PER_FRAME = 29780.5;
  constexpr double PAL_CPU_CYCLES_PER_FRAME = 33247.5;
  const double cpuHz = (region == NESRegion::PAL)
                           ? (MASTER_SPEED_PAL / 16.0)
                           : (MASTER_SPEED_NTSC / 12.0);
  const double frameRate = (region == NESRegion::PAL)
                               ? (cpuHz / PAL_CPU_CYCLES_PER_FRAME)
                               : (cpuHz / NTSC_CPU_CYCLES_PER_FRAME);
  const auto frameDuration = std::chrono::duration_cast<clock::duration>(
      std::chrono::duration<double>(1.0 / frameRate));
  auto nextFrameDeadline = clock::now() + frameDuration;

  uint8_t joypad1State = 0;
  auto setButtonState = [&](uint8_t mask, bool pressed) {
    if (pressed) {
      joypad1State |= mask;
    } else {
      joypad1State = static_cast<uint8_t>(joypad1State & ~mask);
    }
    nes.bus.setJoypad1Buttons(joypad1State);
  };

  auto processEvents = [&]() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      } else if (event.type == SDL_EVENT_KEY_DOWN ||
                 event.type == SDL_EVENT_KEY_UP) {
        const bool pressed = (event.type == SDL_EVENT_KEY_DOWN);
        if (event.key.repeat) {
          continue;
        }
        switch (event.key.key) {
          case SDLK_ESCAPE:
            if (pressed) {
              running = false;
            }
            break;
          case SDLK_Z:
            setButtonState(Bus::JOYPAD_A, pressed);
            break;
          case SDLK_X:
            setButtonState(Bus::JOYPAD_B, pressed);
            break;
          case SDLK_RETURN:
            setButtonState(Bus::JOYPAD_START, pressed);
            break;
          case SDLK_RSHIFT:
          case SDLK_LSHIFT:
            setButtonState(Bus::JOYPAD_SELECT, pressed);
            break;
          case SDLK_UP:
            setButtonState(Bus::JOYPAD_UP, pressed);
            break;
          case SDLK_DOWN:
            setButtonState(Bus::JOYPAD_DOWN, pressed);
            break;
          case SDLK_LEFT:
            setButtonState(Bus::JOYPAD_LEFT, pressed);
            break;
          case SDLK_RIGHT:
            setButtonState(Bus::JOYPAD_RIGHT, pressed);
            break;
          default:
            break;
        }
      }
    }
  };

  uint32_t cpuTicksUntilEventPoll = 1024;

  while (running) {
    nes.cpu.tick();
    if (--cpuTicksUntilEventPoll == 0) {
      processEvents();
      cpuTicksUntilEventPoll = 1024;
      if (!running) {
        break;
      }
    }
    if (pendingNMIEdge) {
      nes.cpu.triggerNMI();
      pendingNMIEdge = false;
    }
    for (int i = 0; i < 3; i++) {
      std::unique_ptr<Frame> frame = nes.ppu.tick();
      const bool nmiState = nes.bus.ppuNMI();
      if (nmiState && !lastNMIState) {
        if (nes.cpu.completedTakenBranchLastTick()) {
          pendingNMIEdge = true;
        } else {
          nes.cpu.triggerNMI();
        }
      }
      lastNMIState = nmiState;
      if (frame) {
        render(*frame);
        processEvents();
        if (!running) {
          break;
        }

        const auto now = clock::now();
        if (now < nextFrameDeadline) {
          std::this_thread::sleep_until(nextFrameDeadline);
        } else {
          nextFrameDeadline = now;
        }
        nextFrameDeadline += frameDuration;
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
    std::unique_ptr<Frame> frame = nes.ppu.tick();
    const bool nmiState = nes.bus.ppuNMI();
    if (nmiState && !lastNMIState) {
      if (nes.cpu.completedTakenBranchLastTick()) {
        pendingNMIEdge = true;
      } else {
        nes.cpu.triggerNMI();
      }
    }
    if (pendingNMIEdge) {
      nes.cpu.triggerNMI();
      pendingNMIEdge = false;
    }
    lastNMIState = nmiState;
    if (frame) {
      render(*frame);
    }

    ppuCycleCount++;
    auto nextTick = startTime + ppuCycleCount * tickDuration;
    std::this_thread::sleep_until(nextTick);
  }
}
