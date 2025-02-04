# NES Emulator
Placeholder

## Building & Testing
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build

ctest --test-dir build -R CPUTests   # Runs only CPU tests
ctest --test-dir build -R PPUTests   # Runs only PPU tests
ctest --test-dir build -R APUTests   # Runs only APU tests
```