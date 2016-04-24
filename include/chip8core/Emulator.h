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
  explicit Emulator();
  explicit Emulator(Emulator const&) = default;
  ~Emulator() = default;

  Emulator& operator=(Emulator const&) = default;
  Emulator& operator=(Emulator&&) = default;


  /**
   * Returns most recent error message
   */
  std::string const& getError() const;

  /**
   * Get a pointer to the graphics data
   * TODO: Add more info
   */
  byte const* getGraphicsData() const;

  /**
   * Set the key to either pressed or unpressed
   * key_number must be between 0 and Emulator::num_keys
   * on should be true if pressed down, else false.
   */
  void setKeyState(int key_number, bool on);

  /**
   * If a function is set here, it will execute when the CPU wants sound
   */
  std::function<void()> onSound;

  /**
   * If a function is set here, it will be called when graphics have changed.
   * This is so you don't have to redraw every clock cycle.
   */
  std::function<void()> onGraphics;

  /**
   * Tell the emulated CPU to process one clock cycle
   */
  void tick();

  /**
   * Loads file with filename into RAM.
   * Returns true on success.
   * Returns false on error, and sets error message (see getError())
   */
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
  halfword fetchOpcode();
  void handleOpcode(halfword opcode);
  void handleOpcode0(halfword opcode);
  void handleOpcode1(halfword opcode);
  void handleOpcode2(halfword opcode);
  void handleOpcode3(halfword opcode);
  void handleOpcode4(halfword opcode);
  void handleOpcode5(halfword opcode);
  void handleOpcode6(halfword opcode);
  void handleOpcode7(halfword opcode);
  void handleOpcode8(halfword opcode);
  void handleOpcode9(halfword opcode);
  void handleOpcodeA(halfword opcode);
  void handleOpcodeB(halfword opcode);
  void handleOpcodeC(halfword opcode);
  void handleOpcodeD(halfword opcode);
  void handleOpcodeE(halfword opcode);
  void handleOpcodeF(halfword opcode);
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
  bool                    tick_lock;
  bool                    awaiting_keypress;
  unsigned                awaiting_keypress_register;
};

#endif /* EMULATOR_H */
