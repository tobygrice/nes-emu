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

  auto processEvents = [&]() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      } else if (event.type == SDL_EVENT_KEY_DOWN ||
                 event.type == SDL_EVENT_KEY_UP) {
        switch (event.key.key) {
          case SDLK_ESCAPE:
            if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
              running = false;
            }
            break;
          default:
            break;
        }
      }
    }

    int keyCount = 0;
    const bool* keys = SDL_GetKeyboardState(&keyCount);
    auto isPressed = [&](SDL_Scancode code) -> bool {
      const int idx = static_cast<int>(code);
      return idx >= 0 && idx < keyCount && keys[idx];
    };

    joypad1State = 0;
    if (isPressed(SDL_SCANCODE_Z) || isPressed(SDL_SCANCODE_K)) {
      joypad1State |= Bus::JOYPAD_A;
    }
    if (isPressed(SDL_SCANCODE_X) || isPressed(SDL_SCANCODE_J)) {
      joypad1State |= Bus::JOYPAD_B;
    }
    if (isPressed(SDL_SCANCODE_RETURN) || isPressed(SDL_SCANCODE_KP_ENTER) ||
        isPressed(SDL_SCANCODE_SPACE)) {
      joypad1State |= Bus::JOYPAD_START;
    }
    if (isPressed(SDL_SCANCODE_TAB) || isPressed(SDL_SCANCODE_LSHIFT) ||
        isPressed(SDL_SCANCODE_RSHIFT)) {
      joypad1State |= Bus::JOYPAD_SELECT;
    }
    if (isPressed(SDL_SCANCODE_UP) || isPressed(SDL_SCANCODE_W)) {
      joypad1State |= Bus::JOYPAD_UP;
    }
    if (isPressed(SDL_SCANCODE_DOWN) || isPressed(SDL_SCANCODE_S)) {
      joypad1State |= Bus::JOYPAD_DOWN;
    }
    if (isPressed(SDL_SCANCODE_LEFT) || isPressed(SDL_SCANCODE_A)) {
      joypad1State |= Bus::JOYPAD_LEFT;
    }
    if (isPressed(SDL_SCANCODE_RIGHT) || isPressed(SDL_SCANCODE_D)) {
      joypad1State |= Bus::JOYPAD_RIGHT;
    }
    nes.bus.setJoypad1Buttons(joypad1State);
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
  nes.renderer.render(frame);
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
