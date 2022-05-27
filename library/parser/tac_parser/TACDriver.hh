#pragma once

#include <cstddef>
#include <istream>
#include <memory>
#include <string>

#include "HFTACParser.hh"
#include "HFTACScanner.hh"
#include "TAC/TAC.hh"

namespace HaveFunCompiler {
namespace Parser {
class TACDriver {
 public:
  TACDriver() = default;
  virtual ~TACDriver();

  bool parse(const char *filename);

  bool parse(std::istream &iss);

  std::ostream &print(std::ostream &stream);

  std::shared_ptr<HaveFunCompiler::ThreeAddressCode::TACRebuilder> get_tacbuilder() const { return tacbuilder; }

 private:
  bool parse_helper(std::istream &stream);

  HaveFunCompiler::Parser::TACParser *parser = nullptr;
  HaveFunCompiler::Parser::TACScanner *scanner = nullptr;
  std::shared_ptr<HaveFunCompiler::ThreeAddressCode::TACRebuilder> tacbuilder = nullptr;

  // const std::string red = "\033[1;31m";
  // const std::string blue = "\033[1;36m";
  // const std::string norm = "\033[0m";
};
}  // namespace Parser
}  // namespace HaveFunCompiler