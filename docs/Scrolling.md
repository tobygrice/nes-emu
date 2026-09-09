# Scrolling and stability investigation

The original emulator could start Super Mario Bros. but stopped at the first horizontal nametable crossing. A headless run of the original code with the local NTSC ROM reproduced a soft lock at camera X = 256, around frame 384 of the controller script. The CPU kept polling sprite zero at `$8150/$8153/$8155`; the game frame counter and player position stopped advancing. The process itself did not throw or exit.

## Causes and corrections

- The background renderer added raw scroll values to screen coordinates, ignored fine X when selecting pattern bits, and emitted two pixels on only four of each eight PPU dots. It now fetches background tiles into shift registers and emits one pixel per visible dot. Attribute bits travel with the corresponding tile.
- `$2005` and `$2006` had separate write latches, and `$2000` nametable selection affected rendering directly. They now share the hardware's `v`, `t`, `x`, and `w` state. Horizontal scroll reloads at dot 257, vertical scroll reloads on the pre-render line, and coarse X/Y wrap according to the PPU rules. This preserves the status bar when Mario changes horizontal scroll after sprite zero.
- Indexed CPU stores could invoke the store handler twice, producing two writes to registers with side effects. Stores now write once at the correct cycle; indexed dummy accesses and read-modify-write dummy writes are modeled explicitly.
- OAM DMA copied bytes immediately and did not pause the CPU. It now waits for a CPU read cycle and transfers on alternating CPU clocks, taking 513 or 514 clocks while the PPU continues running.
- Sprites were overlaid at the end of the frame using the final register and palette state. They are now composed per pixel from the first eight sprites selected for each scanline. Sprite-zero detection uses the same visible background and sprite pixels, including fine scroll, clipping, and the X = 255 exclusion.

Related fixes include palette read-buffer behavior, CHR-ROM write protection, four-screen nametable storage, cartridge validation, PRG RAM and trainer loading, and a frame-buffer overflow check. The SDL renderer now uploads the native 256 by 240 frame and uses nearest-neighbor integer scaling instead of allocating and filling a second enlarged image on the CPU.

## Regression coverage

`runPPUScrollingTests` exercises synthetic patterns and register writes so scrolling behavior can be checked without a commercial ROM. `runCoreRegressionTests` checks observable CPU bus accesses, DMA timing, and cartridge behavior. The existing CPU/PPU nestest traces and documented-opcode Harte cases remain part of the suite.

`runROMSmokeTests` is optional and uses local ROMs only. Its Mario script checks both visible frame production and continued game logic updates; producing frames alone would miss the original soft lock. The test requires the camera to cross a nametable boundary and cover all eight fine-scroll offsets.

With the fixes, the same 1,800-frame Mario run reaches camera X = 1,001, records sprite-zero hits in 1,463 frames, and continues updating the game frame counter through the end. A separate MSVC AddressSanitizer run completes the same script without memory errors. This script includes ordinary deaths and restarts rather than completing the level.

The Pac-Man script presses Start twice (once to skip its title animation, once to start play), then runs for 900 frames. Both the original and corrected versions animate in 111 of the final 120 frames, confirming that the non-scrolling game continues to work.

Validation on Windows: all eight CTest targets pass in Release, including the 1,510,000 documented-opcode Harte cases. Debug passes the other seven targets. Both MSVC builds and a GCC syntax/warning check are clean. Native SDL texture presentation and repeated resource cleanup pass with SDL's dummy video driver; an emulator-only build with static SDL also builds and starts successfully.

## Remaining limits

This is not a fully cycle-accurate NES implementation. Sprite evaluation is batched at dot 257 rather than reproducing every OAM bus operation, and the sprite-overflow hardware bug is not modeled. The existing vblank race approximation remains. PAL scanline counts and the PAL CPU/PPU clock ratio, audio, additional mappers, and palette emphasis are still unimplemented. The automated gameplay check is bounded and does not prove every level or ROM works.

## References

- [NESdev: PPU scrolling](https://www.nesdev.org/wiki/PPU_scrolling)
- [NESdev: PPU rendering](https://www.nesdev.org/wiki/PPU_rendering)
- [NESdev: DMA](https://www.nesdev.org/wiki/DMA)
- [Super Mario Bros. disassembly and RAM symbols](https://6502disassembly.com/nes-smb/SuperMarioBros.html)
