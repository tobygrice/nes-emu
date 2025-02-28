# NES Emulator
Cycle-accurate NES emulator. Currently under development.
- [x] CPU official opcodes
- [x] MMU (bus)
- [x] Nestest format logger
- [ ] CPU unofficial opcodes
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
ctest --test-dir build --verbose -R runCustomCPUTests   # Runs only CPU tests
ctest --test-dir build --verbose --output-on-failure -R runHarteCPUTests    # Runs only Harte CPU tests
ctest --test-dir build --verbose -R PPUTests   # Runs only PPU tests
ctest --test-dir build --verbose -R APUTests   # Runs only APU tests
```
To run nestest and compare logs:
```bash
./build/nesemu nestest.nes > actual.log
awk 'NR==FNR{a[NR]=$0; next} {if ($0 != a[FNR]) {print "Difference at line", FNR; print "Actual:    " $0; print "Expected:  " a[FNR]; exit}}' expected.log actual.log
```