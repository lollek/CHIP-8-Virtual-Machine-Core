
#include "gtest/gtest.h"
#include "chip8core/Emulator.h"

class EmulatorInitialization : public ::testing::Test, public Emulator {
};

TEST_F(EmulatorInitialization, Ram) {
  ASSERT_EQ(1U, sizeof(ram.at(0)));
  ASSERT_EQ(4096U, ram_size);
  ASSERT_EQ(ram_size, ram.size());
  for (unsigned i : ram) {
    ASSERT_EQ(0U, i);
  }
}

TEST_F(EmulatorInitialization, Screen) {
  ASSERT_EQ(8U, sizeof(screen.at(0)));
  ASSERT_EQ(32U, screen_rows);
  ASSERT_EQ(screen_rows, screen.size());
  for (unsigned i : screen) {
    ASSERT_EQ(0U, i);
  }
}

TEST_F(EmulatorInitialization, Registers) {
  ASSERT_EQ(1U, sizeof(registers.at(0)));
  ASSERT_EQ(16U, num_registers);
  ASSERT_EQ(num_registers, registers.size());
  for (unsigned i : registers) {
    ASSERT_EQ(0U, i);
  }
}

TEST_F(EmulatorInitialization, IndexRegister) {
  ASSERT_EQ(2U, sizeof(index_register));
  ASSERT_EQ(0, index_register);
}

TEST_F(EmulatorInitialization, ProgramCounter) {
  ASSERT_EQ(2U, sizeof(program_counter));
  ASSERT_EQ(0x200, program_counter);
}

TEST_F(EmulatorInitialization, SoundTimer) {
  ASSERT_EQ(1U, sizeof(sound_timer));
  ASSERT_EQ(0, sound_timer);
}

TEST_F(EmulatorInitialization, DelayTimer) {
  ASSERT_EQ(1U, sizeof(delay_timer));
  ASSERT_EQ(0, delay_timer);
}

TEST_F(EmulatorInitialization, Stack) {
  ASSERT_EQ(2U, sizeof(stack.at(0)));
  ASSERT_EQ(16U, stack_size);
  ASSERT_EQ(stack_size, stack.size());
  for (unsigned i : stack) {
    ASSERT_EQ(0U, i);
  }
}

TEST_F(EmulatorInitialization, StackPointer) {
  ASSERT_EQ(1U, sizeof(stack_pointer));
  ASSERT_EQ(0, stack_pointer);
}

TEST_F(EmulatorInitialization, KeysState) {
  ASSERT_EQ(1U, sizeof(keys_state.at(0)));
  ASSERT_EQ(16U, num_keys);
  ASSERT_EQ(num_keys, keys_state.size());
  for (unsigned i : keys_state) {
    ASSERT_EQ(0U, i);
  }
}
