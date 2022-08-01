#pragma once

#if !defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer tacyyFlexLexer
#include "FlexLexer.h"
#endif

#include "../src_parser/location.hh"
#include "HFTACParser.hh"

namespace HaveFunCompiler {
namespace Parser {

class TACScanner : public yyFlexLexer {
 public:
  TACScanner(std::istream *in, std::shared_ptr<TACRebuilder> builder) : yyFlexLexer(in), builder_(builder) {}

  using FlexLexer::yylex;

  virtual int yylex(HaveFunCompiler::Parser::TACParser::semantic_type *const lval, location *location);

  location &get_location() { return *loc; }

 private:
  HaveFunCompiler::Parser::TACParser::semantic_type *yylval = nullptr;
  location *loc = nullptr;
  std::shared_ptr<TACRebuilder> builder_;
};
}  // namespace Parser
}  // namespace HaveFunCompiler