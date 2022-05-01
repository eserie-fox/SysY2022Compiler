%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <variant>
#include <unordered_map>
//#include "HFLexer.hh"

void yyerror(const char* msg);
std::unordered_map<std::string,int> val_map;
%}

%require "3.2"
%language "c++"

%define api.value.type variant
%define api.token.constructor

// %union
// {
// 	int num;
// 	std::string str;
// }

%left '+' '-'
%left '*' '/'

%token INT EQ NE LT LE GT GE IF ELSE WHILE CONTINUE RETURN LA LO LN FLOAT CONST VOID BREAK
%token <int> INTEGER
%token <std::string> IDENTIFIER

%type <int> expression
%type <int> assignment
%type <int> assignments

%%

start: assignments_list

assignments_list : 
  assignments_list assignments |
  assignments ;

assignments : 
	IDENTIFIER '=' assignments{
		$$ = $3;
    val_map[std::string($1)] = $$;
		printf("%s: %d\n",$1,$3);
	} |
	assignment ;

assignment: IDENTIFIER '=' expression  
{  
	$$ = $3;
  val_map[std::string($1)] = $$;
	printf("%s: %d\n", $1, $3); 
}
;

expression: INTEGER  			{  $$ = $1;  }
| IDENTIFIER { $$ = val_map[std::string($1)];  }
| expression '+' expression		{  $$ = $1 + $3;  }
| expression '-' expression  	{  $$ = $1 - $3;  }
| expression '*' expression  	{  $$ = $1 * $3;  }
| expression '/' expression  	{  $$ = $1 / $3;  }
| '(' expression ')'  			{  $$ = $2;  }
;

%%
	
void yyerror(const char* msg) 
{
	printf("%s: line %d\n", msg, yylineno);
	exit(0);
}

int main(int argc, char *argv[]) 
{
	if(argc!=2) {
		printf("usage: %s filename\n", argv[0]);
		exit(0);
	}			

	if( (yyin=fopen(argv[1], "r")) ==NULL )
	{
		printf("open file %s failed\n", argv[1]);
		exit(0);
	}

	yyparse();

	fclose(yyin);
	return 0;
}

