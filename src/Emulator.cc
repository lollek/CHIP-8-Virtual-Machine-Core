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
  tick_lock(false)
  {
    srand(time(NULL));

    /* Load fonts to start of memory */
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

  file.seekg(0, std::ios::beg);
  file.read(reinterpret_cast<char *>(&ram.data()[program_counter]), filesize);

  return true;
}

halfword Emulator::fetchOpcode() {
  if (program_counter >= ram_size) {
    throw FatalError("Program counter out of bounds("
                   + std::to_string(program_counter) + ")");
  }

  if (program_counter % 2) {
    throw FatalError("Program counter is misaligned ("
                   + std::to_string(program_counter) + ")");
  }

  halfword opcode = (ram.at(program_counter) << 8) + ram.at(program_counter +1);
  program_counter = (program_counter + 2) % ram_size;
  return opcode;
}

void Emulator::handleOpcode(halfword opcode) {
  /* https://en.wikipedia.org/wiki/CHIP-8#Virtual_machine_description */

  if (opcode == 0x00E0) { /* 0x00E0 */
    /* Clears the screen */
    for (unsigned i = 0; i < screen.size(); ++i) {
      screen.at(i) = 0;
    }
    if (onGraphics != nullptr) {
      onGraphics();
    }

  } else if (opcode == 0x00EE) { /* 0x00EE */
    /* Returns from subroutine */
    if (stack_pointer == 0) {
      throw FatalError("Stack underflow");
    }
    program_counter = stack.at(--stack_pointer);

  } else if ((opcode & 0xF000) == 0x0000) { /* 0x0NNN */
    /* Calls RCA 1802 program at address NNN. */
    goto not_implemented;

  } else if ((opcode & 0xF000) == 0x1000) { /* 0x1NNN */
    /* Jump to opcode & 0x0FFF */
    program_counter = opcode & 0x0FFF;

  } else if ((opcode & 0xF000) == 0x2000) { /* 0x2NNN */
    /* Call subroutine at opcode & 0x0FFF */
    if (stack_pointer >= stack_size) {
      throw FatalError("Stack overflow");
    }
    stack.at(stack_pointer++) = program_counter;
    program_counter = opcode & 0x0FFF;

  } else if ((opcode & 0xF000) == 0x3000) { /* 0x3XNN */
    /* Skips the next instruction if VX equals NN. */
    unsigned v_register = (opcode & 0x0F00) >> 8;
    byte expected_value = opcode & 0x00FF;

    if (registers.at(v_register) == expected_value) {
      fetchOpcode();
    }

  } else if ((opcode & 0xF000) == 0x4000) { /* 0x4XNN */
    /* Skips the next instruction if VX doesn't equal NN. */
    unsigned v_register = (opcode & 0x0F00) >> 8;
    byte expected_value = opcode & 0x00FF;

    if (registers.at(v_register) != expected_value) {
      fetchOpcode();
    }

  } else if ((opcode & 0xF00F) == 0x5000) { /* 0x5XY0 */
    /* Skips the next instruction if VX equals VY. */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    unsigned y_register = (opcode & 0x00F0) >> 4;

    if (registers.at(x_register) == registers.at(y_register)) {
      fetchOpcode();
    }

  } else if ((opcode & 0xF000) == 0x6000) { /* 0x6XNN */
    /* Set VX to NN */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    byte new_value = opcode & 0x00FF;
    registers.at(x_register) = new_value;

  } else if ((opcode & 0xF000) == 0x7000) { /* 0x7XNN */
    /* Add NN to VX */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    byte added_value = opcode & 0x00FF;
    registers.at(x_register) += added_value;

  } else if ((opcode & 0xF00F) == 0x8000) { /* 0x8XY0 */
    /* Set VX to VY */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    unsigned y_register = (opcode & 0x00F0) >> 4;
    registers.at(x_register) = registers.at(y_register);

  } else if ((opcode & 0xF00F) == 0x8001) { /* 0x8XY1 */
    /* Set VX to VX OR VY */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    unsigned y_register = (opcode & 0x00F0) >> 4;
    registers.at(x_register) |= registers.at(y_register);

  } else if ((opcode & 0xF00F) == 0x8002) { /* 0x8XY2 */
    /* Set VX to VX AND VY */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    unsigned y_register = (opcode & 0x00F0) >> 4;
    registers.at(x_register) &= registers.at(y_register);

  } else if ((opcode & 0xF00F) == 0x8003) { /* 0x8XY3 */
    /* Set VX to VX XOR VY */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    unsigned y_register = (opcode & 0x00F0) >> 4;
    registers.at(x_register) ^= registers.at(y_register);

  } else if ((opcode & 0xF00F) == 0x8004) { /* 0x8XY4 */
    /* Add VY to VX and set VF if there is a carry */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    unsigned y_register = (opcode & 0x00F0) >> 4;
    byte x_register_value = registers.at(x_register);
    registers.at(x_register) += registers.at(y_register);
    registers.at(0xF) = x_register_value > registers.at(x_register);

  } else if ((opcode & 0xF00F) == 0x8005) { /* 0x8XY5 */
    /* Subtract VY from VX and set VF if there was no borrow */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    unsigned y_register = (opcode & 0x00F0) >> 4;
    byte x_register_value = registers.at(x_register);
    registers.at(x_register) -= registers.at(y_register);
    registers.at(0xF) = x_register_value >= registers.at(x_register);

  } else if ((opcode & 0xF00F) == 0x8006) { /* 0x8XY6 */
    /* Shift to the right and set VF to previous least significant bit */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    registers.at(0xF) = registers.at(x_register) & 1;
    registers.at(x_register) >>= 1;

  } else if ((opcode & 0xF00F) == 0x8007) { /* 0x8XY7 */
    /* Sets VX to VY minus VX. VF is set to 0 when there's a borrow, else 1 */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    unsigned y_register = (opcode & 0x00F0) >> 4;
    registers.at(x_register) = registers.at(y_register) - registers.at(x_register);
    registers.at(0xF) = registers.at(y_register) >= registers.at(x_register);

  } else if ((opcode & 0xF00F) == 0x800E) { /* 0x8XYE */
    /* Shifts VX left by one. VF is set to the most significant bit before the shift. */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    registers.at(0xF) = registers.at(x_register) & 0x80 ? 1 : 0;
    registers.at(x_register) <<= 1;

  } else if ((opcode & 0xF00F) == 0x9000) { /* 0x9XY0 */
    /* Skips the next instruction if VX doesn't equal VY. */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    unsigned y_register = (opcode & 0x00F0) >> 4;
    if (registers.at(x_register) != registers.at(y_register)) {
      fetchOpcode();
    }

  } else if ((opcode & 0xF000) == 0xA000) { /* 0xANNN */
    /* Sets I to the address NNN. */
    index_register = opcode & 0x0FFF;

  } else if ((opcode & 0xF000) == 0xB000) { /* 0xBNNN */
    /* Jumps to the address NNN plus V[0]. */
    program_counter = ((opcode & 0x0FFF) + registers.at(0)) % 0x1000;

  } else if ((opcode & 0xF000) == 0xC000) { /* 0xCXNN */
    /* Sets VX to a bitwise and operation on a random number and NN. */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    registers.at(x_register) = (rand() % 0xFF) & opcode & 0x00FF;

  } else if ((opcode & 0xF000) == 0xD000) { /* 0xDXYN */
    /* Sprites stored in memory at location in index register (I), 8bits wide.
     * Wraps around the screen. If when drawn, clears a pixel, register VF is
     * set to 1 otherwise it is zero. All drawing is XOR drawing (i.e. it
     * toggles the screen pixels). Sprites are drawn starting at position VX,
     * VY. N is the number of 8bit rows that need to be drawn. If N is greater
     * than 1, second line continues at position VX, VY+1, and so on. */
    unsigned sprite_x = registers.at((opcode & 0x0F00) >> 8);
    unsigned sprite_y = registers.at((opcode & 0x00F0) >> 4);
    unsigned num_rows = opcode & 0x000F;
    registers.at(0xF) = 0;

    unsigned sprite_x_bytes = sprite_x / 8;
    unsigned sprite_x_bits = sprite_x % 8;
    if (sprite_x_bits) {
      return;
    }

    for (unsigned y = 0; y < num_rows; ++y) {
      byte byte_graphics = ram.at(index_register + y);

      unsigned screen_pos = (sprite_x_bytes + ((sprite_y + y) * screen_columns));
      byte& screen_byte = screen.at(screen_pos % screen_bytes);

      if (screen_byte & byte_graphics) {
        registers.at(0xF) = 1;
      }

      byte processed_byte = 0;
      for (unsigned x = 0; x < 8; ++x) {
        if (byte_graphics & (0x80 >> x)) {
          processed_byte |= ((~screen_byte) & (0x80 >> x));
        } else {
          processed_byte |= (screen_byte & (0x80 >> x));
        }
      }

      for (unsigned i = 0; i < 8; ++i) {
        std::cout << (screen_byte & (0x80 >> i) ? "1" : "0");
      }
      std::cout << " | ";
      for (unsigned i = 0; i < 8; ++i) {
        std::cout << (byte_graphics & (0x80 >> i) ? "1" : "0");
      }
      std::cout << " = ";
      for (unsigned i = 0; i < 8; ++i) {
        std::cout << (processed_byte & (0x80 >> i) ? "1" : "0");
      }
      std::cout << "\n";
      screen_byte = processed_byte;
    }

    if (onGraphics != nullptr) {
      onGraphics();
    }

  } else if ((opcode & 0xF0FF) == 0xE09E) { /* 0xEX9E */
    /* Skips the next instruction if the key stored in VX is pressed. */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    if (keys_state.at(registers.at(x_register)) != 0) {
      fetchOpcode();
    }

  } else if ((opcode & 0xF0FF) == 0xE0A1) { /* 0xEXA1 */
    /* Skips the next instruction if the key stored in VX isn't pressed. */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    if (keys_state.at(registers.at(x_register)) == 0) {
      fetchOpcode();
    }

  } else if ((opcode & 0xF0FF) == 0xF007) { /* 0xFX07 */
    /* Sets VX to the value of the delay timer. */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    registers.at(x_register) = delay_timer;

  } else if ((opcode & 0xF0FF) == 0xF00A) { /* 0xFX0A */
    /* A key press is awaited, and then stored in VX. */
    /* Let's silently ignore this and handle somewhere else
     * goto not_implemented; */

  } else if ((opcode & 0xF0FF) == 0xF015) { /* 0xFX15 */
    /* Sets the delay timer to VX. */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    delay_timer = registers.at(x_register);

  } else if ((opcode & 0xF0FF) == 0xF018) { /* 0xFX18 */
    /* Sets the sound timer to VX. */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    sound_timer = registers.at(x_register);

  } else if ((opcode & 0xF0FF) == 0xF01E) { /* 0xFX1E */
    /* Adds VX to I. Also secretly sets VF to 1 on overflow else 0 */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    byte old_index = index_register;
    index_register = (index_register + registers.at(x_register)) % 0x1000;
    registers.at(0xF) = old_index > index_register;

  } else if ((opcode & 0xF0FF) == 0xF029) { /* 0xFX29 */
    /* Sets I to the location of the sprite for the character in VX.
     * Characters 0-F (in hexadecimal) are represented by a 4x5 font.
     * ( I have stored these fonts in the RAM, byte 0 and forward ) */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    index_register = registers[x_register] * 5;

  } else if ((opcode & 0xF0FF) == 0xF033) { /* 0xFX33 */
    /* Stores the Binary-coded decimal representation of VX, with the most
     * significant of three digits at the address in I, the middle digit at I
     * plus 1, and the least significant digit at I plus 2. (In other words,
     * take the decimal representation of VX, place the hundreds digit in memory
     * at location in I, the tens digit at location I+1, and the ones digit at
     * location I+2.) */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    byte value = registers.at(x_register);
    ram.at(index_register + 0) = value / 100;
    ram.at(index_register + 1) = value / 10 % 10;
    ram.at(index_register + 2) = value % 10;

  } else if ((opcode & 0xF0FF) == 0xF055) { /* 0xFX55 */
    /* Stores V0 to VX in memory starting at address I */
    /* TODO: Investigate if last index is inclusive */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    for (unsigned i = 0; i < x_register; ++i) {
      ram.at(index_register + i) = registers.at(i);
    }

  } else if ((opcode & 0xF0FF) == 0xF065) { /* 0xFX65 */
    /* Fills V0 to VX with values from memory starting at address I */
    unsigned x_register = (opcode & 0x0F00) >> 8;
    for (unsigned i = 0; i < x_register; ++i) {
      registers.at(i) = ram.at(index_register + i);
    }

  } else {
not_implemented:
  throw NotImplementedError("Opcode " + std::to_string(opcode) + " not implemented");
  }
}

void Emulator::tick() {
  if (tick_lock) {
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
