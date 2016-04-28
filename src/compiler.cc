#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <functional>

using namespace std;

namespace {

namespace io {

enum token_type {
  REG,
  NN
};

ofstream outfile;
ifstream infile;

void exit_ok() {
  infile.close();
  outfile.close();
  exit(0);
}

void exit_err(string const& error) {
  cerr << error;
  infile.close();
  outfile.close();
  exit(1);
}

void write_bins(char lhs, char rhs) {
  outfile.write(static_cast<char*>(&lhs), 1);
  outfile.write(static_cast<char*>(&rhs), 1);
  if (outfile.fail()) {
    exit_err("Error writing to disk");
  }
}

int xtoi(string const& x) {
  try {
    return stoi(x, 0, 16);
  } catch (exception &e) {
    return -1;
  }
}

string next_token() {
  string value;
  io::infile >> value;
  if (!io::infile) {
    io::exit_ok();
  }
  return value;
}

void tok2n(string const& name, string&& value, char& n) {
  int target = xtoi(value);
  if (0 > target || 0xF < target) {
    exit_err("Error: " + name + " needs to be supplied with a value [0-F]\n");
  }
  n = target;
}

void tok2nn(string const& name, string&& value, char& nn) {
  int target = xtoi(value);
  if (0 > target || 0xFF < target) {
    exit_err("Error: " + name + " needs to be supplied with a value [0-FF]\n");
  }
  nn = target;
}

void tok2nnn(string const& name, string&& value, char& lhs, char& rhs) {
  int target = xtoi(value);
  if (0 > target || 0xFFF < target) {
    exit_err("Error: " + name + " needs to be supplied with a value [0-FFF]\n");
  }
  lhs = target >> 8;
  rhs = target & 0xFF;
}

void tok2nnnn(string const& name, string&& value, char& lhs, char& rhs) {
  int target = xtoi(value);
  if (0 > target || 0xFFFF < target) {
    exit_err("Error: " + name + " needs to be supplied with a value [0-FFFF]\n");
  }
  lhs = target >> 8;
  rhs = target & 0xFF;
}

void tok2reg(string const& name, string&& value, char& reg) {
  if (value.at(0) != 'r') {
    exit_err("Error: " + name + " needs to be supplied with a register r[0-F]\n");
  }

  int target = xtoi(value.substr(1));
  if (0 > target || 0xF < target) {
    exit_err("Error: " + name + " needs to be supplied with a register r[0-F]\n");
  }
  reg = target;
}

void tok2reg_or_nn(string const& name, string&& value, char& reg_or_nn, io::token_type& t) {
  if (value.at(0) == 'r') {
    t = REG;
    tok2reg(name, forward<string>(value), reg_or_nn);
  } else {
    t = NN;
    tok2nn(name, forward<string>(value), reg_or_nn);
  }
}

} // io

namespace op {

// These variables are shared between all below functions
char lhs;
char rhs;
char rhs2;
io::token_type t;

inline void OP(char lhs, char rhs) { io::write_bins(lhs, rhs); }

inline void R(string const& name, char lop, char rop) {
  io::tok2reg(name, io::next_token(), lhs);
  io::write_bins(lop + lhs, rop);
}

inline void RR(string const& name, char lop, char rop) {
  io::tok2reg(name, io::next_token(), lhs);
  io::tok2reg(name, io::next_token(), rhs);
  io::write_bins(lop + lhs, (rhs << 4) + rop);
}

inline void NNN(string const& name, char op) {
  io::tok2nnn(name, io::next_token(), lhs, rhs);
  io::write_bins(op + lhs, rhs);
}

inline void RNN(string const& name, char lop) {
  io::tok2reg(name, io::next_token(), lhs);
  io::tok2nn(name, io::next_token(), rhs);
  io::write_bins(lop + lhs, rhs);
}

inline void RRN(string const& name, char lop) {
  io::tok2reg(name, io::next_token(), lhs);
  io::tok2reg(name, io::next_token(), rhs);
  io::tok2n(name, io::next_token(), rhs2);
  io::write_bins(lop + lhs, (rhs << 4) + rhs2);
}

inline void RV(string const& name, char nnop, char rlop, char rrop) {
  io::tok2reg(name, io::next_token(), lhs);
  io::tok2reg_or_nn(name, io::next_token(), rhs, t);
  if (t == io::NN)       { io::write_bins(nnop + lhs, rhs); }
  else if (t == io::REG) { io::write_bins(rlop + lhs, (rhs << 4) + rrop); }
  else                   { io::exit_err(name + ": Unknown type\n"); }
}

inline void DATA(string const& name) {
  io::tok2nnnn(name, io::next_token(), lhs, rhs);
  io::write_bins(lhs, rhs);
}

} // op


int help(string program_name) {
  cerr
    << "Usage: " << program_name << " FILE\n\n"
    << "Available commands\n"
    << "CLS          - 0x00E0\n"
    << "RET          - 0x00EE\n"
    << "JMP  NNN     - 0x1NNN\n"
    << "CALL NNN     - 0x2NNN\n"
    << "IFN  rX  NN  - 0x3XNN\n"
    << "IF   rX  NN  - 0x4XNN\n"
    << "IFN  rX  rY  - 0x5XY0\n"
    << "SET  rX  NN  - 0x6XNN\n"
    << "ADD  rX  NN  - 0x7XNN\n"
    << "SET  rX  rY  - 0x8XY0\n"
    << "OR   rX  rY  - 0x8XY1\n"
    << "AND  rX  rY  - 0x8XY2\n"
    << "XOR  rX  rY  - 0x8XY3\n"
    << "ADD  rX  rY  - 0x8XY4\n"
    << "SUB  rX  rY  - 0x8XY5\n"
    << "SHR  rX  rY  - 0x8XY6\n"
    << "RSUB rX  rY  - 0x8XY7\n"
    << "SHL  rX  rY  - 0x8XYE\n"
    << "IF   rX  rY  - 0x9XY0\n"
    << "IDX  NNN     - 0xANNN\n"
    << "JMP0 NNN     - 0xBNNN\n"
    << "RND  rX NN   - 0xCXNN\n"
    << "DRAW rX rY N - 0xDXYN\n"
    << "IFK  rX      - 0xEX9E\n"
    << "IFNK rX      - 0xEXA1\n"
    << "GDEL rX      - 0xFX07\n"
    << "WKEY rX      - 0xFX0A\n"
    << "SDEL rX      - 0xFX15\n"
    << "SAUD rX      - 0xFX18\n"
    << "IADD rX      - 0xFX1E\n"
    << "CHAR rX      - 0xFX29\n"
    << "SEP  rX      - 0xFX33\n"
    << "STOR rX      - 0xFX55\n"
    << "LOAD rX      - 0xFX65\n"
    << "DATA NNNN    - 0xNNNN\n"
    << "; comment with semicolon\n";

  return 1;
}


} // anonymous namespace

