# NES Emulator
Cycle-accurate NES emulator. Under development:
- [x] CPU
- [ ] MMU (bus) + cycle counting
- [ ] PPU
- [ ] Input
- [ ] PPU Scrolling
- [ ] APU

Cycle count testing in branching test, flag test, and logical test.

## Building & Testing
```bash
rm build/CMakeCache.txt
cmake -S . -B build
cmake --build build
./build/nesemu nestest.nes > actual.log
awk 'NR==FNR{a[NR]=$0; next} {if ($0 != a[FNR]) {print "Difference at line", FNR; print "Actual:    " $0; print "Expected:  " a[FNR]; exit}}' expected.log actual.log

ctest --test-dir build --verbose # run all tests
ctest --test-dir build --verbose -R runCustomCPUTests   # Runs only CPU tests
ctest --test-dir build --verbose --output-on-failure -R runHarteCPUTests    # Runs only Harte CPU tests
ctest --test-dir build --verbose -R PPUTests   # Runs only PPU tests
ctest --test-dir build --verbose -R APUTests   # Runs only APU tests
```