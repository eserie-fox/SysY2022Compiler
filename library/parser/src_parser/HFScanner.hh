#pragma once

#if !defined(yyFlexLexerOnce)
#include "FlexLexer.h"
#endif

#include "HFParser.hh"
#include "location.hh"

namespace HaveFunCompiler {
namespace Parser {
class Scanner : public yyFlexLexer {
 public:
  Scanner(std::istream *in,std::shared_ptr<TACBuilder> builder) : yyFlexLexer(in) ,builder_(builder){ }
  
  using FlexLexer::yylex;

  virtual int yylex(HaveFunCompiler::Parser::Parser::semantic_type *const lval,
                    HaveFunCompiler::Parser::Parser::location_type *location);

  HaveFunCompiler::Parser::Parser::location_type &get_location() { return *loc; }

 private:
  HaveFunCompiler::Parser::Parser::semantic_type *yylval = nullptr;
  HaveFunCompiler::Parser::Parser::location_type *loc = nullptr;
  std::shared_ptr<TACBuilder> builder_;
};
}  // namespace Parser
}  // namespace HaveFunCompiler