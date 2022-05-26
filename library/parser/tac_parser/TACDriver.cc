#include <cassert>
#include <cctype>
#include <fstream>

#include "TACDriver.hh"

HaveFunCompiler::TACParser::TACDriver::~TACDriver() {
  delete scanner;
  scanner = nullptr;
  delete parser;
  parser = nullptr;
}

bool HaveFunCompiler::TACParser::TACDriver::parse(const char *filename) {
  assert(filename != nullptr);
  std::ifstream in_file(filename);
  if (!in_file.good()) {
    exit(EXIT_FAILURE);
  }

  return parse_helper(in_file);
}

bool HaveFunCompiler::TACParser::TACDriver::parse(std::istream &stream) {
  if (!stream.good() || stream.eof()) {
    return false;
  }
  return parse_helper(stream);
}

bool HaveFunCompiler::TACParser::TACDriver::parse_helper(std::istream &stream) {
  try {
    tacbuilder = std::make_shared<HaveFunCompiler::ThreeAddressCode::TACBuilder>();
  } catch (std::bad_alloc &ba) {
    std::cerr << "Failed to allocate tacbuilder: (" << ba.what() << "), exiting!\n";
    exit(EXIT_FAILURE);
  }

  delete scanner;
  try {
    scanner = new HaveFunCompiler::TACParser::TACScanner(&stream, tacbuilder);
  } catch (std::bad_alloc &ba) {
    std::cerr << "Failed to allocate scanner: (" << ba.what() << "), exiting!\n";
    exit(EXIT_FAILURE);
  }
  delete parser;

  try {
    parser = new HaveFunCompiler::TACParser::TACParser((*scanner), (*this));
  } catch (std::bad_alloc &ba) {
    std::cerr << "Failed to allocate parser: (" << ba.what() << "), exiting!\n";
    exit(EXIT_FAILURE);
  }



  const int accept(0);
  if (parser->parse() != accept) {
    std::cerr << "Parse failed!\n";
    return false;
  }
  return true;
}

std::ostream &HaveFunCompiler::TACParser::TACDriver::print(std::ostream &stream) {
  stream << tacbuilder->GetTACList()->ToString();
  return (stream);
}