# NES Emulator

This NES emulator is a personal project to experiment with distributed systems and parallel programming in C++. The goal is to have each processing unit of the NES console running
in parallel with single-tick granularity, coordinated by a single shared clock (as in actual hardware).

Running an emulator with single-tick granularity will cause a huge amount of overhead due to constant context-switching between components (the PPU ticks over 5 million times per second, the CPU over 1.5 million). 
This project is an experiment in parallel processing to see if this overhead can be mitigated or even removed (hardware allowing).

I put down this project last year to focus on university. I intend to pick it back up at some point, but for now, it remains unfinished and I have moved on to other projects.

## Development Progress:
- [x] CPU official opcodes
- [x] Bus
- [x] Nintendulator formatted logs
- [x] CPU unofficial opcodes
- [x] Convert CPU to single-cycle accuracy
- [x] PPU
- [x] Input
- [x] PPU Scrolling

## Usage
To run a ROM, use:
```bash
./nesemu <rom.nes>
```
To print CPU log to stdout, include the `--trace` flag:
```bash
./nesemu <rom.nes> --trace
```

## Building & Testing
To compile (compiles to build/release/nesemu):
```bash
cmake -S . -B build
cmake --preset release
cmake --build --preset release -j
```
If you also wish to compile tests, use:
```bash
cmake -S . -B build
cmake --build build -j
```
To run tests:
```bash
ctest --test-dir build --verbose # run all tests
ctest --test-dir build --verbose --output-on-failure -R runCPUHarteTests # runs 256,000 instructions, expect to take a while
ctest --test-dir build --verbose --output-on-failure -R runCPUNestest # runs independent of PPU
ctest --test-dir build --verbose --output-on-failure -R runPPUNestest # will fail if CPU is not correct
ctest --test-dir build --verbose --output-on-failure -R runPPUTimingTests
```
