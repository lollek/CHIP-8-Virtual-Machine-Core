
#include "gtest/gtest.h"
#include "chip8core/Emulator.h"

class EmulatorLoadFileToRam : public ::testing::Test, public Emulator {
};


TEST_F(EmulatorLoadFileToRam, FileDoesNotExist) {
  bool status = loadFileToRam("");
  ASSERT_EQ(false, status);
  ASSERT_EQ("File empty or not found", error_msg);

  for (unsigned i = 80; i < ram.size(); ++i) {
    ASSERT_EQ(0U, ram.at(i));
  }
}

TEST_F(EmulatorLoadFileToRam, FileMuchTooBig) {
  bool status = loadFileToRam("../test/4097B.txt");
  ASSERT_EQ(false, status);
  ASSERT_EQ("File too big. Only 3584 bytes available. File is 4097 bytes",
            error_msg);

  for (unsigned i = 80; i < ram.size(); ++i) {
    ASSERT_EQ(0U, ram.at(i));
  }
}

TEST_F(EmulatorLoadFileToRam, FileExactlyTooBig) {
  bool status = loadFileToRam("../test/3585B.txt");
  ASSERT_EQ(false, status);
  ASSERT_EQ("File too big. Only 3584 bytes available. File is 3585 bytes",
            error_msg);

  for (unsigned i = 80; i < ram.size(); ++i) {
    ASSERT_EQ(0U, ram.at(i));
  }
}

TEST_F(EmulatorLoadFileToRam, FileExactlyRight) {
  bool status = loadFileToRam("../test/3584B.txt");
  ASSERT_EQ(true, status);

  for (unsigned i = 80; i < ram_size; ++i) {
    if (i < 0x200) {
      ASSERT_EQ(0U, ram.at(i));
    } else {
      ASSERT_EQ('c', ram.at(i));
    }
  }
}

TEST_F(EmulatorLoadFileToRam, LoadDataCorrectly) {
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
}
