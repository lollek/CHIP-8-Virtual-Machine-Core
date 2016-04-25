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
    << "ADD rX NN  - 0x7XNN\n"
    << "SET rX rY  - 0x8XY0\n";

  return 1;
}

namespace op {

// These variables are shared between all below functions
char lhs;
char rhs;
io::token_type t;

void CLS() { io::write_bins(0x00, 0xE0); }

void RET() { io::write_bins(0x00, 0xEE); }

void JMP() {
  io::tok2nnn("JMP", io::next_token(), lhs, rhs);
  io::write_bins(0x10 + lhs, rhs);
}

void CALL() {
  io::tok2nnn("CALL", io::next_token(), lhs, rhs);
  io::write_bins(0x20 + lhs, rhs);
}

void IFN() {
  io::tok2reg("IFN", io::next_token(), lhs);
  io::tok2reg_or_nn("IFN", io::next_token(), rhs, t);
  if (t == io::NN) {
    io::write_bins(0x30 + lhs, rhs);
  } else if (t == io::REG) {
    io::write_bins(0x50 + lhs, rhs << 4);
  } else {
    io::exit_err("IFN: Unknown type\n");
  }
}

void IF() {
  io::tok2reg("IF", io::next_token(), lhs);
  io::tok2nn("IF", io::next_token(), rhs);
  io::write_bins(0x40 + lhs, rhs);
}

void SET() {
  io::tok2reg("SET", io::next_token(), lhs);
  io::tok2reg_or_nn("SET", io::next_token(), rhs, t);
  if (t == io::NN) {
    io::write_bins(0x60 + lhs, rhs);
  } else if (t == io::REG) {
    io::write_bins(0x80 + lhs, rhs << 4);
  } else {
    io::exit_err("SET: Unknown type\n");
  }
}

void ADD() {
  io::tok2reg("ADD", io::next_token(), lhs);
  io::tok2nn("ADD", io::next_token(), rhs);
  io::write_bins(0x70 + lhs, rhs);
}

} // op


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
    {"CLS",  op::CLS},
    {"RET",  op::RET},
    {"JMP",  op::JMP},
    {"CALL", op::CALL},
    {"IFN",  op::IFN},
    {"IF",   op::IF},
    {"SET",  op::SET},
    {"ADD",  op::ADD},
  };


  for (;;) {
    // This constructor will exit when no more input is received
    string const tok{io::next_token()};

    auto const fun{op2fun.find(tok)};
    if (fun == op2fun.end()) {
      io::exit_err("Unknown command '" + tok + "'\n");
    }

    fun->second();
  }

  // Not reached
}
