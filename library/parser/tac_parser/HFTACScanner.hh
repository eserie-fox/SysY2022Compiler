#pragma once

#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#include "HFTACParser.hh"
#include "../src_parser/location.hh"

namespace HaveFunCompiler {
namespace TACParser {

using location = HaveFunCompiler::Parser::location;

class TACScanner : public yyFlexLexer {
 public:
  TACScanner(std::istream *in, std::shared_ptr<TACBuilder> builder) : yyFlexLexer(in), builder_(builder) {}

  using FlexLexer::yylex;

  virtual int yylex(HaveFunCompiler::TACParser::TACParser::semantic_type *const lval, location *location);

  location &get_location() { return *loc; }

 private:
  HaveFunCompiler::TACParser::TACParser::semantic_type *yylval = nullptr;
  location *loc = nullptr;
  std::shared_ptr<TACBuilder> builder_;
};
}  // namespace TACParser
}  // namespace HaveFunCompiler