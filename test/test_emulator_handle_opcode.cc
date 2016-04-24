
#include "gtest/gtest.h"
#include "chip8core/Emulator.h"

class EmulatorHandleOpcode : public ::testing::Test, public Emulator {
};

TEST_F(EmulatorHandleOpcode, OP_0x00E0) {
  for (unsigned i = 0; i < screen.size(); ++i) {
    screen.at(i) = 57;
  }

  handleOpcode(0x00E0);
  for (unsigned i = 0; i < screen.size(); ++i) {
    ASSERT_EQ(0U, screen.at(i));
  }
}

TEST_F(EmulatorHandleOpcode, OP_0x00EE) {
  ASSERT_EQ(0U, stack_pointer);
  ASSERT_EQ(0U, stack.at(stack_pointer));
  ASSERT_THROW(handleOpcode(0x00EE), FatalError);

  stack.at(stack_pointer) = 1337U;
  ++stack_pointer;
  ASSERT_EQ(0x200U, program_counter);
  handleOpcode(0x00EE);
  ASSERT_EQ(1337U, program_counter);
  ASSERT_EQ(0U, stack_pointer);
  ASSERT_EQ(1337U, stack.at(stack_pointer));
}

TEST_F(EmulatorHandleOpcode, OP_0x1NNN) {
  for (halfword op = 0x1000; op < 0x2000; ++op) {
    handleOpcode(op);
    ASSERT_EQ(op - 0x1000, program_counter);
  }
}

TEST_F(EmulatorHandleOpcode, OP_0x2NNN) {
  ASSERT_EQ(0U, stack_pointer);
  for (halfword op = 0x2000; op < 0x3000; ++op) {
    ASSERT_NE(op & 0x0FFF, program_counter);

    /* Test normally */
    if (stack_pointer < stack_size - 1) {
      halfword last_program_counter = program_counter;
      ASSERT_EQ(op % stack_size, stack_pointer);
      handleOpcode(op);
      ASSERT_EQ(stack.at(stack_pointer -1), last_program_counter);

    /* Every 16th op we try a stack overflow instead */
    } else {
      ++stack_pointer;
      ASSERT_THROW(handleOpcode(op), FatalError);
      stack_pointer = 0;
    }
  }
}

TEST_F(EmulatorHandleOpcode, OP_0x3XNN) {
  ASSERT_EQ(0x200, program_counter);

  for (unsigned i = 0; i < registers.size(); ++i) {
    program_counter = 0x200;
    registers.at(i) = i;
    handleOpcode(0x3000 + (i << 8) + i);
    ASSERT_EQ(0x202, program_counter);
    handleOpcode(0x3000 + (i << 8) + i + 1);
    ASSERT_EQ(0x202, program_counter);
  }
}

TEST_F(EmulatorHandleOpcode, OP_0x4XNN) {
  ASSERT_EQ(0x200, program_counter);

  for (unsigned i = 0; i < registers.size(); ++i) {
    program_counter = 0x200;
    registers.at(i) = i;
    handleOpcode(0x4000 + (i << 8) + i);
    ASSERT_EQ(0x200, program_counter);
    handleOpcode(0x4000 + (i << 8) + i + 1);
    ASSERT_EQ(0x202, program_counter);
  }
}

TEST_F(EmulatorHandleOpcode, OP_0x5XY0) {
  ASSERT_EQ(0x200, program_counter);

  registers.at(0) = 42;
  registers.at(1) = 42;

  handleOpcode(0x5010);
  ASSERT_EQ(0x202, program_counter);
  handleOpcode(0x5100);
  ASSERT_EQ(0x204, program_counter);

  handleOpcode(0x5000);
  ASSERT_EQ(0x206, program_counter);
  handleOpcode(0x5110);
  ASSERT_EQ(0x208, program_counter);
  handleOpcode(0x5130);
  ASSERT_EQ(0x208, program_counter);
}

TEST_F(EmulatorHandleOpcode, OP_0x6XNN) {
  for (unsigned i = 0; i < registers.size(); ++i) {
    ASSERT_EQ(0U, registers.at(i));
    handleOpcode(0x6000 + (i << 8) + i + 5);
    ASSERT_EQ(i + 5, registers.at(i));
  }
}

