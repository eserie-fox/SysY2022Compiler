#pragma once

#include <cstddef>
#include <istream>
#include <string>

#include "HFParser.hh"
#include "HFScanner.hh"

namespace HaveFunCompiler {
namespace Parser {
class Driver {
 public:
  Driver() = default;
  virtual ~Driver();

  void parse(const char *filename);

  void parse(std::istream &iss);

  void add_upper();

  void add_lower();

  void add_word(const std::string &word);

  void add_newline();

  void add_char();

  std::ostream &print(std::ostream &stream);

 private:
  void parse_helper(std::istream &stream);

  std::size_t chars = 0;
  std::size_t words = 0;
  std::size_t lines = 0;
  std::size_t uppercase = 0;
  std::size_t lowercase = 0;
  HaveFunCompiler::Parser::Parser *parser = nullptr;
  HaveFunCompiler::Parser::Scanner *scanner = nullptr;

  const std::string red = "\033[1;31m";
  const std::string blue = "\033[1;36m";
  const std::string norm = "\033[0m";
};
}  // namespace Parser
}  // namespace HaveFunCompiler