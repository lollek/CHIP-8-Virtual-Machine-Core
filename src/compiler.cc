#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <functional>

using namespace std;

namespace {
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

class Token {
public:
  explicit Token() {
    infile >> value;
    if (!infile) {
      exit_ok();
    }
  }

  enum type {
    REG,
    NN
  };

  string const& as_str() {
    return value;
  }

  void make_nn(string const& name, char& nn) {
    int target = xtoi(value);
    if (0 > target || 0xFF < target) {
      exit_err("Error: " + name + " needs to be supplied with a value [0-FF]\n");
    }
    nn = target;
  }

  void make_nnn(string const& name, char& lhs, char& rhs) {
    int target = xtoi(value);
    if (0 > target || 0xFFF < target) {
      exit_err("Error: " + name + " needs to be supplied with a value [0-FFF]\n");
    }
    lhs = target >> 8;
    rhs = target & 0xFF;
  }

  void make_reg(string const& name, char& reg) {
    if (value.at(0) != 'r') {
      exit_err("Error: " + name + " needs to be supplied with a register r[0-F]\n");
    }

    int target = xtoi(value.substr(1));
    if (0 > target || 0xF < target) {
      exit_err("Error: " + name + " needs to be supplied with a register r[0-F]\n");
    }
    reg = target;
  }

  void make_reg_or_nn(string const& name, char& reg_or_nn, type& t) {
    if (value.at(0) == 'r') {
      t = REG;
      make_reg(name, reg_or_nn);
    } else {
      t = NN;
      make_nn(name, reg_or_nn);
    }
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

void write_bins(char lhs, char rhs) {
  outfile.write(static_cast<char*>(&lhs), 1);
  outfile.write(static_cast<char*>(&rhs), 1);
  if (outfile.fail()) {
    exit_err("Error writing to disk");
  }
}


void cls() { write_bins(0x00, 0xE0); }
void ret() { write_bins(0x00, 0xEE); }
void jmp() {
  char lhs, rhs;
  Token().make_nnn("JMP", lhs, rhs);
  write_bins(0x10 + lhs, rhs);
}
void call() {
  char lhs, rhs;
  Token().make_nnn("CALL", lhs, rhs);
  write_bins(0x20 + lhs, rhs);
}
void ifn() {
  char xreg, nn;
  Token::type t;
  Token().make_reg("IFN", xreg);
  Token().make_reg_or_nn("IFN", nn, t);
  if (t == Token::NN) {
    write_bins(0x30 + xreg, nn);
  } else if (t == Token::REG) {
    write_bins(0x50 + xreg, nn << 4);
  } else {
    exit_err("IFN: Unknown type\n");
  }
}
void if_() {
  char xreg, nn;
  Token().make_reg("IF", xreg);
  Token().make_nn("IF", nn);
  write_bins(0x40 + xreg, nn);
}
void set() {
  char xreg, nn;
  Token().make_reg("SET", xreg);
  Token().make_nn("SET", nn);
  write_bins(0x60 + xreg, nn);
}
void add() {
  char xreg, nn;
  Token().make_reg("ADD", xreg);
  Token().make_nn("ADD", nn);
  write_bins(0x70 + xreg, nn);
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
    << "IFN rX rY  - 0x5XY0\n"
    << "SET rX NN  - 0x6XNN\n"
    << "ADD rX NN  - 0x7XNN\n";

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

  map<string const, function<void()>> const op2fun {
    {"CLS",  cls},
    {"RET",  ret},
    {"JMP",  jmp},
    {"CALL", call},
    {"IFN",  ifn},
    {"IF",   if_},
    {"SET",   set},
    {"ADD",   add},
  };


  for (;;) {
    // This constructor will exit when no more input is received
    Token tok;

    auto const fun{op2fun.find(tok.as_str())};
    if (fun == op2fun.end()) {
      exit_err("Unknown command '" + tok.as_str() + "'\n");
    }

    fun->second();
  }

  // Not reached
}
