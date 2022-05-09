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


"int"  {  return token::INT;  }

"float" { return token::FLOAT; }

"const" { return token::CONST; }

"void" { return token::VOID; }

"return"  {  return token::RETURN;  }

"continue"  {  return token::CONTINUE;  }

"break" { return token::BREAK; }

"if"  {  return token::IF;  }

"else"  {  return token::ELSE;  }

"while"  {  return token::WHILE;  }

[A-Za-z_]([A-Za-z]|_|[0-9])* {
  yylval->build<std::string>(yytext);
  return(token::IDENTIFIER);
}

[1-9]([0-9])*	{
  yylval->build<int>(atoi(yytext));
	return token::IntConst;
}

[0](0-7)* {
    std::string temp = std::string(yytext);
    int val = 0;
    for(size_t i = 0;i<temp.size();i++){
        val = val * 8 + temp[i] - '0';
        i++;
    }
    yylval->build<int>(val);
    return token::IntConst;
}

["0x""0X"]([0-9a-fA-F])+   {
    std::string temp = std::string(yytext);
    int val = 0;
    for(size_t i=2;i<temp.size();i++){
        if(temp[i]>='0' and temp[i]<='9')
            val = val * 16 + temp[i] - '0';
        else if(temp[i]>='a' && temp[i]<='f')
            val = val*16 + temp[i]-'a';
        else
            val = val * 16 + temp[i]-'A';
        i++;
    }
    yylval->build<int>(val);
    return token::IntConst;
}

"&&" {  return token::LA;}

"||" { return token::LO;}

"!"  { return token::LN;}

"=="  {  return token::EQ;  }

"!="  {  return token::NE;  }

"<="  {  return token::LE;  }

"<"  {  return token::LT;  }

">="  {  return token::GE;  }

">"  {  return token::GT;  }

"+"  {  return token::ADD; }

"-"  {  return token::SUB; }

"*"  {  return token::MUL; }

"/"  {  return token::DIV; }

";"  {  return token::SEMI; }

"%"  {  return token::MOD; }

"="  {  return token::LEQ; }

","  {  return token::COM; }

"("  {  return token::LS; }

")"  {  return token::RS; }

"{"  {  return token::LB; }

"}"  {  return token::RB; }

([ \t\r])+ {
  loc->step();
}


\n {
  loc->lines(yyleng);
  loc->step();
}

<<EOF>> return token::END;

%%