TEST_F(EmulatorHandleOpcode, OP_0x7XNN) {
  for (unsigned i = 0; i < registers.size(); ++i) {
    ASSERT_EQ(0U, registers.at(i));
    handleOpcode(0x7000 + (i << 8) + i + 5);
    ASSERT_EQ(i + 5, registers.at(i));
    handleOpcode(0x7000 + (i << 8) + 1);
    ASSERT_EQ(i + 6, registers.at(i));
  }
}

TEST_F(EmulatorHandleOpcode, OP_0x8XY0) {
  ASSERT_EQ(0U, registers.at(0));
  registers.at(0) = 255;

  for (unsigned i = 1; i < registers.size(); ++i) {
    ASSERT_EQ(0U, registers.at(i));
    handleOpcode(0x8000 + (i << 8) + ((i - 1) << 4));
    ASSERT_EQ(255U, registers.at(i));
  }
}

TEST_F(EmulatorHandleOpcode, OP_0x8XY1) {
  ASSERT_EQ(0U, registers.at(0));
  registers.at(0) = 255;

  for (unsigned i = 1; i < registers.size(); ++i) {
    ASSERT_EQ(0U, registers.at(i));
    handleOpcode(0x8001 + (i << 8) + ((i - 1) << 4));
    ASSERT_EQ(255U, registers.at(i));
  }

  registers.at(0) = 0xF0;
  registers.at(1) = 0x0F;
  handleOpcode(0x8011);
  ASSERT_EQ(255U, registers.at(0));
}

TEST_F(EmulatorHandleOpcode, OP_0x8XY2) {
  ASSERT_EQ(0U, registers.at(0));
  registers.at(0) = 255;

  for (unsigned i = 1; i < registers.size(); ++i) {
    ASSERT_EQ(0U, registers.at(i));
    handleOpcode(0x8002 + (i << 8) + ((i - 1) << 4));
    ASSERT_EQ(0, registers.at(i));
  }

  registers.at(0) = 0xF0;
  registers.at(1) = 0x0F;
  handleOpcode(0x8012);
  ASSERT_EQ(0U, registers.at(0));

  registers.at(0) = 0xF0;
  registers.at(1) = 0x3F;
  handleOpcode(0x8012);
  ASSERT_EQ(0x30U, registers.at(0));
}

TEST_F(EmulatorHandleOpcode, OP_0x8XY3) {
  handleOpcode(0x8003);
  ASSERT_EQ(0U, registers.at(0));

  registers.at(1) = 0xF0;
  handleOpcode(0x8113);
  ASSERT_EQ(0U, registers.at(1));

  registers.at(2) = 0xF0;
  registers.at(3) = 0x3F;
  handleOpcode(0x8233);
  ASSERT_EQ(0xCF, registers.at(2));
  ASSERT_EQ(0x3F, registers.at(3));
}

TEST_F(EmulatorHandleOpcode, OP_0x8XY4) {
  registers.at(0xF) = 255;

  registers.at(0) = 10;
  registers.at(1) = 20;
  handleOpcode(0x8014);
  ASSERT_EQ(30, registers.at(0));
  ASSERT_EQ(0, registers.at(0xF));

  registers.at(2) = 255;
  registers.at(3) = 255;
  handleOpcode(0x8234);
  ASSERT_EQ(254, registers.at(2));
  ASSERT_EQ(1, registers.at(0xF));

  registers.at(4) = 1;
  registers.at(5) = 255;
  handleOpcode(0x8454);
  ASSERT_EQ(0, registers.at(4));
  ASSERT_EQ(1, registers.at(0xF));
}

