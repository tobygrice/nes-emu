# NES Emulator

This NES emulator is a personal project to experiment with distributed systems and parallel programming in C++. The goal is to have each processing unit of the NES console running
in parallel with single-tick granularity, coordinated by a single shared clock (as in actual hardware).

Running an emulator with single-tick granularity will cause a huge amount of overhead due to constant context-switching between components (the PPU ticks over 5 million times per second, the CPU over 1.5 million). 
This project is an experiment in parallel processing to see if this overhead can be mitigated or even removed (hardware allowing).

I put down this project last year to focus on university. I intend to pick it back up at some point, but for now, it remains unfinished and I have moved on to other projects.

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
