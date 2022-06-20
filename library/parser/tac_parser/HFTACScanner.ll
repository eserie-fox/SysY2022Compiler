%{
#include <string>

#include "HFTACScanner.hh"
#undef YY_DECL
#define YY_DECL int HaveFunCompiler::Parser::TACScanner::yylex(HaveFunCompiler::Parser::TACParser::semantic_type *const lval,\
                   HaveFunCompiler::Parser::location *location)

using token = HaveFunCompiler::Parser::TACParser::token;

#define yyterminate() return(token::END)

#define YY_NO_UNISTD_H

#define YY_USER_ACTION loc->step();loc->columns(yyleng);

%}

%option debug
%option nodefault
%option yyclass="HaveFunCompiler::Parser::TACScanner"
%option noyywrap
%option c++

%%

%{
  yylval = lval;
  loc = location;
  builder_->SetLocation(location);
%}


[/][/].*[\n] {

}

("/*")(.|\n)*("*/") {

}

"int"  {  return token::INT;  }

"float" { return token::FLOAT; }

"const" { return token::CONST; }

"ret"  {  return token::RET;  }

"ifz"  {  return token::IFZ;  }

"label" { return token::LABEL; }

"goto" { return token::GOTO; }

"string" { return token::STRING; }

"fbegin" { return token::FBEGIN; }

"fend" { return token::FEND; }

"arg" { return token::ARG; }

"param" { return token::PARAM; }

"call" { return token::CALL; }



[A-Za-z_]([A-Za-z]|_|[0-9])* {
  yylval->build<std::string>(yytext);
  return(token::IDENTIFIER);
}

[1-9]([0-9])*	{
  yylval->build<int>(atoi(yytext));
	return token::IntConst;
}

[0]([0-9])* {
    std::string temp = std::string(yytext);
    int val = 0;
    for(size_t i = 0;i<temp.size();i++){
      if(temp[i]-'0'>7){
        std::cerr<<"Illegal-octal-digit: at line"<<*loc<<std::endl;
        exit(EXIT_FAILURE);
      }
        val = val * 8 + temp[i] - '0';
    }
    yylval->build<int>(val);
    return token::IntConst;
}

[0][xX]([A-F]|[a-f]|[0-9])+  {
    std::string temp = std::string(yytext);
    int val = 0;
    for(size_t i=2;i<temp.size();i++){
      if(temp[i]>='0' && temp[i]<='9')
        val = val * 16 + temp[i] - '0';
      else if(temp[i]>='a' && temp[i]<='f')
        val = val*16 + temp[i]-'a';
      else
        val = val * 16 + temp[i]-'A';
    }
    yylval->build<int>(val);
    return token::IntConst;
}

[0-9]*[.][0-9]*([eE]([+-])?[0-9]+)?([flFL])?   {
  std::string floatNum = std::string(yytext);
  if(floatNum[floatNum.size()-1]<'0' || floatNum[floatNum.size()-1]>'9')
    floatNum = floatNum.substr(0, floatNum.size()-1);
  yylval->build<float>(std::stof(floatNum));
  return token::floatConst;
}

[0-9]+[eE]([+-])?[0-9]+([flFL])?  {
  std::string floatNum = std::string(yytext);
  if(floatNum[floatNum.size()-1]<'0' || floatNum[floatNum.size()-1]>'9')
    floatNum = floatNum.substr(0, floatNum.size()-1);
  yylval->build<float>(std::stof(floatNum));
  return token::floatConst;
}

[0][xX](([A-F]|[a-f]|[0-9])+)?[.]([A-F]|[a-f]|[0-9])*[pP]([+-])?[0-9]+([flFL])?  {
  std::string floatNum = std::string(yytext);
  if(floatNum[floatNum.size()-1]<'0' || floatNum[floatNum.size()-1]>'9')
    floatNum = floatNum.substr(0, floatNum.size()-1);
  yylval->build<float>(std::stof(floatNum));
  return token::floatConst;
}

[0][xX][0-9A-Fa-f]+[pP]([+-])?[0-9]+([flFL])?  {
  std::string floatNum = std::string(yytext);
  if(floatNum[floatNum.size()-1]<'0' || floatNum[floatNum.size()-1]>'9')
    floatNum = floatNum.substr(0, floatNum.size()-1);
  yylval->build<float>(std::stof(floatNum));
  return token::floatConst;
}

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

"%"  {  return token::MOD; }

"="  {  return token::LEQ; }
"("  {  return token::LS; }

")"  {  return token::RS; }

"["  {  return token::LM; }

"]"  {  return token::RM; }

"&" { return token::AND; }

([ \t\r])+ {

}


\n+ {
  loc->lines(yyleng);
  loc->step();
}


<<EOF>> return token::END;

%%