TEST_F(EmulatorHandleOpcode, OP_0x8XY5) {
  registers.at(0xF) = 255;

  registers.at(0) = 10;
  registers.at(1) = 20;
  handleOpcode(0x8015);
  ASSERT_EQ(246, registers.at(0));
  ASSERT_EQ(0, registers.at(0xF));

  registers.at(2) = 255;
  registers.at(3) = 255;
  handleOpcode(0x8235);
  ASSERT_EQ(0, registers.at(2));
  ASSERT_EQ(1, registers.at(0xF));

  registers.at(4) = 1;
  registers.at(5) = 255;
  handleOpcode(0x8455);
  ASSERT_EQ(2, registers.at(4));
  ASSERT_EQ(0, registers.at(0xF));

  registers.at(6) = 1;
  registers.at(7) = 0;
  handleOpcode(0x8675);
  ASSERT_EQ(1, registers.at(6));
  ASSERT_EQ(1, registers.at(0xF));
}

TEST_F(EmulatorHandleOpcode, OP_0x8XY6) {
  registers.at(0xF) = 255;
  registers.at(0) = 1;
  handleOpcode(0x8006);
  ASSERT_EQ(0, registers.at(0));
  ASSERT_EQ(1, registers.at(0xF));

  registers.at(0xF) = 255;
  registers.at(0) = 2;
  handleOpcode(0x8006);
  ASSERT_EQ(1, registers.at(0));
  ASSERT_EQ(0, registers.at(0xF));
}

TEST_F(EmulatorHandleOpcode, OP_0x8XY7) {
  registers.at(0xF) = 255;
  registers.at(0) = 0;
  registers.at(1) = 1;
  handleOpcode(0x8017);
  ASSERT_EQ(1, registers.at(0));
  ASSERT_EQ(1, registers.at(1));
  ASSERT_EQ(1, registers.at(0xF));

  registers.at(0) = 1;
  registers.at(1) = 0;
  handleOpcode(0x8017);
  ASSERT_EQ(255, registers.at(0));
  ASSERT_EQ(0, registers.at(1));
  ASSERT_EQ(0, registers.at(0xF));
}

TEST_F(EmulatorHandleOpcode, OP_0x8XYE) {
  registers.at(0xF) = 255;
  registers.at(0) = 0;
  handleOpcode(0x800E);
  ASSERT_EQ(0, registers.at(0));
  ASSERT_EQ(0, registers.at(0xF));

  registers.at(1) = 0x80;
  handleOpcode(0x810E);
  ASSERT_EQ(0, registers.at(1));
  ASSERT_EQ(1, registers.at(0xF));

  registers.at(2) = 0x70;
  handleOpcode(0x820E);
  ASSERT_EQ(0xE0, registers.at(2));
  ASSERT_EQ(0, registers.at(0xF));
}

TEST_F(EmulatorHandleOpcode, OP_0x9XY0) {
  ASSERT_EQ(0x200, program_counter);
  registers.at(0) = 42;
  registers.at(1) = 43;
  handleOpcode(0x9010);
  ASSERT_EQ(0x202, program_counter);

  registers.at(2) = 255;
  handleOpcode(0x9210);
  ASSERT_EQ(0x204, program_counter);

  ASSERT_EQ(0x204, program_counter);
  handleOpcode(0x9000);
  handleOpcode(0x9110);
  handleOpcode(0x9220);
  handleOpcode(0x9330);
  handleOpcode(0x9440);
  handleOpcode(0x9550);
  handleOpcode(0x9660);
  handleOpcode(0x9770);
  handleOpcode(0x9880);
  handleOpcode(0x9990);
  handleOpcode(0x9AA0);
  handleOpcode(0x9BB0);
  handleOpcode(0x9CC0);
  handleOpcode(0x9DD0);
  handleOpcode(0x9EE0);
  handleOpcode(0x9FF0);
  ASSERT_EQ(0x204, program_counter);
}

TEST_F(EmulatorHandleOpcode, OP_0xANNN) {
  for (unsigned i = 0; i < 0x1000; ++i) {
    handleOpcode(0xA000 + i);
    ASSERT_EQ(i, index_register);
  }
}

TEST_F(EmulatorHandleOpcode, OP_0xBNNN) {
  for (unsigned i = 0; i < 0x1000; ++i) {
    handleOpcode(0xB000 + i);
    ASSERT_EQ(i, program_counter);
  }

  registers.at(0) = 200;
  for (unsigned i = 0; i < 0x1000; ++i) {
    handleOpcode(0xB000 + i);
    ASSERT_EQ((i + 200) % 0x1000, program_counter);
  }
}

