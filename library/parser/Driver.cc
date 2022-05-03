#include <cassert>
#include <cctype>
#include <fstream>

#include "Driver.hh"

HaveFunCompiler::Parser::Driver::~Driver() {
  delete scanner;
  scanner = nullptr;
  delete parser;
  parser = nullptr;
}

void HaveFunCompiler::Parser::Driver::parse(const char *filename) {
  assert(filename != nullptr);
  std::ifstream in_file(filename);
  if (!in_file.good()) {
    exit(EXIT_FAILURE);
  }

  return parse_helper(in_file);
}

void HaveFunCompiler::Parser::Driver::parse(std::istream &stream) {
  if (!stream.good() || stream.eof()) {
    return;
  }
  return parse_helper(stream);
}

void HaveFunCompiler::Parser::Driver::parse_helper(std::istream &stream) {
  delete scanner;
  try {
    scanner = new HaveFunCompiler::Parser::Scanner(&stream);
  } catch (std::bad_alloc &ba) {
    std::cerr << "Failed to allocate scanner: (" << ba.what() << "), exiting!\n";
    exit(EXIT_FAILURE);
  }
  delete parser;

  try {
    parser = new HaveFunCompiler::Parser::Parser((*scanner), (*this));
  } catch (std::bad_alloc &ba) {
    std::cerr << "Failed to allocate parser: (" << ba.what() << "), exiting!\n";
    exit(EXIT_FAILURE);
  }
  const int accept(0);
  if (parser->parse() != accept) {
    std::cerr << "Parse failed!\n";
  }
  return;
}

void HaveFunCompiler::Parser::Driver::add_upper() {
  uppercase++;
  chars++;
  words++;
}

void HaveFunCompiler::Parser::Driver::add_lower() {
  lowercase++;
  chars++;
  words++;
}

void HaveFunCompiler::Parser::Driver::add_word(const std::string &word) {
  std::cout << "WORD" << std::endl;
  words++;
  chars += word.length();
  for (const char &c : word) {
    if (islower(c)) {
      lowercase++;
    } else if (isupper(c)) {
      uppercase++;
    }
  }
}

void HaveFunCompiler::Parser::Driver::add_newline() {
  lines++;
  chars++;
}

void HaveFunCompiler::Parser::Driver::add_char() { chars++; }

std::ostream &HaveFunCompiler::Parser::Driver::print(std::ostream &stream) {
  stream << red << "Results: " << norm << "\n";
  stream << blue << "Uppercase: " << norm << uppercase << "\n";
  stream << blue << "Lowercase: " << norm << lowercase << "\n";
  stream << blue << "Lines: " << norm << lines << "\n";
  stream << blue << "Words: " << norm << words << "\n";
  stream << blue << "Characters: " << norm << chars << "\n";
  return (stream);
}