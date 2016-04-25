#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "chip8core/Emulator.h"

unsigned constexpr Emulator::ram_size;
unsigned constexpr Emulator::num_registers;
unsigned constexpr Emulator::screen_columns;
unsigned constexpr Emulator::screen_rows;
unsigned constexpr Emulator::screen_bytes;
unsigned constexpr Emulator::stack_size;
unsigned constexpr Emulator::num_keys;
halfword constexpr Emulator::program_counter_start;



Emulator::Emulator() :
  onSound(nullptr),
  onGraphics(nullptr),
  ram(std::vector<byte>(ram_size, 0)),
  screen(std::vector<screen_row>(screen_bytes, 0)),
  registers(std::vector<byte>(num_registers, 0)),
  index_register(0),
  program_counter(program_counter_start),
  sound_timer(0),
  delay_timer(0),
  stack(std::vector<halfword>(stack_size, 0)),
  stack_pointer(0),
  keys_state(std::vector<byte>(num_keys, 0)),
  error_msg(),
  tick_lock(false),
  awaiting_keypress(false),
  awaiting_keypress_register(0)
  {
    srand(time(NULL));
    addFontDataToRam();
}

void Emulator::resetState() {
  *this = Emulator();
}

void Emulator::addFontDataToRam() {
  // Load fonts to start of memory
  std::vector<byte> font {
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
  };
  memcpy(ram.data(), font.data(), font.size());
}

std::string const& Emulator::getError() const {
  return error_msg;
}

byte const* Emulator::getGraphicsData() const {
  return screen.data();
}

void Emulator::setKeyState(int key_number, bool on) {
  keys_state.at(key_number) = on ? 0xFF : 0x00;

  if (awaiting_keypress) {
    registers.at(awaiting_keypress_register) = key_number;
    awaiting_keypress = false;
  }
}

bool Emulator::loadFileToRam(std::string const& filename) {
  std::ifstream file(filename, std::ios::binary|std::ios::ate);
  ssize_t filesize = file.tellg();
  ssize_t available_ram = ram_size - program_counter;

  if (filesize <= 0) {
    error_msg = "File empty or not found";
    return false;

  } else if (filesize > available_ram) {
    error_msg = "File too big. Only " + std::to_string(available_ram)
              + " bytes available. File is " + std::to_string(filesize) + " bytes";
    return false;
  }

  tick_lock = true;
  file.seekg(0, std::ios::beg);
  resetState();
  file.read(reinterpret_cast<char *>(&ram.data()[program_counter]), filesize);

  tick_lock = false;
  return true;
}

halfword Emulator::fetchOpcode() {
  if (program_counter >= ram_size - 1) {
    error_msg = "Program counter out of bounds";
    return 0xFFFFU;
  }

  halfword opcode = (ram.at(program_counter) << 8) + ram.at(program_counter +1);
  increment_pc();
  return opcode;
}

bool Emulator::handleOpcode0(halfword opcode) {
  switch (opcode) {

    // 0x00E0 - Clears the screen
    case 0x00E0:
      std::fill(screen.begin(), screen.end(), 0);
      if (onGraphics != nullptr) { onGraphics(); }
      return true;

    // 0x00EE - Returns from subroutine
    case 0x00EE:
      if (stack_pointer == 0) {
        error_msg = "Stack underflow";
        return false;
      }
      program_counter = stack.at(--stack_pointer);
      return true;

    // 0x0NNN - Calls RCA 1802 program at address NNN.
    default: {
      std::stringstream ss;
      ss << "Opcode " << std::hex << opcode << " not implemented";
      error_msg = ss.str();
      return false;
    }
  }
}

inline bool Emulator::handleOpcode1(halfword opcode) {
  // 0x1NNN - Jump to opcode & 0x0FFF
  program_counter = op_nnn_value(opcode);
  return true;
}

inline bool Emulator::handleOpcode2(halfword opcode) {
  // 0x2NNN - Call subroutine at opcode & 0x0FFF
  if (stack_pointer >= stack_size) {
    error_msg = "Stack overflow";
    return false;
  }
  stack.at(stack_pointer++) = program_counter;
  program_counter = op_nnn_value(opcode);
  return true;
}

