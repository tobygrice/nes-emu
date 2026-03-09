#include "../include/Clock.h"

#include <SDL3/SDL.h>
#include <cstdint>

#include "../include/NES.h"

Clock::Clock(NES &nes)
    : nes(nes), region(NESRegion::None), running(false), lastNMIState(false),
      pendingNMIEdge(false),
      frameDuration(std::chrono::steady_clock::duration::zero()) {}

void Clock::setRegion(NESRegion region) {
    if (region == NESRegion::None) {
        return;
    }

    this->region = region;

    double cycles_per_frame = (region == NESRegion::NTSC) ? 29780.5 : 33247.5;
    double cpuHz = (region == NESRegion::NTSC) ? MASTER_SPEED_NTSC / 12.0
                                               : MASTER_SPEED_PAL / 16.0;

    // calculate frame duration
    using clock = std::chrono::steady_clock;
    const double framerate = cpuHz / cycles_per_frame;
    this->frameDuration = std::chrono::duration_cast<clock::duration>(
        std::chrono::duration<double>(1.0 / framerate));
}

// start game loop
void Clock::start() {
    if (region == NESRegion::None) {
        throw std::runtime_error("No region set");
    } else {
        running = true;
        lastNMIState = false;
        pendingNMIEdge = false;
        gameLoop();
    }
}

void Clock::gameLoop() {
    using clock = std::chrono::steady_clock;
    auto nextFrameDeadline = clock::now() + frameDuration;

    uint32_t cpuTicksUntilEventPoll = 1024;

    while (running) {
        // tick CPU
        nes.cpu.tick();
        // process input events every 1024 CPU ticks
        if (--cpuTicksUntilEventPoll == 0) {
            this->processEvents();
            cpuTicksUntilEventPoll = 1024;
            if (!running) {
                break;
            }
        }
        // trigger NMI if pending
        if (pendingNMIEdge) {
            nes.cpu.triggerNMI();
            pendingNMIEdge = false;
        }

        // tick PPU three times for each CPU tick
        for (int i = 0; i < 3; i++) {
            std::unique_ptr<Frame> frame = nes.ppu.tick();
            const bool nmiState = nes.bus.ppuNMI();
            // check if NMI has just been raised:
            if (nmiState && !lastNMIState) {
                // CPU timing quirk: NMI is not actioned if the CPU completed a
                // branch on the previous tick, mark pending
                if (nes.cpu.completedTakenBranchLastTick()) {
                    pendingNMIEdge = true;
                } else {
                    nes.cpu.triggerNMI();
                }
            }
            lastNMIState = nmiState;
            if (frame) {
                // ppu has generated a new frame, render it and process events
                render(*frame);
                this->processEvents();
                if (!running) {
                    break;
                }
                // maintain frame timing:
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

// read inputs
void Clock::processEvents() {
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
    const bool *keys = SDL_GetKeyboardState(&keyCount);
    auto isPressed = [&](SDL_Scancode code) -> bool {
        const int idx = static_cast<int>(code);
        return idx >= 0 && idx < keyCount && keys[idx];
    };

    uint8_t joypad1State = 0;
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
}

void Clock::render(const Frame &frame) { nes.renderer.render(frame); }
