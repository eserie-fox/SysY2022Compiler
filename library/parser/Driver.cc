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

  try {
    tacbuilder = std::make_shared<HaveFunCompiler::ThreeAddressCode::TACBuilder>();
  } catch (std::bad_alloc &ba) {
    std::cerr << "Failed to allocate tacbuilder: (" << ba.what() << "), exiting!\n";
    exit(EXIT_FAILURE);
  }

  const int accept(0);
  if (parser->parse() != accept) {
    std::cerr << "Parse failed!\n";
  }
  return;
}


std::ostream &HaveFunCompiler::Parser::Driver::print(std::ostream &stream) {
  stream << tacbuilder->GetTACList()->ToString();
  return (stream);
}