inline bool Emulator::handleOpcode3(halfword opcode) {
  // 0x3XNN - Skips the next instruction if VX equals NN.
  if (vx_register(opcode) == op_nn_value(opcode)) { increment_pc(); }
  return true;
}

inline bool Emulator::handleOpcode4(halfword opcode) {
  // 0x4XNN - Skips the next instruction if VX doesn't equal NN.
  if (vx_register(opcode) != op_nn_value(opcode)) { increment_pc(); }
  return true;
}

inline bool Emulator::handleOpcode5(halfword opcode) {
  // 0x5XY0 - Skips the next instruction if VX equals VY
  // NOTE: At the moment, ignore the 0x000F value, but it's possible that this
  // should raise an error
  if (vx_register(opcode) == vy_register(opcode)) { increment_pc(); }
  return true;
}

inline bool Emulator::handleOpcode6(halfword opcode) {
  // 0x6XNN - Set VX to NN
  vx_register(opcode) = op_nn_value(opcode);
  return true;
}

inline bool Emulator::handleOpcode7(halfword opcode) {
  // 0x7XNN - Add NN to VX
  vx_register(opcode) += op_nn_value(opcode);
  return true;
}

bool Emulator::handleOpcode8(halfword opcode) {
  switch (op_z_value(opcode)) {

    // 0x8XY0 - Set VX to VY
    case 0x0000: vx_register(opcode) = vy_register(opcode); return true;

    // 0x8XY1 - Set VX to VX OR VY
    case 0x0001: vx_register(opcode) |= vy_register(opcode); return true;

    // 0x8XY2 - Set VX to VX AND VY
    case 0x0002: vx_register(opcode) &= vy_register(opcode); return true;

    // 0x8XY3 - Set VX to VX XOR VY
    case 0x0003: vx_register(opcode) ^= vy_register(opcode); return true;

    // 0x8XY4 - Add VY to VX and set VF if there is a carry
    case 0x0004: {
      byte old_value = vx_register(opcode);
      vx_register(opcode) += vy_register(opcode);
      vf_register() = old_value > vx_register(opcode);
      return true;
    }

    // 0x8XY5 - Subtract VY from VX and set VF if there was no borrow
    case 0x0005: {
      byte old_value = vx_register(opcode);
      vx_register(opcode) -= vy_register(opcode);
      vf_register() = old_value >= vx_register(opcode);
      return true;
    }

    // 0x8XY6 - Shift VY to the right and copy it to VX
    // VF is set to the previous least significant bit
    case 0x0006:
      vf_register() = vy_register(opcode) & 1;
      vx_register(opcode) = vy_register(opcode) >>= 1;
      return true;

    // 0x8XY7 - Sets VX to VY minus VX. VF is set to 0 when there's a borrow, else 1
    case 0x0007:
      vx_register(opcode) = vy_register(opcode) - vx_register(opcode);
      vf_register() = vy_register(opcode) >= vx_register(opcode);
      return true;

    // 0x8XYE - Shifts VY left by one and copy it to VX.
    // VF is set to the most significant bit before the shift.
    case 0x000E:
      vf_register() = vy_register(opcode) & 0x80 ? 1 : 0;
      vx_register(opcode) = vy_register(opcode) <<= 1;
      return true;

    default: {
      std::stringstream ss;
      ss << "Opcode " << std::hex << opcode << " not implemented";
      error_msg = ss.str();
      return false;
    }
  }
}

inline bool Emulator::handleOpcode9(halfword opcode) {
  // 0x9XY0 - Skips the next instruction if VX doesn't equal VY.
  // NOTE: At the moment, ignore the 0x000F value, but it's possible that this
  // should raise an error
  if (vx_register(opcode) != vy_register(opcode)) { increment_pc(); }
  return true;
}

inline bool Emulator::handleOpcodeA(halfword opcode) {
  // 0xANNN - Sets I to the address NNN.
  index_register = op_nnn_value(opcode);
  return true;
}

inline bool Emulator::handleOpcodeB(halfword opcode) {
  // 0xBNNN - Jumps to the address NNN plus V[0].
  program_counter = (op_nnn_value(opcode) + registers.at(0)) % 0x1000;
  return true;
}

