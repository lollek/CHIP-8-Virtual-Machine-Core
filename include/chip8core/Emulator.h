#ifndef EMULATOR_H
#define EMULATOR_H

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <functional>

using byte       = uint8_t;
using halfword   = uint16_t;
using screen_row = uint8_t;

class Emulator {
public:
  Emulator();
  ~Emulator() = default;

  std::string const& getError() const;
  byte const* getGraphicsData() const;
  void setKeyState(int key_number, bool on);

  std::function<void()> onSound;
  std::function<void()> onGraphics;

  void tick();

  bool loadFileToRam(std::string const& file);

  unsigned static constexpr ram_size = 4096;
  unsigned static constexpr num_registers = 16;
  unsigned static constexpr screen_columns = 64 / 8;
  unsigned static constexpr screen_rows = 32;
  unsigned static constexpr screen_bytes = screen_rows * screen_columns;
  unsigned static constexpr stack_size = 16;
  unsigned static constexpr num_keys = 16;
  halfword static constexpr program_counter_start = 0x200;

  class FatalError : public std::runtime_error {
  public:
    FatalError(std::string error_message);
  };

  class NotImplementedError : public std::runtime_error {
  public:
    NotImplementedError(std::string error_message);
  };


protected:
  halfword fetchOpcode() ;
  void handleOpcode(halfword opcode);
  void resetState();
  void addFontDataToRam();

  std::vector<byte>       ram;
  std::vector<screen_row> screen;
  std::vector<byte>       registers;
  halfword                index_register;
  halfword                program_counter;
  byte                    sound_timer;
  byte                    delay_timer;
  std::vector<halfword>   stack;
  byte                    stack_pointer;
  std::vector<byte>       keys_state;

  std::string             error_msg;
  bool tick_lock;
};

#endif /* EMULATOR_H */
