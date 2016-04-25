#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <functional>

using namespace std;

namespace {
ofstream outfile;
ifstream infile;

bool write_bins(char lhs, char rhs) {
  outfile.write(static_cast<char*>(&lhs), 1);
  outfile.write(static_cast<char*>(&rhs), 1);
  return !outfile.fail();
}

bool get_nn(string const& name, char& nn) {
  int target;
  infile >> hex >> target;
  if (0 > target || 0xFF < target) {
    cerr << target << "Error: " << name << " needs to be supplied with a value [0-FF]\n";
    return false;
  }
  nn = target;
  return true;
}

bool get_nnn(string const& name, char& lhs, char& rhs) {
  int target;
  infile >> hex >> target;
  if (0 > target || 0xFFF < target) {
    cerr << target << "Error: " << name << " needs to be supplied with a value [0-FFF]\n";
    return false;
  }
  lhs = target >> 8;
  rhs = target & 0xFF;
  return true;
}

bool get_x(string const& name, char& xreg) {
  int target;
  infile >> hex >> target;
  if (0 > target || 0xF < target) {
    cerr << target << "Error: " << name << " needs to be supplied with a value [0-F]\n";
    return false;
  }
  xreg = target;
  return true;
}

bool cls() { return write_bins(0x00, 0xE0); }
bool ret() { return write_bins(0x00, 0xEE); }
bool jmp() {
  char lhs, rhs;
  return get_nnn("JMP", lhs, rhs) &&
         write_bins(0x10 + lhs, rhs);
}
bool call() {
  char lhs, rhs;
  return get_nnn("CALL", lhs, rhs) &&
         write_bins(0x20 + lhs, rhs);
}
bool ifn() {
  char xreg, nn;
  return get_x("IFN", xreg) &&
         get_nn("IFN", nn) &&
         write_bins(0x30 + xreg, nn);
}
bool if_() {
  char xreg, nn;
  return get_x("IF", xreg) &&
         get_nn("IF", nn) &&
         write_bins(0x40 + xreg, nn);
}

map<string const, function<bool()>> const op2fun {
  {"CLS",  cls},
  {"RET",  ret},
  {"JMP",  jmp},
  {"CALL", call},
  {"IFN",  ifn},
  {"IF",   if_},
};

int help(string program_name) {
  cerr
    << "Usage: " << program_name << " FILE\n\n"
    << "Available commands\n"
    << "CLS       - 0x00E0\n"
    << "RET       - 0x00EE\n"
    << "JMP NNN   - 0x1NNN\n"
    << "CALL NNN  - 0x2NNN\n"
    << "IFN X NN  - 0x3XNN\n"
    << "IF X NN   - 0x4XNN\n";

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
  string const outfile_name = ss.str();
  outfile = ofstream(outfile_name, ios::binary);
  if (!outfile) {
    cerr << argv[0] << ": Unable to open " << outfile_name << "\n";
    return 1;
  }

  for (;;) {
    string s;
    infile >> s;
    if (!infile) {
      break;
    }

    auto fun = op2fun.find(s);
    if (fun == op2fun.end()) {
      cerr << "Unknown command '" << s << "'\n";
      return 1;
    }

    fun->second();
  }

  infile.close();
  outfile.flush();
  outfile.close();
  return 0;
}
