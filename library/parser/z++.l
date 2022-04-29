%{
#include <stdlib.h>
#include <string>
#include "HFParser.h"

%}
%option nounput
%option yylineno
%option bison-bridge

%%

"int"  {  return INT;  }

"float" { return FLOAT; }

"const" { return CONST; }

"void" { return VOID; }

"return"  {  return RETURN;  }

"continue"  {  return CONTINUE;  }

"break" { return BREAK; }

"if"  {  return IF;  }

"else"  {  return ELSE;  }

"while"  {  return WHILE;  }

[A-Za-z|_]([A-Za-z]|_|[0-9])*  {  
    yylval.emplace<std::string>(yytext);
	return IDENTIFIER;
}

[1-9]([0-9])*	{
    yylval.emplace<int>(atoi(yytext));
	return INTEGER;
}

[0](0-7)* {
    std::string temp = std::string(yytext);
    int val = 0;
    for(size_t i = 0;i<temp.size();i++){
        val = val * 8 + temp[i] - '0';
        i++;
    }
    yylval.emplace<int>(val);
    return INTEGER;
}

["0x"|"0X"]([0-9|a-f|A-F])+   {
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
    yylval.emplace<int>(val);
    return INTEGER;
}

"&&" {  return LA;}

"||" { return LO;}

"!"  { return LN;}

"=="  {  return EQ;  }

"!="  {  return NE;  }

"<="  {  return LE;  }

"<"  {  return LT;  }

">="  {  return GE;  }

">"  {  return GT;  }

[ \t\r\n]|#.*

. 	{	return *yytext; }

%%

int yywrap()
{
	return 1;
}
