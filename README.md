# NES Emulator
Cycle-accurate NES emulator. Currently under development.
- [x] CPU official opcodes
- [x] Bus
- [x] Nestest format logger
- [x] CPU unofficial opcodes
- [ ] PPU
- [ ] Input
- [ ] PPU Scrolling
- [ ] APU

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
ctest --test-dir build --verbose -R runPPUTests   # Runs only PPU tests
ctest --test-dir build --verbose -R runAPUTests   # Runs only APU tests
```
To run nestest and compare logs:
```bash
./build/nesemu test_roms/nestest.nes > actual.log
awk 'NR==FNR{a[NR]=$0; next} {if ($0 != a[FNR]) {print "Difference at line", FNR; print "Actual:    " $0; print "Expected:  " a[FNR]; exit}}' expected.log actual.log
```