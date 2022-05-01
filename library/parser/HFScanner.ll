%{
#include <string>

#include "HFScanner.hh"
#undef YY_DECL
#define YY_DECL int HaveFunCompiler::Parser::Scanner::yylex(HaveFunCompiler::Parser::Parser::semantic_type *const lval,\
                   [[maybe_unused]] HaveFunCompiler::Parser::Parser::location_type *location)

using token = HaveFunCompiler::Parser::Parser::token;

#define yyterminate() return(token::END)

#define YY_NO_UNISTD_H

#define YY_USER_ACTION loc->step();loc->columns(yyleng);

%}

%option debug
%option nodefault
%option yyclass="HaveFunCompiler::Parser::Scanner"
%option noyywrap
%option c++

%%

%{
  yylval = lval;

%}


[a-z] {
  return(token::LOWER);
}

[A-Z] {
  return(token::UPPER);
}

[a-zA-Z]+ {
  yylval->build<std::string>(yytext);
  return(token::WORD);
}

\n {
  loc->lines();
  return(token::NEWLINE);
}

. {
  return(token::CHAR);
}

%%