bool Emulator::handleOpcodeC(halfword opcode) {
  // 0xCXNN - Sets VX to a bitwise and operation on a random number and NN.
  vx_register(opcode) = (rand() % 0xFF) & op_nn_value(opcode);
  return true;
}

bool Emulator::handleOpcodeD(halfword opcode) {
  // 0xDXYN
  // Sprites stored in memory at location in index register (I), 8bits wide.
  // Wraps around the screen. If when drawn, clears a pixel, register VF is
  // set to 1 otherwise it is zero. All drawing is XOR drawing (i.e. it
  // toggles the screen pixels). Sprites are drawn starting at position VX,
  // VY. N is the number of 8bit rows that need to be drawn. If N is greater
  // than 1, second line continues at position VX, VY+1, and so on.

  byte const sprite_x       = vx_register(opcode);
  byte const sprite_y       = vy_register(opcode);
  byte const sprite_x_bytes = sprite_x / 8;
  byte const sprite_x_bits  = sprite_x % 8;

  vf_register() = 0;

  // Since drawings are most often not byte-aligned:
  // * - Take out both possibly affected bytes
  // * - Align the drawing to the two bytes
  // * - XOR the data
  // * - Put both bytes back
  // In some cases, this would make us draw past the screen, so we skip those
  byte const num_rows = op_z_value(opcode);
  for (byte y = 0; y < num_rows; ++y) {
    halfword const graphics_data = ram.at(index_register + y) << (8 - sprite_x_bits);
    byte const screen_pos = (sprite_x_bytes + ((sprite_y + y) * screen_columns));
    bool const has_right_byte = screen_pos + 1 < screen_bytes;

    byte scratch_byte = 0;
    byte& screen_byte_left = screen.at(screen_pos % screen_bytes);
    byte& screen_byte_right = has_right_byte
      ? (screen.at((screen_pos + 1) % screen_bytes))
      : scratch_byte;

    halfword screen_data = (screen_byte_left << 8) + screen_byte_right;
    if (screen_data & graphics_data) { vf_register() = 1; }
    screen_data ^= graphics_data;

    screen_byte_left = ((screen_data & 0xFF00) >> 8);
    screen_byte_right = (screen_data & 0x00FF);
  }

  if (onGraphics != nullptr) {
    onGraphics();
  }

  return true;
}

bool Emulator::handleOpcodeE(halfword opcode) {
  switch (op_nn_value(opcode)) {

    // 0xEX9E - Skips the next instruction if the key stored in VX is pressed.
    case 0x009E:
      if (keys_state.at(vx_register(opcode)) != 0) { increment_pc(); }
      return true;

    // 0xEXA1 - Skips the next instruction if the key stored in VX isn't pressed.
    case 0x00A1:
      if (keys_state.at(vx_register(opcode)) == 0) { increment_pc(); }
      return true;

    default: {
      std::stringstream ss;
      ss << "Opcode " << std::hex << opcode << " not implemented";
      error_msg = ss.str();
      return false;
    }
  }
}

