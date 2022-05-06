#pragma once

#include <cstddef>
#include <istream>
#include <string>
#include <memory>

#include "TAC/TAC.hh"
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

  std::ostream &print(std::ostream &stream);

  std::shared_ptr<HaveFunCompiler::ThreeAddressCode::TACBuilder> get_tacbuilder() const { return tacbuilder; }

 private:
  void parse_helper(std::istream &stream);

  HaveFunCompiler::Parser::Parser *parser = nullptr;
  HaveFunCompiler::Parser::Scanner *scanner = nullptr;
  std::shared_ptr<HaveFunCompiler::ThreeAddressCode::TACBuilder> tacbuilder = nullptr;

  // const std::string red = "\033[1;31m";
  // const std::string blue = "\033[1;36m";
  // const std::string norm = "\033[0m";
};
}  // namespace Parser
}  // namespace HaveFunCompiler