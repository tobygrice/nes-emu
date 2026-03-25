# NES Emulator

This NES emulator is a personal project to experiment with distributed systems and parallel programming in C++. The goal was to have each processing unit of the NES console running in parallel with single-tick granularity, coordinated by a single shared clock (as in actual hardware).

I found that parallelising the system used around 3 times as much compute, due to the significant overhead required for scheduling and component coordination. Furthermore, CPU schedulers are too imprecise to coordinate each component to a single tick, so component operations had to be batched. This resulted in a less performant *and* less accurate emulator. As such, I decided to revert it to a serial implementation that runs with single-tick granularity.

The emulator is functional for ROMs using iNES 1.0, with no mapper (mapper 0). There are significant scrolling bugs, so while Super Mario Bros. runs, it crashes shortly into 1-1. Early titles without scrolling run great (eg. Pacman).

## Development Progress

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
./nesemu rom.nes
```
If you want to print trace to stdout, include the `--trace` flag:
```bash
./nesemu rom.nes --trace > trace.log
```

## Controls

| Joypad | Input Key/s    |
| ------ | -------------- |
| A      | Z, J           |
| B      | X, K           |
| START  | ENTER, SPACE   |
| SELECT | TAB, SHIFT     |
| ↑      | W, Up arrow    |
| ↓      | S, Down arrow  |
| ←      | A, Left arrow  |
| →      | D, Right arrow |


## Building & Testing

Compilation uses cmake:
```bash
cmake -S . -B build
cmake --build build -j
```

To run all tests:
```bash
ctest --test-dir build --verbose
```
To run specific tests:
```bash
ctest --test-dir build --verbose --output-on-failure -R runCPUHarteTests # runs 2,560,000 instructions, expect to take a while
ctest --test-dir build --verbose --output-on-failure -R runCPUNestest # runs independent of PPU
ctest --test-dir build --verbose --output-on-failure -R runPPUNestest # will fail if CPU is not correct
ctest --test-dir build --verbose --output-on-failure -R runPPUTimingTests
```
