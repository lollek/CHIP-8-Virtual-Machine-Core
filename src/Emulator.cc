#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "chip8core/Emulator.h"

Emulator::FatalError::FatalError(std::string error_message) :
  std::runtime_error(error_message) {
}

Emulator::NotImplementedError::NotImplementedError(std::string error_message) :
  std::runtime_error(error_message) {
}

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
    throw FatalError("Program counter out of bounds("
                   + std::to_string(program_counter) + ")");
  }

  halfword opcode = (ram.at(program_counter) << 8) + ram.at(program_counter +1);
  program_counter = (program_counter + 2) % ram_size;
  return opcode;
}

void Emulator::handleOpcode0(halfword opcode) {
  // 0x00E0 - Clears the screen
  if (opcode == 0x00E0) {
    std::fill(screen.begin(), screen.end(), 0);
    if (onGraphics != nullptr) {
      onGraphics();
    }

  // 0x00EE - Returns from subroutine
  } else if (opcode == 0x00EE) {
    if (stack_pointer == 0) {
      throw FatalError("Stack underflow");
    }
    program_counter = stack.at(--stack_pointer);

  // 0x0NNN - Calls RCA 1802 program at address NNN.
  } else if ((opcode & 0xF000) == 0x0000) {
    throw NotImplementedError("Opcode " + std::to_string(opcode) + " not implemented");
  }
}

void Emulator::handleOpcode1(halfword opcode) {
  // 0x1NNN - Jump to opcode & 0x0FFF
  program_counter = opcode & 0x0FFF;
}

void Emulator::handleOpcode2(halfword opcode) {
  // 0x2NNN - Call subroutine at opcode & 0x0FFF
  if (stack_pointer >= stack_size) {
    throw FatalError("Stack overflow");
  }
  stack.at(stack_pointer++) = program_counter;
  program_counter = opcode & 0x0FFF;
}

void Emulator::handleOpcode3(halfword opcode) {
  // 0x3XNN - Skips the next instruction if VX equals NN.
  unsigned const x_register = (opcode & 0x0F00) >> 8;
  if (registers.at(x_register) == (opcode & 0x00FF)) {
    program_counter = (program_counter + 2) % ram_size;
  }
}

void Emulator::handleOpcode4(halfword opcode) {
  // 0x4XNN - Skips the next instruction if VX doesn't equal NN.
  unsigned const x_register = (opcode & 0x0F00) >> 8;
  if (registers.at(x_register) != (opcode & 0x00FF)) {
    program_counter = (program_counter + 2) % ram_size;
  }
}

void Emulator::handleOpcode5(halfword opcode) {
  // 0x5XY0 - Skips the next instruction if VX equals VY
  // NOTE: At the moment, ignore the 0x000F value, but it's possible that this
  // should raise an error
  unsigned const x_register = (opcode & 0x0F00) >> 8;
  unsigned const y_register = (opcode & 0x00F0) >> 4;
  if (registers.at(x_register) == registers.at(y_register)) {
    program_counter = (program_counter + 2) % ram_size;
  }
}

void Emulator::handleOpcode6(halfword opcode) {
  // 0x6XNN - Set VX to NN
  unsigned const x_register = (opcode & 0x0F00) >> 8;
  registers.at(x_register) = opcode & 0x00FF;
}

void Emulator::handleOpcode7(halfword opcode) {
  // 0x7XNN - Add NN to VX
  unsigned const x_register = (opcode & 0x0F00) >> 8;
  registers.at(x_register) += opcode & 0x00FF;
}

void Emulator::handleOpcode8(halfword opcode) {
  unsigned const x_register = (opcode & 0x0F00) >> 8;
  unsigned const y_register = (opcode & 0x00F0) >> 4;
  switch (opcode & 0x000F) {

    // 0x8XY0 - Set VX to VY
    case 0x0000: registers.at(x_register) = registers.at(y_register); break;

    // 0x8XY1 - Set VX to VX OR VY
    case 0x0001: registers.at(x_register) |= registers.at(y_register); break;

    // 0x8XY2 - Set VX to VX AND VY
    case 0x0002: registers.at(x_register) &= registers.at(y_register); break;

    // 0x8XY3 - Set VX to VX XOR VY
    case 0x0003: registers.at(x_register) ^= registers.at(y_register); break;

    // 0x8XY4 - Add VY to VX and set VF if there is a carry
    case 0x0004: {
      byte x_register_value = registers.at(x_register);
      registers.at(x_register) += registers.at(y_register);
      registers.at(0xF) = x_register_value > registers.at(x_register);
    } break;

    // 0x8XY5 - Subtract VY from VX and set VF if there was no borrow
    case 0x0005: {
      byte x_register_value = registers.at(x_register);
      registers.at(x_register) -= registers.at(y_register);
      registers.at(0xF) = x_register_value >= registers.at(x_register);
    } break;

    // 0x8XY6 - Shift to the right and set VF to previous least significant bit
    case 0x0006:
      registers.at(0xF) = registers.at(x_register) & 1;
      registers.at(x_register) >>= 1;
      break;

    // 0x8XY7 - Sets VX to VY minus VX. VF is set to 0 when there's a borrow, else 1
    case 0x0007:
      registers.at(x_register) = registers.at(y_register) - registers.at(x_register);
      registers.at(0xF) = registers.at(y_register) >= registers.at(x_register);
      break;

    // 0x8XYE - Shifts VX left by one.
    // VF is set to the most significant bit before the shift.
    case 0x000E:
      registers.at(0xF) = registers.at(x_register) & 0x80 ? 1 : 0;
      registers.at(x_register) <<= 1;
      break;

    default:
      throw NotImplementedError("Opcode " + std::to_string(opcode) + " not implemented");
  }
}

