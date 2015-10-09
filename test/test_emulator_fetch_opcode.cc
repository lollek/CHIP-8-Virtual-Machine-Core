
#include "gtest/gtest.h"
#include "chip8emu/Emulator.h"

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
