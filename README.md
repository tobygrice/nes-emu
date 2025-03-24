# NES Emulator
*Note: I am taking a break from this to focus on uni, until the midsem break starts on 14/04. The PPU is implemented but non-functional: my intention is to start the PPU from scratch and implement a solution that renders a pixel on each tick (like actual hardware), as opposed to the current set up of rendering an entire frame at once when vblank is entered.*

This NES emulator is a personal project to experiment with distributed systems and parallel programming in C++. The goal is to have each processing unit of the NES console running
in parallel with single-tick granularity, coordinated by a single shared clock (as in actual hardware).

Running an emulator on a single-tick basis will cause a huge amount of overhead (the PPU ticks over 5 million times per second). Running at a full 60fps would be fantastic but is ultimately not the goal: the intention is primarily to emulate the original hardware as closely as possible. Once accurate emulation has been achieved, I will shift focus towards efficiency.

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
ctest --test-dir build --verbose --output-on-failure -R runCPUTests
ctest --test-dir build --verbose --output-on-failure -R runCPUNestest
ctest --test-dir build --verbose -R runPPUTests   # Runs only PPU tests
ctest --test-dir build --verbose -R runAPUTests   # Runs only APU tests
```
To run nestest and compare logs:
```bash
./build/nesemu test_roms/nestest.nes > logs/actual.log
./logs/cmplogs.sh logs/nestest_cpu_exp.log logs/nestest_cpu_act.log
```