void Emulator::handleOpcode9(halfword opcode) {
  // 0x9XY0 - Skips the next instruction if VX doesn't equal VY.
  // NOTE: At the moment, ignore the 0x000F value, but it's possible that this
  // should raise an error
  unsigned const x_register = (opcode & 0x0F00) >> 8;
  unsigned const y_register = (opcode & 0x00F0) >> 4;
  if (registers.at(x_register) != registers.at(y_register)) {
    program_counter = (program_counter + 2) % ram_size;
  }
}

void Emulator::handleOpcodeA(halfword opcode) {
  // 0xANNN - Sets I to the address NNN.
  index_register = opcode & 0x0FFF;
}

void Emulator::handleOpcodeB(halfword opcode) {
  // 0xBNNN - Jumps to the address NNN plus V[0].
  program_counter = ((opcode & 0x0FFF) + registers.at(0)) % 0x1000;
}

void Emulator::handleOpcodeC(halfword opcode) {
  // 0xCXNN - Sets VX to a bitwise and operation on a random number and NN.
  unsigned const x_register = (opcode & 0x0F00) >> 8;
  registers.at(x_register) = (rand() % 0xFF) & opcode & 0x00FF;
}

void Emulator::handleOpcodeD(halfword opcode) {
  // 0xDXYN
  // Sprites stored in memory at location in index register (I), 8bits wide.
  // Wraps around the screen. If when drawn, clears a pixel, register VF is
  // set to 1 otherwise it is zero. All drawing is XOR drawing (i.e. it
  // toggles the screen pixels). Sprites are drawn starting at position VX,
  // VY. N is the number of 8bit rows that need to be drawn. If N is greater
  // than 1, second line continues at position VX, VY+1, and so on.

  unsigned const x_register = (opcode & 0x0F00) >> 8;
  unsigned const y_register = (opcode & 0x00F0) >> 4;
  unsigned sprite_x = registers.at(x_register);
  unsigned sprite_y = registers.at(y_register);
  unsigned num_rows = opcode & 0x000F;
  registers.at(0xF) = 0;

  unsigned sprite_x_bytes = sprite_x / 8;
  unsigned sprite_x_bits = sprite_x % 8;

  // Since drawings are most often not byte-aligned:
  // * - Take out both possibly affected bytes
  // * - Align the drawing to the two bytes
  // * - XOR the data
  // * - Put both bytes back
  // In some cases, this would make us draw past the screen, so we skip those
  for (unsigned y = 0; y < num_rows; ++y) {
    unsigned screen_pos = (sprite_x_bytes + ((sprite_y + y) * screen_columns));
    byte scratch_byte = 0;
    byte& screen_byte_left = screen.at(screen_pos % screen_bytes);
    byte& screen_byte_right = (screen_pos + 1 < screen_bytes)
      ? (screen.at((screen_pos + 1) % screen_bytes))
      : scratch_byte;

    halfword screen_data = (screen_byte_left << 8) + screen_byte_right;
    halfword graphics_data = ram.at(index_register + y) << (8 - sprite_x_bits);
    if (screen_data & graphics_data) {
      registers.at(0xF) = 1;
    }
    screen_data ^= graphics_data;

    screen_byte_left = ((screen_data & 0xFF00) >> 8);
    screen_byte_right = (screen_data & 0x00FF);
  }

  if (onGraphics != nullptr) {
    onGraphics();
  }
}

