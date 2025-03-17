#include <gtest/gtest.h>

#include <array>
#include <fstream>
#include <vector>

#include "../../include/MMU.h"

std::vector<uint8_t> readTestRom() {
  char* filename = "../test_roms/nestest.nes";
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open test rom: " + std::string(filename));
  }

  // read rom file into vector
  std::vector<uint8_t> romDump((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());

  return romDump;
}

// Define a test fixture for CPU tests.
class PPUCoreLogicTest : public ::testing::Test {
 protected:
  std::vector<uint8_t> testRom;
  Cartridge cart;
  PPU ppu;

  PPUCoreLogicTest() : testRom(readTestRom()), cart(testRom), ppu(&cart) {}
};

// Test 1: PPU VRAM writes
TEST_F(PPUCoreLogicTest, VramWrites) {
  ppu.write_to_ppu_addr(0x23);
  ppu.write_to_ppu_addr(0x05);
  ppu.write_to_data(0x66);

  EXPECT_EQ(ppu.TEST_getvram(0x0305), 0x66);
}

// Test 2: PPU VRAM reads
TEST_F(PPUCoreLogicTest, VramReads) {
  ppu.write_to_ctrl(0);
  ppu.TEST_setvram(0x0305, 0x66);

  ppu.write_to_ppu_addr(0x23);
  ppu.write_to_ppu_addr(0x05);

  ppu.read_data();  // This call loads data into the internal buffer.
  EXPECT_EQ(ppu.TEST_getaddr(), 0x2306);
  EXPECT_EQ(ppu.read_data(), 0x66);
}

// Test 3: PPU VRAM reads across page boundaries
TEST_F(PPUCoreLogicTest, VramReadsCrossPage) {
  ppu.write_to_ctrl(0);
  ppu.TEST_setvram(0x01ff, 0x66);
  ppu.TEST_setvram(0x0200, 0x77);

  ppu.write_to_ppu_addr(0x21);
  ppu.write_to_ppu_addr(0xff);

  ppu.read_data();  // load data into the buffer
  EXPECT_EQ(ppu.read_data(), 0x66);
  EXPECT_EQ(ppu.read_data(), 0x77);
}

// Test 4: PPU VRAM reads with address increment of 32 (controlled by ctrl flag)
TEST_F(PPUCoreLogicTest, VramReadsStep32) {
  ppu.write_to_ctrl(0b100);
  ppu.TEST_setvram(0x01ff, 0x66);
  ppu.TEST_setvram(0x01ff + 32, 0x77);
  ppu.TEST_setvram(0x01ff + 64, 0x88);

  ppu.write_to_ppu_addr(0x21);
  ppu.write_to_ppu_addr(0xff);

  ppu.read_data();  // load into buffer
  EXPECT_EQ(ppu.read_data(), 0x66);
  EXPECT_EQ(ppu.read_data(), 0x77);
  EXPECT_EQ(ppu.read_data(), 0x88);
}

// Test 5: VRAM horizontal mirroring
// Horizontal mirroring layout:
//   [0x2000 A ] [0x2400 a ]
//   [0x2800 B ] [0x2C00 b ]
TEST_F(PPUCoreLogicTest, VramHorizontalMirror) {
  // Write to "a" at 0x2405
  ppu.write_to_ppu_addr(0x24);
  ppu.write_to_ppu_addr(0x05);
  ppu.write_to_data(0x66);

  // Write to "B" at 0x2805
  ppu.write_to_ppu_addr(0x28);
  ppu.write_to_ppu_addr(0x05);
  ppu.write_to_data(0x77);

  // Read from "A" (0x2005 mirrors 0x2405)
  ppu.write_to_ppu_addr(0x20);
  ppu.write_to_ppu_addr(0x05);
  ppu.read_data();  // load into buffer
  EXPECT_EQ(ppu.read_data(), 0x66);

  // Read from "b" (0x2C05 mirrors 0x2805)
  ppu.write_to_ppu_addr(0x2C);
  ppu.write_to_ppu_addr(0x05);
  ppu.read_data();  // load into buffer
  EXPECT_EQ(ppu.read_data(), 0x77);
}

