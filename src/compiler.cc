#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <functional>

using namespace std;

namespace {
ofstream outfile;
ifstream infile;

class Token {
public:
  explicit Token() {
    infile >> value;
    if (!infile) {
      infile.close();
      outfile.close();
      exit(0);
    }
  }

  string const& as_str() {
    return value;
  }

  bool make_nn(string const& name, char& nn) {
    int target = xtoi(value);
    if (0 > target || 0xFF < target) {
      cerr << "Error: " << name << " needs to be supplied with a value [0-FF]\n";
      return false;
    }
    nn = target;
    return true;
  }

  bool make_nnn(string const& name, char& lhs, char& rhs) {
    int target = xtoi(value);
    if (0 > target || 0xFFF < target) {
      cerr << "Error: " << name << " needs to be supplied with a value [0-FFF]\n";
      return false;
    }
    lhs = target >> 8;
    rhs = target & 0xFF;
    return true;
  }

  bool make_reg(string const& name, char& reg) {
    if (value.at(0) != 'r') {
      cerr << "Error: " << name << " needs to be supplied with a register r[0-F]\n";
      reg = -1;
      return false;
    }

    int target = xtoi(value.substr(1));
    if (0 > target || 0xF < target) {
      cerr << "Error: " << name << " needs to be supplied with a register r[0-F]\n";
      return false;
    }
    reg = target;
    return true;
  }

private:
  int xtoi(string const& x) {
    try {
      return stoi(x, 0, 16);
    } catch (exception &e) {
      return -1;
    }
  }

  string value;
};

bool write_bins(char lhs, char rhs) {
  outfile.write(static_cast<char*>(&lhs), 1);
  outfile.write(static_cast<char*>(&rhs), 1);
  return !outfile.fail();
}


bool cls() { return write_bins(0x00, 0xE0); }
bool ret() { return write_bins(0x00, 0xEE); }
bool jmp() {
  char lhs, rhs;
  return Token().make_nnn("JMP", lhs, rhs) &&
         write_bins(0x10 + lhs, rhs);
}
bool call() {
  char lhs, rhs;
  return Token().make_nnn("CALL", lhs, rhs) &&
         write_bins(0x20 + lhs, rhs);
}
bool ifn() {
  char xreg, nn;
  return Token().make_reg("IFN", xreg) &&
         Token().make_nn("IFN", nn) &&
         write_bins(0x30 + xreg, nn);
}
bool if_() {
  char xreg, nn;
  return Token().make_reg("IF", xreg) &&
         Token().make_nn("IF", nn) &&
         write_bins(0x40 + xreg, nn);
}

int help(string program_name) {
  cerr
    << "Usage: " << program_name << " FILE\n\n"
    << "Available commands\n"
    << "CLS        - 0x00E0\n"
    << "RET        - 0x00EE\n"
    << "JMP NNN    - 0x1NNN\n"
    << "CALL NNN   - 0x2NNN\n"
    << "IFN rX NN  - 0x3XNN\n"
    << "IF rX NN   - 0x4XNN\n"
    << "IFN rX rY  - 0x5XY0\n";

  return 1;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
  if (argc != 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
    return help(argv[0]);
  }

  infile = ifstream(argv[1], ios::binary);
  if (!infile) {
    cerr << argv[0] << ": Unable to open " << argv[1] << "\n";
    return 1;
  }

  stringstream ss;
  ss << argv[1] << ".out";
  string const outfile_name{ss.str()};
  outfile = ofstream(outfile_name, ios::binary);
  if (!outfile) {
    cerr << argv[0] << ": Unable to open " << outfile_name << "\n";
    return 1;
  }

  map<string const, function<bool()>> const op2fun {
    {"CLS",  cls},
    {"RET",  ret},
    {"JMP",  jmp},
    {"CALL", call},
    {"IFN",  ifn},
    {"IF",   if_},
  };


  for (;;) {
    // This constructor will exit when no more input is received
    Token tok;

    auto const fun{op2fun.find(tok.as_str())};
    if (fun == op2fun.end()) {
      cerr << "Unknown command '" << tok.as_str() << "'\n";
      return 1;
    }

    if (!fun->second()) {
      return 1;
    }
  }

  // Not reached
}