bool Emulator::handleOpcodeF(halfword opcode) {
  switch (op_nn_value(opcode)) {

    // 0xFX07 - Sets VX to the value of the delay timer.
    case 0x0007: vx_register(opcode) = delay_timer; return true;

    // 0xFX0A - A key press is awaited, and then stored in VX.
    case 0x000A:
      awaiting_keypress = true;
      awaiting_keypress_register = op_x_value(opcode);
      return true;

    // 0xFX15 - Sets the delay timer to VX.
    case 0x0015: delay_timer = vx_register(opcode); return true;

    // 0xFX18 - Sets the sound timer to VX.
    case 0x0018: sound_timer = vx_register(opcode); return true;

    // 0xFX1E - Adds VX to I. Also secretly sets VF to 1 on overflow else 0
    case 0x001E: {
      byte old_index = index_register;
      index_register = (index_register + vx_register(opcode)) % 0x1000;
      vf_register() = old_index > index_register;
      return true;
    }

    // 0xFX29 - Sets I to the location of the sprite for the character in VX.
    // Characters 0-F (in hexadecimal) are represented by a 4x5 font.
    // ( I have stored these fonts in the RAM, byte 0 and forward )
    case 0x0029: index_register = vx_register(opcode) * 5; return true;

    // 0xFX33
    // Stores the Binary-coded decimal representation of VX, with the most
    // significant of three digits at the address in I, the middle digit at I
    // plus 1, and the least significant digit at I plus 2. (In other words,
    // take the decimal representation of VX, place the hundreds digit in memory
    // at location in I, the tens digit at location I+1, and the ones digit at
    // location I+2.)
    case 0x0033: {
      byte value = vx_register(opcode);
      ram.at(index_register + 0) = value / 100;
      ram.at(index_register + 1) = value / 10 % 10;
      ram.at(index_register + 2) = value % 10;
      return true;
    }

    // 0xFX55 - Stores V0 to VX in memory starting at address I.
    // Also sets I to I + X + 1
    case 0x0055: {
      halfword end = op_x_value(opcode);
      for (halfword i = 0; i <= end; ++i) {
        ram.at(index_register++) = registers.at(i);
      }
      return true;
    }

    // 0xFX65 - Fills V0 to VX with values from memory starting at address I
    // Also sets I to I + X + 1
    case 0x0065: {
      halfword end = op_x_value(opcode);
      for (halfword i = 0; i <= end; ++i) {
        registers.at(i) = ram.at(index_register++);
      }
      return true;
    }

    default: {
      std::stringstream ss;
      ss << "Opcode " << std::hex << opcode << " not implemented";
      error_msg = ss.str();
      return false;
    }
  }
}

bool Emulator::handleOpcode(halfword opcode) {
  switch (opcode & 0xF000) {
    case 0x0000: return handleOpcode0(opcode);
    case 0x1000: return handleOpcode1(opcode);
    case 0x2000: return handleOpcode2(opcode);
    case 0x3000: return handleOpcode3(opcode);
    case 0x4000: return handleOpcode4(opcode);
    case 0x5000: return handleOpcode5(opcode);
    case 0x6000: return handleOpcode6(opcode);
    case 0x7000: return handleOpcode7(opcode);
    case 0x8000: return handleOpcode8(opcode);
    case 0x9000: return handleOpcode9(opcode);
    case 0xA000: return handleOpcodeA(opcode);
    case 0xB000: return handleOpcodeB(opcode);
    case 0xC000: return handleOpcodeC(opcode);
    case 0xD000: return handleOpcodeD(opcode);
    case 0xE000: return handleOpcodeE(opcode);
    case 0xF000: return handleOpcodeF(opcode);
    default:     return false;
  }
}

inline halfword Emulator::op_w_value(halfword opcode) {
  return (opcode & 0xF000) >> 12;
}
inline halfword Emulator::op_x_value(halfword opcode) {
  return (opcode & 0x0F00) >> 8;
}
inline halfword Emulator::op_y_value(halfword opcode) {
  return (opcode & 0x00F0) >> 4;
}
inline halfword Emulator::op_z_value(halfword opcode) {
  return opcode & 0x000F;
}
inline halfword Emulator::op_nnn_value(halfword opcode) {
  return opcode & 0x0FFF;
}
inline halfword Emulator::op_nn_value(halfword opcode) {
  return opcode & 0x00FF;
}

inline byte& Emulator::vx_register(halfword opcode) {
  return registers.at(op_x_value(opcode));
}
inline byte& Emulator::vy_register(halfword opcode) {
  return registers.at(op_y_value(opcode));
}
inline byte& Emulator::vf_register() {
  return registers.at(0xF);
}

inline void Emulator::increment_pc() {
  program_counter = (program_counter + 2) % ram_size;
}


bool Emulator::tick() {
  if (awaiting_keypress || tick_lock) {
    return true;
  }
  tick_lock = true;

  halfword opcode = fetchOpcode();
  bool return_value = handleOpcode(opcode);

  if (delay_timer > 0) {
    --delay_timer;
  }

  if (sound_timer > 0) {
    if (--sound_timer == 0 && onSound != nullptr) {
      onSound();
    }
  }

  tick_lock = false;
  return return_value;
}