void Emulator::handleOpcodeE(halfword opcode) {
  unsigned const x_register = (opcode & 0x0F00) >> 8;

  // 0xEX9E - Skips the next instruction if the key stored in VX is pressed.
  if ((opcode & 0xF0FF) == 0xE09E) {
    if (keys_state.at(registers.at(x_register)) != 0) {
      program_counter = (program_counter + 2) % ram_size;
    }

  } else if ((opcode & 0xF0FF) == 0xE0A1) {
    // 0xEXA1 - Skips the next instruction if the key stored in VX isn't pressed.
    if (keys_state.at(registers.at(x_register)) == 0) {
      program_counter = (program_counter + 2) % ram_size;
    }

  } else {
    throw NotImplementedError("Opcode " + std::to_string(opcode) + " not implemented");
  }
}

void Emulator::handleOpcodeF(halfword opcode) {
  unsigned const x_register = (opcode & 0x0F00) >> 8;

  // 0xFX07 - Sets VX to the value of the delay timer.
  if ((opcode & 0xF0FF) == 0xF007) {
   registers.at(x_register) = delay_timer;

   // 0xFX0A - A key press is awaited, and then stored in VX.
  } else if ((opcode & 0xF0FF) == 0xF00A) {
    awaiting_keypress = true;
    awaiting_keypress_register = x_register;

   // 0xFX15 -* Sets the delay timer to VX.
  } else if ((opcode & 0xF0FF) == 0xF015) {
    delay_timer = registers.at(x_register);

    // 0xFX18 - Sets the sound timer to VX.
  } else if ((opcode & 0xF0FF) == 0xF018) {
    sound_timer = registers.at(x_register);

    // 0xFX1E - Adds VX to I. Also secretly sets VF to 1 on overflow else 0
  } else if ((opcode & 0xF0FF) == 0xF01E) {
    byte old_index = index_register;
    index_register = (index_register + registers.at(x_register)) % 0x1000;
    registers.at(0xF) = old_index > index_register;

    // 0xFX29 - Sets I to the location of the sprite for the character in VX.
    // Characters 0-F (in hexadecimal) are represented by a 4x5 font.
    // ( I have stored these fonts in the RAM, byte 0 and forward )
  } else if ((opcode & 0xF0FF) == 0xF029) {
    index_register = registers[x_register] * 5;

    // 0xFX33
    // Stores the Binary-coded decimal representation of VX, with the most
    // significant of three digits at the address in I, the middle digit at I
    // plus 1, and the least significant digit at I plus 2. (In other words,
    // take the decimal representation of VX, place the hundreds digit in memory
    // at location in I, the tens digit at location I+1, and the ones digit at
    // location I+2.)
  } else if ((opcode & 0xF0FF) == 0xF033) {
    byte value = registers.at(x_register);
    ram.at(index_register + 0) = value / 100;
    ram.at(index_register + 1) = value / 10 % 10;
    ram.at(index_register + 2) = value % 10;

    // 0xFX55 - Stores V0 to VX in memory starting at address I
  } else if ((opcode & 0xF0FF) == 0xF055) {
    for (unsigned i = 0; i < x_register; ++i) {
      ram.at(index_register + i) = registers.at(i);
    }

    // 0xFX65 - Fills V0 to VX with values from memory starting at address I
  } else if ((opcode & 0xF0FF) == 0xF065) {
    for (unsigned i = 0; i < x_register; ++i) {
      registers.at(i) = ram.at(index_register + i);
    }

  } else {
    throw NotImplementedError("Opcode " + std::to_string(opcode) + " not implemented");
  }
}

void Emulator::handleOpcode(halfword opcode) {
  /* https://en.wikipedia.org/wiki/CHIP-8#Virtual_machine_description */

  switch (opcode & 0xF000) {
    case 0x0000: handleOpcode0(opcode); break;
    case 0x1000: handleOpcode1(opcode); break;
    case 0x2000: handleOpcode2(opcode); break;
    case 0x3000: handleOpcode3(opcode); break;
    case 0x4000: handleOpcode4(opcode); break;
    case 0x5000: handleOpcode5(opcode); break;
    case 0x6000: handleOpcode6(opcode); break;
    case 0x7000: handleOpcode7(opcode); break;
    case 0x8000: handleOpcode8(opcode); break;
    case 0x9000: handleOpcode9(opcode); break;
    case 0xA000: handleOpcodeA(opcode); break;
    case 0xB000: handleOpcodeB(opcode); break;
    case 0xC000: handleOpcodeC(opcode); break;
    case 0xD000: handleOpcodeD(opcode); break;
    case 0xE000: handleOpcodeE(opcode); break;
    case 0xF000: handleOpcodeF(opcode); break;
  }
}

void Emulator::tick() {
  if (awaiting_keypress || tick_lock) {
    return;
  }
  tick_lock = true;

  halfword opcode = fetchOpcode();
  handleOpcode(opcode);

  if (delay_timer > 0) {
    --delay_timer;
  }

  if (sound_timer > 0) {
    if (--sound_timer == 0 && onSound != nullptr) {
      onSound();
    }
  }

  tick_lock = false;
}
