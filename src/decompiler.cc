#include <iomanip>
#include <iostream>
#include <fstream>

using namespace std;

void describe_opcode(int line, int opcode);

inline int op_x_value(int opcode) {
  return (opcode & 0x0F00) >> 8;
}
inline int op_y_value(int opcode) {
  return (opcode & 0x00F0) >> 4;
}
inline int op_z_value(int opcode) {
  return opcode & 0x000F;
}
inline int op_nnn_value(int opcode) {
  return opcode & 0x0FFF;
}
inline int op_nn_value(int opcode) {
  return opcode & 0x00FF;
}

void describe_opcode(int line, int opcode) {
  cout
    << "L" << hex << line << ": "
    << hex << setfill('0') << setw(4) << opcode << " - ";

  switch (opcode & 0xF000) {
    case 0x0000:
      switch (opcode) {
        case 0x00E0: cout << "Clear screen"; break;
        case 0x00EE: cout << "RET"; break;
        default:     cout << "RCA 1802 (deprecated opcode)"; break;
      } break;

    case 0x1000:
      cout << "JMP " << hex << setfill('0') << setw(3) << op_nnn_value(opcode);
      break;

    case 0x2000:
      cout << "CALL " << hex << setfill('0') << setw(3) << op_nnn_value(opcode);
      break;

    case 0x3000:
      cout << "IF reg[" << hex << op_x_value(opcode)
           << "] != " << hex << setfill('0') << setw(2) << op_nn_value(opcode) << ":";
      break;

    case 0x4000:
      cout << "IF reg[" << hex << op_x_value(opcode)
           << "] == " << hex << setfill('0') << setw(2) << op_nn_value(opcode) << ":";
      break;

    case 0x5000:
      cout << "IF reg[" << hex << op_x_value(opcode)
           << "] != reg[" << hex << op_y_value(opcode) << "]:";
      break;

    case 0x6000:
      cout << "reg[" << hex << op_x_value(opcode)
           << "] = " << hex << setfill('0') << setw(2) << op_nn_value(opcode);
      break;

    case 0x7000:
      cout << "reg[" << hex << op_x_value(opcode)
           << "] += " << hex << setfill('0') << setw(2) << op_nn_value(opcode);
      break;

    case 0x8000:
      switch (opcode & 0xF00F) {
        case 0x8000:
          cout << "reg[" << hex << op_x_value(opcode)
               << "] = reg[" << hex << op_y_value(opcode) << "]";
          break;

        case 0x8001:
          cout << "reg[" << hex << op_x_value(opcode)
               << "] |= reg[" << hex << op_y_value(opcode) << "]";
          break;

        case 0x8002:
          cout << "reg[" << hex << op_x_value(opcode)
               << "] &= reg[" << hex << op_y_value(opcode) << "]";
          break;

        case 0x8003:
          cout << "reg[" << hex << op_x_value(opcode)
               << "] ^= reg[" << hex << op_y_value(opcode) << "]";
          break;

        case 0x8004:
          cout << "reg[" << hex << op_x_value(opcode)
               << "] += reg[" << hex << op_y_value(opcode)
               << "] (modifies reg[f])";
          break;

        case 0x8005:
          cout << "reg[" << hex << op_x_value(opcode)
               << "] -= reg[" << hex << op_y_value(opcode)
               << "] (modifies reg[f])";
          break;

        case 0x8006:
          cout << "reg[" << hex << op_x_value(opcode)
               << "] = reg[" << hex << op_y_value(opcode)
               << "] >>= 1 (modifies reg[f])";
          break;

        case 0x8007:
          cout << "reg[" << hex << op_x_value(opcode)
               << "] = reg[" << hex << op_y_value(opcode)
               << "] - reg[" << hex << op_x_value(opcode)
               << "] (modifies reg[f])";
          break;

        case 0x800E:
          cout << "reg[" << hex << op_x_value(opcode)
               << "] = reg[" << hex << op_y_value(opcode)
               << "] <<= 1 (modifies reg[f])";
          break;

        default:
          cout << "<Possibly a sprite?>";
          break;
      } break;

    case 0x9000:
          cout << "IF reg[" << hex << op_x_value(opcode)
               << "] == reg[" << hex << op_y_value(opcode)
               << "]:";
          break;

    case 0xa000:
          cout << "I = " << hex << setw(3) << setfill('0') << op_nnn_value(opcode);
          break;

    case 0xb000:
          cout << "JMP " << hex << setw(3) << setfill('0') << op_nnn_value(opcode)
               << " + reg[0]";
          break;

    case 0xc000:
          cout << "reg[" << hex << op_x_value(opcode) << "] = rand() & "
               << hex << setw(2) << setfill('0') << op_nn_value(opcode);
          break;

    case 0xd000:
          cout << "DRAW " << op_z_value(opcode) << " sprites to x/y: reg["
               << hex << op_x_value(opcode) << "]/reg["
               << hex << op_y_value(opcode) << "]";
          break;

    case 0xe000:
          switch (opcode & 0xF0FF) {
            case 0xE09E:
              cout << "IF KEY PRESSED @ reg[" << hex << op_x_value(opcode)
                   << "]:";
              break;

            case 0xE0A1:
              cout << "IF KEY NOT PRESSED @ reg[" << hex << op_x_value(opcode)
                   << "]:";
              break;

            default:
              cout << "<Possibly a sprite?>";
              break;
          } break;

    case 0xf000:
        switch (opcode & 0xF0FF) {
          case 0xF007:
            cout << "reg[" << hex << op_x_value(opcode) << "] = delay_timer";
            break;

          case 0xF00A:
            cout << "AWAIT KEYPRESS TO reg[" << hex << op_x_value(opcode) << "]";
            break;

          case 0xF015:
            cout << "delay_timer = reg[" << hex << op_x_value(opcode) << "]";
            break;

          case 0xF018:
            cout << "sound_timer = reg[" << hex << op_x_value(opcode) << "]";
            break;

          case 0xF01E:
            cout << "I += reg[" << hex << op_x_value(opcode) << "] (modifies reg[f])";
            break;

          case 0xF029:
            cout << "I = character representation of reg[" << hex << op_x_value(opcode)
                 << "]";
            break;

          case 0xF033:
            cout << "Write reg[" << hex << op_x_value(opcode) << "] to I/I+1/I+2";
            break;

          case 0xF055:
            cout << "STORE data from reg[0] - reg[" << hex << op_x_value(opcode)
                 << "] to I";
            break;

          case 0xF065:
            cout << "LOAD data from I to reg[0] - reg[" << hex << op_x_value(opcode)
                 << "]";
            break;

          default:
            cout << "<Possibly a sprite?>";
            break;
        }
      break;
  }

  cout << "\n";
}

int main(int argc, char* argv[]) {
  if (argc != 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
    cerr << "Usage: " << argv[0] << " FILE\n";
    return 1;
  }

  ifstream file(argv[1], ios::binary);
  if (!file) {
    cerr << argv[0] << ": Unable to open " << argv[1] << "\n";
    return 1;
  }

  int line = 0x200;
  for (;;) {
    int rhs;
    file.read(reinterpret_cast<char*>(&rhs), 1);
    int lhs;
    file.read(reinterpret_cast<char*>(&lhs), 1);

    if (!file) {
      break;
    }

    describe_opcode(line, (rhs << 8) + lhs);
    line += 2;
  }

  file.close();
  return 0;
}
