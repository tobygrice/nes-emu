## Line-by-Line
### Visible Scanlines (0-239)
- program should _not_ access PPU memory during this time, [unless rendering is turned off](https://www.nesdev.org/wiki/PPU_registers#Mask_.28.242001.29_.3E_write "PPU registers").
**Cycle 0**:
- Idle cycle.
**Cycles 1-256**: 
In BACKGROUND rendering:
- Data for each tile is fetched:
	1. Nametable byte
	2. Attribute table byte
	3. Pattern table tile low
	4. Pattern table tile high (+8 bytes from pattern table tile low)
- Each memory access takes 2 PPU cycles to complete, and 4 must be performed per tile: 8 cycles generates 8 pixels
In SPRITE/FOREGROUND rendering (parallel process):
**Cycles 257-320**
-  tile data for the sprites on the _next_ scanline are fetched here.

## Memory
| Address Range | Size  | Description            | Mapped by       | Stored In |
| ------------- | ----- | ---------------------- | --------------- | --------- |
| $0000–$0FFF   | $1000 | Pattern table 0        | Cartridge       | CHR ROM   |
| $1000–$1FFF   | $1000 | Pattern table 1        | Cartridge       | CHR ROM   |
| $2000–$23FF   | $0400 | Nametable 0            | Cartridge       | PPU VRAM  |
| $2400–$27FF   | $0400 | Nametable 1            | Cartridge       | PPU VRAM  |
| $2800–$2BFF   | $0400 | Nametable 2            | Cartridge       | PPU VRAM  |
| $2C00–$2FFF   | $0400 | Nametable 3            | Cartridge       | PPU VRAM  |
| $3000–$3EFF   | $0F00 | Unused                 | Cartridge       |           |
| $3F00–$3F1F   | $0020 | Palette RAM indexes    | Internal to PPU |           |
| $3F20–$3FFF   | $00E0 | Mirrors of $3F00–$3F1F | Internal to PPU |           |

4 sections, modified to change graphics
### Pattern Tables (in chr_rom)
- hold raw sprite image data for a game
- 2 pattern tables (left and right) ![[Pasted image 20250327094132.png]]
- Each 64KiB of memory
- Each tile is 8px by 8px
- 256 tiles
### Nametables
- Structured as a grid of 32x30 tiles
	- Remember, each tile in pattern table is 8x8 pixels
	- 32x30 tiles = 256x240 px = NES resolution
- Grid of bytes that **reference** tiles in the pattern table
- Used to compose background for a game
#### Attribute Tables
- a small region of memory at the end of each nametable
- grid of bytes referenced by ppu to determine which palette to use when rendering a tile referenced by the nametable
- Remember, 4 active palettes for background rendering -> can be represented with two bits
- Therefore 1 byte can represent the palette selection for 4 tiles
- Tiles grouped into 2x2 blocks, palette data held as follows: ![[Pasted image 20250327100315.png]]![[Pasted image 20250327100250.png]]
- This 00-01-10-10 would be stored as 00011010 (0x1A) in the attribute table
### Colour Palettes
- PPU capable of generating 64 unique colours
- Can only work with a palette of 4 colours at once
- Palette region define up to 8 active palettes of 4 colours
	- 4 for background
	- 4 for sprites
- First value in a palette is always interpreted as the background colour (doesn't matter what value is set at that address, it will be ignored)
- 

### OAM
- Foreground graphics / sprites
- Can store data for up to 64 sprites at any given time
- Each sprite entry consists of 4 bytes
	- First byte is vertical/y coordinate of sprite
	- Second byte specifies which tile to render from the pattern table
	- Third byte controls a series of attributes
	- Fourth byte is horizontal/x coordinate
- Sprite attibutes (3rd byte)
	- Bits 0 and 1 are used to select the foreground palette (remember, 4 active palettes for foreground)
	- Bits 2,3,4 unused
	- Bit 5: 0 -> sprite rendered on top of bg, 1 -> sprite rendered below bg
	- Bit 6: flip sprite horizontally (0 not flipped)
	- Bit 7: flip sprite vertically (0 not flipped)

## CPU Interaction
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

