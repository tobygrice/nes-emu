# NES Emulator
*Note: I am taking a break from this to focus on uni, until the midsem break starts on 14/04. The PPU is implemented but non-functional: my intention is to start the PPU from scratch and implement a solution that renders a pixel on each tick (like actual hardware), as opposed to the current set up of rendering an entire frame at once when vblank is entered.*

This NES emulator is a personal project to experiment with distributed systems and parallel programming in C++. The goal is to have each processing unit of the NES console running
in parallel with single-tick granularity, coordinated by a single shared clock (as in actual hardware).

Running an emulator with single-tick granularity will cause a huge amount of overhead due to constant context-switching between components (the PPU ticks over 5 million times per second, the CPU over 1.5 million). This project is an experiment in parallel processing to see if this overhead can be mitigated or even removed (hardware allowing). Regardless of whether this can be achieved, I am not expecting this to run at full speed, nor is this the goal: the primary target is cycle-accurate emulation at all costs. Once this goal has been reached, I will then attempt to improve efficiency.

## Development progress:
- [x] CPU official opcodes
- [x] Bus
- [x] Nintendulator formatted logs
- [x] CPU unofficial opcodes
- [x] Convert CPU to single-cycle accuracy
- [ ] PPU
- [ ] Input
- [ ] PPU Scrolling
- [ ] APU
- [ ] Mappers

## Building & Testing
To compile:
```bash
rm build/CMakeCache.txt
cmake -S . -B build
cmake --build build
```
To execute a ROM file (.nes):
``` bash
./build/nesemu game.nes > out.log
```
To run tests:
```bash
ctest --test-dir build --verbose # run all tests
ctest --test-dir build --verbose --output-on-failure -R runCPUHarteTests
ctest --test-dir build --verbose --output-on-failure -R runCPUNestest
ctest --test-dir build --verbose -R runPPUTests   # Runs only PPU tests
ctest --test-dir build --verbose -R runAPUTests   # Runs only APU tests
```
To run nestest and compare logs:
```bash
./build/nesemu rom_testing/nestest.nes > logs/actual.log
./logs/cmplogs.sh logs/nestest_ppu_exp.log logs/actual.log
```