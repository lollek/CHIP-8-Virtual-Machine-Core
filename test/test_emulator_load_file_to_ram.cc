
#include "gtest/gtest.h"
#include "chip8emu/Emulator.h"

class EmulatorLoadFileToRam : public ::testing::Test, public Emulator {
};


TEST_F(EmulatorLoadFileToRam, FileDoesNotExist) {
  bool status = loadFileToRam("");
  ASSERT_EQ(false, status);
  ASSERT_EQ("File empty or not found", error_msg);

  for (unsigned i : ram) {
    ASSERT_EQ(0U, i);
  }
}

TEST_F(EmulatorLoadFileToRam, FileMuchTooBig) {
  bool status = loadFileToRam("../test/4097B.txt");
  ASSERT_EQ(false, status);
  ASSERT_EQ("File too big. Only 3584 bytes available. File is 4097 bytes",
            error_msg);

  for (unsigned i : ram) {
    ASSERT_EQ(0U, i);
  }
}

TEST_F(EmulatorLoadFileToRam, FileExactlyTooBig) {
  bool status = loadFileToRam("../test/3585B.txt");
  ASSERT_EQ(false, status);
  ASSERT_EQ("File too big. Only 3584 bytes available. File is 3585 bytes",
            error_msg);

  for (unsigned i : ram) {
    ASSERT_EQ(0U, i);
  }
}

TEST_F(EmulatorLoadFileToRam, FileExactlyRight) {
  bool status = loadFileToRam("../test/3584B.txt");
  ASSERT_EQ(true, status);

  for (unsigned i = 0; i < ram_size; ++i) {
    if (i < 0x200) {
      ASSERT_EQ(0U, ram.at(i));
    } else {
      ASSERT_EQ('c', ram.at(i));
    }
  }
}
