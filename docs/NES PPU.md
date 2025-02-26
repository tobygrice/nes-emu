https://bugzmanov.github.io/nes_ebook/chapter_6_1.html
![[Pasted image 20250223200357.png]]
2 registers for accessing PPU memory map:
- Address (0x2006) & Data (0x2007) - provide access to the memory map available for PPU
3 registers control internal memory (OAM) that keeps the state of sprites
- OAM Address (0x2003) & OAM Data (0x2004) - Object Attribute Memory - the space responsible for sprites
- Direct Memory Access (0x4014) - for fast copying of 256 bytes from CPU RAM to OAM
3 Write-only registers are controlling PPU actions:
- Controller (0x2000) - instructs PPU on general logic flow (which memory table to use, if PPU should interrupt CPU, etc.)
- Mask (0x2001) - instructs PPU how to render sprites and background
- Scroll (0x2005) - instructs PPU how to set a viewport
One read-only register is used for reporting PPU status:
- Status 0x2002


- PPU renders 262 scanlines per frame. (0 - 240 are visible scanlines, the rest are so-called vertical overscan)
- Each scanline lasts for 341 PPU clock cycles, with each clock cycle producing one pixel. 
- The NES screen resolution is 320x240, thus scanlines 241 - 262 are not visible.
- Upon entering the 241st scanline, PPU triggers VBlank NMI on the CPU. PPU makes no memory accesses during 241-262 scanlines, so PPU memory can be freely accessed by the program. The majority of games play it safe and update the screen state only during this period, essentially preparing the view state for the next frame.


### Address (**0x2006**) and Data (**0x2007**) Registers
Say the CPU wants to access memory cell at `0x0600` PPU memory space:

It has to load the requesting address into the Addr register. It has to write to the register twice - to load 2 bytes into 1-byte register:
```6502-assembly
LDA #$06
STA $2006
LDA #$00
STA $2006
```
Note: it **doesn't** follow _little-endian_ notation.
Then, the CPU can request data load from PPU Data register (0x2007)
`LDA $2007`

> Because CHR ROM and RAM are considered external devices to PPU, PPU can't return the value immediately. PPU has to fetch the data and keep it in internal buffer.  
> The first read from 0x2007 would return the content of this internal buffer filled during the previous load operation. From the CPU perspective, this is a dummy read.
> **IMPORTANT:** This buffered reading behavior is specific only to ROM and RAM.  Reading palette data from $3F00-$3FFF works differently. The palette data is placed immediately on the data bus, and hence no dummy read is required.

CPU has to read from `0x2007` one more time to finally get the value from the PPUs internal buffer.

`LDA $2007`

Visualisation of this sequence:

![[Pasted image 20250225162224.png]]

![[Pasted image 20250225162207.png]]