TEST_F(EmulatorHandleOpcode, OP_0xCXNN) {
  for (unsigned i = 0; i < registers.size(); ++i) {
    registers.at(i) = 42;
    handleOpcode(0xC000 + (i << 8));
    ASSERT_EQ(0U, registers.at(i));
  }

  for (unsigned i = 0; i < registers.size(); ++i) {
    registers.at(i) = 255;
    handleOpcode(0xC00F + (i << 8));
    ASSERT_TRUE(0xF >= registers.at(i));
  }
}

TEST_F(EmulatorHandleOpcode, OP_0xDXYN) {
  ram.at(index_register) = 0x12;
  ram.at(index_register + 1) = 0x34;
  ram.at(index_register + 2) = 0x56;
  ram.at(index_register + 3) = 0x78;

  /* Draw a byte-aligned line at the corner */
  ASSERT_EQ(0U, screen.at(0));
  handleOpcode(0xD001);
  ASSERT_EQ(0x12, screen.at(0));
  ASSERT_EQ(0, registers.at(0xF));

  /* Drawing the exact same byte should reverse the drawing */
  handleOpcode(0xD001);
  ASSERT_EQ(0U, screen.at(0));
  ASSERT_EQ(1, registers.at(0xF));

  /* Draw a few lines which are not byte-aligned */
  ASSERT_EQ(0U, screen.at(0 + (1 * screen_columns)));
  ASSERT_EQ(0U, screen.at(1 + (1 * screen_columns)));
  ASSERT_EQ(0U, screen.at(0 + (2 * screen_columns)));
  ASSERT_EQ(0U, screen.at(1 + (2 * screen_columns)));
  ASSERT_EQ(0U, screen.at(0 + (3 * screen_columns)));
  ASSERT_EQ(0U, screen.at(1 + (3 * screen_columns)));
  ASSERT_EQ(0U, screen.at(0 + (4 * screen_columns)));
  ASSERT_EQ(0U, screen.at(1 + (4 * screen_columns)));
  registers.at(0) = 4;
  registers.at(1) = 1;
  handleOpcode(0xD014);
  ASSERT_EQ(0x01, screen.at(0 + (1 * screen_columns)));
  ASSERT_EQ(0x20, screen.at(1 + (1 * screen_columns)));
  ASSERT_EQ(0x03, screen.at(0 + (2 * screen_columns)));
  ASSERT_EQ(0x40, screen.at(1 + (2 * screen_columns)));
  ASSERT_EQ(0x05, screen.at(0 + (3 * screen_columns)));
  ASSERT_EQ(0x60, screen.at(1 + (3 * screen_columns)));
  ASSERT_EQ(0x07, screen.at(0 + (4 * screen_columns)));
  ASSERT_EQ(0x80, screen.at(1 + (4 * screen_columns)));
  ASSERT_EQ(0, registers.at(0xF));

  /* Remove them */
  handleOpcode(0xD014);
  ASSERT_EQ(0U, screen.at(0 + (1 * screen_columns)));
  ASSERT_EQ(0U, screen.at(1 + (1 * screen_columns)));
  ASSERT_EQ(0U, screen.at(0 + (2 * screen_columns)));
  ASSERT_EQ(0U, screen.at(1 + (2 * screen_columns)));
  ASSERT_EQ(0U, screen.at(0 + (3 * screen_columns)));
  ASSERT_EQ(0U, screen.at(1 + (3 * screen_columns)));
  ASSERT_EQ(0U, screen.at(0 + (4 * screen_columns)));
  ASSERT_EQ(0U, screen.at(1 + (4 * screen_columns)));
  ASSERT_EQ(1, registers.at(0xF));


  /* Drawing past the buffer should not wrap around (it should simple be * ignored) */
  ram.at(index_register) = 0xFF;
  registers.at(4) = (screen_columns * 8) - 1;
  registers.at(5) = screen_rows -1;
  ASSERT_EQ(0x00, screen.at((screen_columns - 1) + ((screen_rows - 1) * screen_columns)));
  ASSERT_EQ(0x00, screen.at(0));
  handleOpcode(0xD451);

  ram.at(index_register) = 0x00;
  ASSERT_EQ(0x01, screen.at((screen_columns - 1) + ((screen_rows - 1) * screen_columns)));
  ASSERT_EQ(0x00, screen.at(0));
  ASSERT_EQ(0, registers.at(0xF));

}

