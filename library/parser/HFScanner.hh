#pragma once

#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif

#include "HFParser.hh"
#include "location.hh"

namespace HaveFunCompiler {
namespace Parser {
class Scanner : public yyFlexLexer {
 public:
  Scanner(std::istream *in) : yyFlexLexer(in) { loc = new HaveFunCompiler::Parser::Parser::location_type(); }

  using FlexLexer::yylex;

  virtual int yylex(HaveFunCompiler::Parser::Parser::semantic_type *const lval,
                    HaveFunCompiler::Parser::Parser::location_type *location);

 private:
  HaveFunCompiler::Parser::Parser::semantic_type *yylval = nullptr;
  HaveFunCompiler::Parser::Parser::location_type *loc = nullptr;
};
}  // namespace Parser
}  // namespace HaveFunCompiler