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
ctest --test-dir build --verbose

ctest --test-dir build --verbose -R runCustomCPUTests   # Runs only CPU tests
ctest --test-dir build --verbose --output-on-failure -R runHarteCPUTests    # Runs only Harte CPU tests
ctest --test-dir build --verbose -R PPUTests   # Runs only PPU tests
ctest --test-dir build --verbose -R APUTests   # Runs only APU tests
```