int main(int argc, char* argv[]) {
  if (argc != 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
    return help(argv[0]);
  }

  io::infile = ifstream(argv[1], ios::binary);
  if (!io::infile) {
    cerr << argv[0] << ": Unable to open " << argv[1] << "\n";
    return 1;
  }

  stringstream ss;
  ss << argv[1] << ".out";
  string const outfile_name{ss.str()};
  io::outfile = ofstream(outfile_name, ios::binary);
  if (!io::outfile) {
    cerr << argv[0] << ": Unable to open " << outfile_name << "\n";
    return 1;
  }

  map<string const, function<void()>> const op2fun {
    {"CLS",  bind(op::OP,   0x00, 0xE0) },
    {"RET",  bind(op::OP,   0x00, 0xEE) },
    {"JMP",  bind(op::NNN, "JMP", 0x10)},
    {"CALL", bind(op::NNN,"CALL", 0x20)},
    {"IFN",  bind(op::RV,  "IFN", 0x30, 0x50, 0x00)},
    {"IF",   bind(op::RV,   "IF", 0x40, 0x90, 0x00)},
    {"SET",  bind(op::RV,  "SET", 0x60, 0x80, 0x00)},
    {"ADD",  bind(op::RV,  "ADD", 0x70, 0x80, 0x04)},
    {"OR",   bind(op::RR,   "OR", 0x80, 0x01)},
    {"AND",  bind(op::RR,  "AND", 0x80, 0x02)},
    {"XOR",  bind(op::RR,  "XOR", 0x80, 0x03)},
    {"SUB",  bind(op::RR,  "SUB", 0x80, 0x05)},
    {"SHR",  bind(op::RR,  "SHR", 0x80, 0x06)},
    {"RSUB", bind(op::RR, "RSUB", 0x80, 0x07)},
    {"SHL",  bind(op::RR,  "SHL", 0x80, 0x0E)},
    {"IDX",  bind(op::NNN, "IDX", 0xA0)},
    {"JMP0", bind(op::NNN,"JMP0", 0xB0)},
    {"RND",  bind(op::RNN, "RND", 0xC0)},
    {"DRAW", bind(op::RRN,"DRAW", 0xD0)},
    {"IFK",  bind(op::R,   "IFK", 0xE0, 0x9E)},
    {"IFNK", bind(op::R,  "IFNK", 0xE0, 0xA1)},
    {"GDEL", bind(op::R,  "GDEL", 0xF0, 0x07)},
    {"WKEY", bind(op::R,  "WKEY", 0xF0, 0x0A)},
    {"SDEL", bind(op::R,  "SDEL", 0xF0, 0x15)},
    {"SAUD", bind(op::R,  "SAUD", 0xF0, 0x18)},
    {"IADD", bind(op::R,  "IADD", 0xF0, 0x1E)},
    {"CHAR", bind(op::R,  "CHAR", 0xF0, 0x29)},
    {"SEP",  bind(op::R,   "SEP", 0xF0, 0x33)},
    {"STOR", bind(op::R,  "STOR", 0xF0, 0x55)},
    {"LOAD", bind(op::R,  "LOAD", 0xF0, 0x65)},
    {"DATA",  bind(op::DATA, "DATA")},
  };


  for (;;) {
    // This constructor will exit when no more input is received
    string tok{io::next_token()};
    if (tok.at(0) == ';') {
      getline(io::infile, tok);
      continue;
    }

    auto const fun{op2fun.find(tok)};
    if (fun == op2fun.end()) {
      io::exit_err("Unknown command '" + tok + "'\n");
    }

    fun->second();
  }

  // Not reached
}
