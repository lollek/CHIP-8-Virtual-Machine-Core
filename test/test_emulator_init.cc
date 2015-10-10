
#include "gtest/gtest.h"
#include "chip8core/Emulator.h"

class EmulatorInitialization : public ::testing::Test, public Emulator {
};

TEST_F(EmulatorInitialization, Ram) {
  ASSERT_EQ(1U, sizeof(ram.at(0)));
  ASSERT_EQ(4096U, ram_size);
  ASSERT_EQ(ram_size, ram.size());

  /* 0 */
  ASSERT_EQ(0xF0, ram.at(0));
  ASSERT_EQ(0x90, ram.at(1));
  ASSERT_EQ(0x90, ram.at(2));
  ASSERT_EQ(0x90, ram.at(3));
  ASSERT_EQ(0xF0, ram.at(4));

  /* 1 */
  ASSERT_EQ(0x20, ram.at(5));
  ASSERT_EQ(0x60, ram.at(6));
  ASSERT_EQ(0x20, ram.at(7));
  ASSERT_EQ(0x20, ram.at(8));
  ASSERT_EQ(0x70, ram.at(9));

  /* 2 */
  ASSERT_EQ(0xF0, ram.at(10));
  ASSERT_EQ(0x10, ram.at(11));
  ASSERT_EQ(0xF0, ram.at(12));
  ASSERT_EQ(0x80, ram.at(13));
  ASSERT_EQ(0xF0, ram.at(14));

  /* 3 */
  ASSERT_EQ(0xF0, ram.at(15));
  ASSERT_EQ(0x10, ram.at(16));
  ASSERT_EQ(0xF0, ram.at(17));
  ASSERT_EQ(0x10, ram.at(18));
  ASSERT_EQ(0xF0, ram.at(19));

  /* 4 */
  ASSERT_EQ(0x90, ram.at(20));
  ASSERT_EQ(0x90, ram.at(21));
  ASSERT_EQ(0xF0, ram.at(22));
  ASSERT_EQ(0x10, ram.at(23));
  ASSERT_EQ(0x10, ram.at(24));

  /* 5 */
  ASSERT_EQ(0xF0, ram.at(25));
  ASSERT_EQ(0x80, ram.at(26));
  ASSERT_EQ(0xF0, ram.at(27));
  ASSERT_EQ(0x10, ram.at(28));
  ASSERT_EQ(0xF0, ram.at(29));

  /* 6 */
  ASSERT_EQ(0xF0, ram.at(30));
  ASSERT_EQ(0x80, ram.at(31));
  ASSERT_EQ(0xF0, ram.at(32));
  ASSERT_EQ(0x90, ram.at(33));
  ASSERT_EQ(0xF0, ram.at(34));

  /* 7 */
  ASSERT_EQ(0xF0, ram.at(35));
  ASSERT_EQ(0x10, ram.at(36));
  ASSERT_EQ(0x20, ram.at(37));
  ASSERT_EQ(0x40, ram.at(38));
  ASSERT_EQ(0x40, ram.at(39));

  /* 8 */
  ASSERT_EQ(0xF0, ram.at(40));
  ASSERT_EQ(0x90, ram.at(41));
  ASSERT_EQ(0xF0, ram.at(42));
  ASSERT_EQ(0x90, ram.at(43));
  ASSERT_EQ(0xF0, ram.at(44));

  /* 9 */
  ASSERT_EQ(0xF0, ram.at(45));
  ASSERT_EQ(0x90, ram.at(46));
  ASSERT_EQ(0xF0, ram.at(47));
  ASSERT_EQ(0x10, ram.at(48));
  ASSERT_EQ(0xF0, ram.at(49));

  /* A */
  ASSERT_EQ(0xF0, ram.at(50));
  ASSERT_EQ(0x90, ram.at(51));
  ASSERT_EQ(0xF0, ram.at(52));
  ASSERT_EQ(0x90, ram.at(53));
  ASSERT_EQ(0x90, ram.at(54));

  /* B */
  ASSERT_EQ(0xE0, ram.at(55));
  ASSERT_EQ(0x90, ram.at(56));
  ASSERT_EQ(0xE0, ram.at(57));
  ASSERT_EQ(0x90, ram.at(58));
  ASSERT_EQ(0xE0, ram.at(59));

  /* C */
  ASSERT_EQ(0xF0, ram.at(60));
  ASSERT_EQ(0x80, ram.at(61));
  ASSERT_EQ(0x80, ram.at(62));
  ASSERT_EQ(0x80, ram.at(63));
  ASSERT_EQ(0xF0, ram.at(64));

  /* D */
  ASSERT_EQ(0xE0, ram.at(65));
  ASSERT_EQ(0x90, ram.at(66));
  ASSERT_EQ(0x90, ram.at(67));
  ASSERT_EQ(0x90, ram.at(68));
  ASSERT_EQ(0xE0, ram.at(69));

  /* E */
  ASSERT_EQ(0xF0, ram.at(70));
  ASSERT_EQ(0x80, ram.at(71));
  ASSERT_EQ(0xF0, ram.at(72));
  ASSERT_EQ(0x80, ram.at(73));
  ASSERT_EQ(0xF0, ram.at(74));

  /* F */
  ASSERT_EQ(0xF0, ram.at(75));
  ASSERT_EQ(0x80, ram.at(76));
  ASSERT_EQ(0xF0, ram.at(77));
  ASSERT_EQ(0x80, ram.at(78));
  ASSERT_EQ(0x80, ram.at(79));

  for (unsigned i = 80; i < ram.size(); ++i) {
    ASSERT_EQ(0U, ram.at(i));
  }
}

TEST_F(EmulatorInitialization, Screen) {
  ASSERT_EQ(1U, sizeof(screen.at(0)));
  ASSERT_EQ(32U, screen_rows);
  ASSERT_EQ(8U, screen_columns);
  ASSERT_EQ(screen_rows * screen_columns, screen_bytes);
  ASSERT_EQ(screen_bytes, screen.size());
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
