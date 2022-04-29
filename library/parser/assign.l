%{
#include <stdlib.h>
#include "HFParser.h"

%}
%option nounput
%option yylineno

%%

[A-Za-z]([A-Za-z]|[0-9])*	{  
								yylval.str = strdup(yytext); 
								return IDENTIFIER;  
							}

[0-9]+	{  
			yylval.num = atoi(yytext); return INTEGER;  
		}

"+"|"-"|"*"|"/"|"("|")"|"="	{  
								return *yytext;  
							}

[ \t\r\n]

.	{
		printf("lexical error: line %d\n", yylineno);
		exit(0);
	}

%%

int yywrap()
{
	return 1;
}