TEST_F(EmulatorHandleOpcode, OP_0xEX9E) {
  unsigned current_pc = 0x200;
  ASSERT_EQ(current_pc, program_counter);

  for (unsigned i= 0; i < 16; ++i) {
    keys_state.at(i) = 1;
    handleOpcode(0xE09E + (i << 8));
    current_pc += 2;
    ASSERT_EQ(current_pc, program_counter);
  }

  for (unsigned i= 0; i < 16; ++i) {
    keys_state.at(i) = 0;
    handleOpcode(0xE09E + (i << 8));
    ASSERT_EQ(current_pc, program_counter);
  }
}

TEST_F(EmulatorHandleOpcode, OP_0xEXA1) {
  unsigned current_pc = 0x200;
  ASSERT_EQ(current_pc, program_counter);

  for (unsigned i= 0; i < 16; ++i) {
    keys_state.at(i) = 0;
    handleOpcode(0xE0A1 + (i << 8));
    current_pc += 2;
    ASSERT_EQ(current_pc, program_counter);
  }

  for (unsigned i= 0; i < 16; ++i) {
    keys_state.at(i) = 1;
    handleOpcode(0xE0A1 + (i << 8));
    ASSERT_EQ(current_pc, program_counter);
  }
}

TEST_F(EmulatorHandleOpcode, OP_0xFX07) {
  delay_timer = 57U;
  for (unsigned i = 0; i < registers.size(); ++i) {
    handleOpcode(0xF007 + (i << 8));
    ASSERT_EQ(57U, registers.at(i));
  }
}

TEST_F(EmulatorHandleOpcode, OP_0xFX0A) {
  /* ASSERT_THROW(handleOpcode(0xF00A), NotImplementedError); */
}

TEST_F(EmulatorHandleOpcode, OP_0xFX15) {
  for (unsigned i = 0; i < registers.size(); ++i) {
    registers.at(i) = i;
    handleOpcode(0xF015 + (i << 8));
    ASSERT_EQ(i, delay_timer);
  }
}

TEST_F(EmulatorHandleOpcode, OP_0xFX18) {
  for (unsigned i = 0; i < registers.size(); ++i) {
    registers.at(i) = i;
    handleOpcode(0xF018 + (i << 8));
    ASSERT_EQ(i, sound_timer);
  }
}

TEST_F(EmulatorHandleOpcode, OP_0xFX1E) {
  ASSERT_EQ(0U, index_register);
  registers.at(0) = 10;
  handleOpcode(0xF01E);
  ASSERT_EQ(10U, index_register);
  ASSERT_EQ(0U, registers.at(0xF));

  registers.at(1) = 255;
  handleOpcode(0xF11E);
  ASSERT_EQ(265U, index_register);
  ASSERT_EQ(0U, registers.at(0xF));

  index_register = 0x0FFF;
  registers.at(2) = 1;
  handleOpcode(0xF21E);
  ASSERT_EQ(0U, index_register);
  ASSERT_EQ(1U, registers.at(0xF));
}

TEST_F(EmulatorHandleOpcode, OP_0xFX29) {
  for (unsigned i = 0; i < registers.size(); ++i) {
    registers.at(i) = i;
    handleOpcode(0xf029 + (i << 8));
    ASSERT_EQ(5 * i, index_register);
  }
}

