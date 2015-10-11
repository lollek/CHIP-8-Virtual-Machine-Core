
#include "gtest/gtest.h"
#include "chip8core/Emulator.h"

class EmulatorFetchOpcode : public ::testing::Test, public Emulator {
};

TEST_F(EmulatorFetchOpcode, DefaultNoData) {
  ASSERT_EQ(0x200, program_counter);
  ASSERT_EQ(0U, fetchOpcode());
  ASSERT_EQ(0x202, program_counter);
}

TEST_F(EmulatorFetchOpcode, OutsideRAM) {
  program_counter = -2;
  ASSERT_EQ(static_cast<uint16_t>(-2), program_counter);
  ASSERT_THROW(fetchOpcode(), FatalError);
}

TEST_F(EmulatorFetchOpcode, JustPastRam) {
  program_counter = 4096;
  ASSERT_EQ(4096, program_counter);
  ASSERT_THROW(fetchOpcode(), FatalError);
}

TEST_F(EmulatorFetchOpcode, JustInsideRAMLoops) {
  program_counter = 4094;
  ASSERT_EQ(4094, program_counter);
  ASSERT_EQ(0U, fetchOpcode());
  ASSERT_EQ(0, program_counter);
}

TEST_F(EmulatorFetchOpcode, UnevenProgramCounter) {
  program_counter++;
  ASSERT_EQ(0x201, program_counter);
  ASSERT_THROW(fetchOpcode(), FatalError);
}

TEST_F(EmulatorFetchOpcode, ReadDataCorrectly) {
  bool status = loadFileToRam("../test/atof.txt");
  ASSERT_EQ(true, status);

  EXPECT_EQ(0x00, ram.at(0x200 - 2));
  EXPECT_EQ(0x00, ram.at(0x200 - 1));
  EXPECT_EQ(0x01, ram.at(0x200 + 0));
  EXPECT_EQ(0x23, ram.at(0x200 + 1));
  EXPECT_EQ(0x45, ram.at(0x200 + 2));
  EXPECT_EQ(0x67, ram.at(0x200 + 3));
  EXPECT_EQ(0x89, ram.at(0x200 + 4));
  EXPECT_EQ(0xAB, ram.at(0x200 + 5));
  EXPECT_EQ(0xCD, ram.at(0x200 + 6));
  EXPECT_EQ(0xEF, ram.at(0x200 + 7));

  EXPECT_EQ(0x0123, fetchOpcode());
  EXPECT_EQ(0x4567, fetchOpcode());
  EXPECT_EQ(0x89ab, fetchOpcode());
  EXPECT_EQ(0xcdef, fetchOpcode());
}