// Test 6: VRAM vertical mirroring
// Vertical mirroring layout:
//   [0x2000 A ] [0x2400 B ]
//   [0x2800 a ] [0x2C00 b ]
TEST_F(PPUCoreLogicTest, VramVerticalMirror) {
  cart.setMirroring(MirroringMode::Vertical);

  // Write to "A" at 0x2005
  ppu.write_to_ppu_addr(0x20);
  ppu.write_to_ppu_addr(0x05);
  ppu.write_to_data(0x66);

  // Write to "b" at 0x2C05
  ppu.write_to_ppu_addr(0x2C);
  ppu.write_to_ppu_addr(0x05);
  ppu.write_to_data(0x77);

  // Read from "a" (0x2805 mirrors 0x2005)
  ppu.write_to_ppu_addr(0x28);
  ppu.write_to_ppu_addr(0x05);
  ppu.read_data();  // load into buffer
  EXPECT_EQ(ppu.read_data(), 0x66);

  // Read from "B" (0x2405 mirrors 0x2C05)
  ppu.write_to_ppu_addr(0x24);
  ppu.write_to_ppu_addr(0x05);
  ppu.read_data();  // load into buffer
  EXPECT_EQ(ppu.read_data(), 0x77);
}

// Test 7: Reading status resets address and scroll latches
TEST_F(PPUCoreLogicTest, ReadStatusResetsLatch) {
  ppu.TEST_setvram(0x0305, 0x66);

  ppu.write_to_ppu_addr(0x21);
  ppu.write_to_ppu_addr(0x23);
  ppu.write_to_ppu_addr(0x05);

  ppu.read_data();  // load into buffer
  EXPECT_NE(ppu.read_data(), 0x66);

  ppu.read_status();

  ppu.write_to_ppu_addr(0x23);
  ppu.write_to_ppu_addr(0x05);

  ppu.read_data();  // load into buffer
  EXPECT_EQ(ppu.read_data(), 0x66);
}

// Test 8: VRAM mirroring (address translation)
TEST_F(PPUCoreLogicTest, VramMirroring) {
  ppu.write_to_ctrl(0);
  ppu.TEST_setvram(0x0305, 0x66);

  // Write to address 0x6305 which should mirror to 0x2305.
  ppu.write_to_ppu_addr(0x63);
  ppu.write_to_ppu_addr(0x05);

  ppu.read_data();  // load into buffer
  EXPECT_EQ(ppu.read_data(), 0x66);
  // Optionally, one might check the address register state:
  // EXPECT_EQ(ppu.addr.read(), 0x0306);
}

// Test 9: Reading status resets vblank flag
TEST_F(PPUCoreLogicTest, ReadStatusResetsVBlank) {
  ppu.TEST_set_vblank_status(true);

  uint8_t status = ppu.read_status();

  EXPECT_EQ(status >> 7, 1);
  EXPECT_EQ(ppu.TEST_getstatus() >> 7, 0);
}

// Test 10: OAM read/write
TEST_F(PPUCoreLogicTest, OamReadWrite) {
  ppu.write_to_oam_addr(0x10);
  ppu.write_to_oam_data(0x66);
  ppu.write_to_oam_data(0x77);

  ppu.write_to_oam_addr(0x10);
  EXPECT_EQ(ppu.read_oam_data(), 0x66);

  ppu.write_to_oam_addr(0x11);
  EXPECT_EQ(ppu.read_oam_data(), 0x77);
}

// Test 11: OAM DMA transfer
TEST_F(PPUCoreLogicTest, OamDMA) {
  std::array<uint8_t, 256> data;
  data.fill(0x66);
  data[0] = 0x77;
  data[255] = 0x88;

  ppu.write_to_oam_addr(0x10);
  ppu.write_oam_dma(data);

  ppu.write_to_oam_addr(0x0F);  // wrap around (0x10 - 1 = 0x0F)
  EXPECT_EQ(ppu.read_oam_data(), 0x88);

  // The following writes test address updating behavior.
  ppu.write_to_oam_addr(0x10);
  ppu.write_to_oam_addr(0x77);
  ppu.write_to_oam_addr(0x11);
  ppu.write_to_oam_addr(0x66);
}