TEST_F(EmulatorHandleOpcode, OP_0xFX33) {
  index_register = 100;
  registers.at(0) = 159;
  ASSERT_EQ(0U, ram.at(index_register + 0));
  ASSERT_EQ(0U, ram.at(index_register + 1));
  ASSERT_EQ(0U, ram.at(index_register + 2));

  handleOpcode(0xF033);
  ASSERT_EQ(1U, ram.at(index_register + 0));
  ASSERT_EQ(5U, ram.at(index_register + 1));
  ASSERT_EQ(9U, ram.at(index_register + 2));

  ram.at(index_register + 0) = 0;
  ram.at(index_register + 1) = 0;
  ram.at(index_register + 2) = 0;

  registers.at(0) = 59;
  ASSERT_EQ(0U, ram.at(index_register + 0));
  ASSERT_EQ(0U, ram.at(index_register + 1));
  ASSERT_EQ(0U, ram.at(index_register + 2));

  handleOpcode(0xF033);
  ASSERT_EQ(0U, ram.at(index_register + 0));
  ASSERT_EQ(5U, ram.at(index_register + 1));
  ASSERT_EQ(9U, ram.at(index_register + 2));

  ram.at(index_register + 0) = 0;
  ram.at(index_register + 1) = 0;
  ram.at(index_register + 2) = 0;

  registers.at(0) = 9;
  ASSERT_EQ(0U, ram.at(index_register + 0));
  ASSERT_EQ(0U, ram.at(index_register + 1));
  ASSERT_EQ(0U, ram.at(index_register + 2));

  handleOpcode(0xF033);
  ASSERT_EQ(0U, ram.at(index_register + 0));
  ASSERT_EQ(0U, ram.at(index_register + 1));
  ASSERT_EQ(9U, ram.at(index_register + 2));

  ram.at(index_register + 0) = 0;
  ram.at(index_register + 1) = 0;
  ram.at(index_register + 2) = 0;
}

TEST_F(EmulatorHandleOpcode, OP_0xFX55) {
  index_register = 80;

  for (unsigned i = 0; i < registers.size(); ++i) {
    registers.at(i) = i + 1;
  }

  ASSERT_EQ(0U, ram.at(index_register));

  halfword previous_index = index_register;
  handleOpcode(0xF155);
  ASSERT_EQ(1U, registers.at(0));
  ASSERT_EQ(1U, ram.at(previous_index));
  ASSERT_EQ(0U, ram.at(previous_index + 1));
  ASSERT_EQ(previous_index + 1 + 1, index_register);

  index_register = previous_index;
  handleOpcode(0xF255);
  ASSERT_EQ(1U, ram.at(previous_index));
  ASSERT_EQ(2U, ram.at(previous_index + 1));
  ASSERT_EQ(0U, ram.at(previous_index + 2));
  ASSERT_EQ(previous_index + 2 + 1, index_register);

  index_register = previous_index;

  handleOpcode(0xFF55);
  for (unsigned i = 0; i < registers.size() - 1; ++i) {
    ASSERT_EQ(i + 1, ram.at(previous_index + i));
  }

  index_register = 0;
}

TEST_F(EmulatorHandleOpcode, OP_0xFX65) {
  index_register = 0;

  for (unsigned i = 0; i < registers.size(); ++i) {
    ram.at(index_register + i) = i + 1;
  }

  ASSERT_EQ(1U, ram.at(index_register));
  ASSERT_EQ(0U, registers.at(0));

  halfword previous_index = index_register;
  handleOpcode(0xF165);
  ASSERT_EQ(1U, ram.at(previous_index));
  ASSERT_EQ(1U, registers.at(0));
  ASSERT_EQ(0U, registers.at(1));
  ASSERT_EQ(index_register, previous_index + 1 + 1);

  index_register = previous_index;
  handleOpcode(0xF265);
  ASSERT_EQ(1U, registers.at(0));
  ASSERT_EQ(2U, registers.at(1));
  ASSERT_EQ(0U, registers.at(2));
  ASSERT_EQ(index_register, previous_index + 2 + 1);

  index_register = previous_index;
  handleOpcode(0xFF65);
  for (unsigned i = 0; i < registers.size() - 1; ++i) {
    ASSERT_EQ(i + 1, registers.at(i));
  }
  ASSERT_EQ(index_register, previous_index + 0xF + 1);

  index_register = 0;
}
