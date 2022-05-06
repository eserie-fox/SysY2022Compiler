%skeleton "lalr1.cc"
%require "3.0"
%debug
%defines
%define api.namespace {HaveFunCompiler::Parser}
%define api.parser.class {Parser}


%code requires{
  namespace HaveFunCompiler{
    namespace Parser{
      class Driver;
      class Scanner;
    }
  }

#include "TAC/TAC.hh"
using TACListPtr = HaveFunCompiler::ThreeAddressCode::TACListPtr;
using ExpressionPtr = HaveFunCompiler::ThreeAddressCode::ExpressionPtr;
using TACFactory = HaveFunCompiler::ThreeAddressCode::TACFactory;
using ValueType =  HaveFunCompiler::ThreeAddressCode::SymbolValue::ValueType;

// The following definitions is missing when %locations isn't used
# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif




}

%parse-param {Scanner &scanner}
%parse-param {Driver &driver}

%code{
  #include <iostream>
  #include <cstdlib>
  #include <fstream>
  
  #include "Driver.hh"
  

#undef yylex
#define yylex scanner.yylex
#define tacbuilder driver.get_tacbuilder()
#define tacfactory TACFactory::Instance()
}

%define api.value.type variant
%define parse.assert

%token              END 0 "end of file"
%token              UPPER
%token              LOWER
%token<std::string> WORD
%token              NEWLINE
%token              CHAR
%token INT EQ NE LT LE GT GE IF ELSE WHILE CONTINUE RETURN LA LO LN FLOAT CONST VOID BREAK
%token <int> IntConst
%token <float> floatConst
%token <std::string> IDENTIFIER


%left '!'
%left EQ NE LT LE GT GE
%left '+' '-'
%left '*' '/'

%type <TACListPtr> CompUnit Decls Decl FuncDef ConstDecl VarDecl ConstDef_list ConstDef ConstInitVal ConstExp_list ConstInitVal_list VarDef_list VarDef InitVal InitVal_list Block FuncFParams FuncFParam BlockItem_list BlockItem Stmt LVal PrimaryExp UnaryOp FuncRParams
%type <ExpressionPtr> ConstExp Exp Exp_list Cond AddExp LOrExp Number UnaryExp MulExp RelExp EqExp LAndExp 
/* %type <HaveFunCompiler::parser::SymbolPtr> */

%locations

%%


CompUnit
  : CompUnit Decls
  {
     $$ = TACFactory::Instance()->NewTACList((*$1) + (*$2));
     tacbuilder->SetTACList($$);
  }
  | Decls
  {
    $$ = $1;
    tacbuilder->SetTACList($$);
  };

Decls
  : Decl
  | FuncDef
  ;

Decl
  : ConstDecl
  | VarDecl
  ;

ConstDecl: CONST BType ConstDef_list ';' 
{
  $$ = $3;
  tacbuilder->Pop();
};

ConstDef_list
  : ConstDef
  | ConstDef_list ',' ConstDef
  {
     $$ = TACFactory::Instance()->NewTACList((*$1) + (*$2));
  }
  ;

BType
  : INT
  {
    tacbuilder->Push((int)ValueType::Int);
  }
  | FLOAT
  {
    tacbuilder->Push((int)ValueType::Float);
  }
  ;

ConstDef
  : IDENTIFIER '=' ConstInitVal
  {
    int type;
    tacbuilder->Top(&type);
    switch(type){
      case (int)ValueType::Int:
        auto sym = $3->ret;
        
      case (int)ValueType::Float:

      default:
        break;
    }
  }
  | IDENTIFIER ConstExp_list '=' ConstInitVal
  ;

ConstExp_list
  : '[' ConstExp ']'
  | ConstExp_list '[' ConstExp ']'
  ;

ConstInitVal
  : ConstExp
  | '{' '}'
  | '{' ConstInitVal_list '}'
  ;

ConstInitVal_list
  : ConstInitVal
  | ConstDef_list ',' ConstInitVal
  ;

VarDecl: BType VarDef_list ';' ;

VarDef_list
  : VarDef
  | VarDef_list VarDef
  ;

VarDef
  : IDENTIFIER ConstExp_list
  | IDENTIFIER ConstExp_list '=' InitVal
  ;

InitVal
  : Exp
  | '{' '}'
  | '{' InitVal_list '}'
  ;

InitVal_list
  : InitVal
  | InitVal_list ',' InitVal
  ;

FuncDef 
  : VOID IDENTIFIER '(' FuncFParams ')' Block 
  | BType IDENTIFIER '(' FuncFParams ')' Block 
  ;

FuncFParams
  : FuncFParam
  | FuncFParams ',' FuncFParam
  ;

FuncFParam
  : BType IDENTIFIER
  | BType IDENTIFIER '[' ']' Exp_list
  ;

Exp_list
  : 
  | '[' Exp ']'
  | Exp_list '[' Exp ']'
  ;

Block
  : '{' '}'
  | '{' BlockItem_list '}'
  ;

BlockItem_list
  : BlockItem
  | BlockItem_list BlockItem
  ;

BlockItem
  : Decl
  | Stmt
  ;

Stmt
  : LVal '=' Exp ';'
  | Exp_list ';'
  | Block
  | IF '(' Cond ')' Stmt 
  | IF '(' Cond ')' Stmt ELSE Stmt
  | WHILE '(' Cond ')' Stmt
  | BREAK ';'
  | CONTINUE ';'
  | RETURN ';'
  | RETURN Exp ';'
  ;

Exp : AddExp ;

Cond : LOrExp ;

LVal : IDENTIFIER Exp_list ;

PrimaryExp
  : '(' Exp ')'
  | LVal
  | Number
  ;

Number
  : IntConst
  | floatConst
  ;

UnaryExp
  : PrimaryExp
  | IDENTIFIER '(' ')'
  | IDENTIFIER '(' FuncRParams ')'
  | UnaryOp UnaryExp
  ;

UnaryOp
  : '+'
  | '-'
  | '!'
  ;

FuncRParams
  : Exp
  | FuncRParams ',' Exp
  ;

MulExp
  : UnaryExp
  | MulExp '*' UnaryExp
  | MulExp '/' UnaryExp
  | MulExp '%' UnaryExp
  ;

AddExp
  : MulExp
  | AddExp '+' MulExp
  | AddExp '-' MulExp
  ;

RelExp
  : AddExp
  | RelExp LT AddExp
  | RelExp GT AddExp
  | RelExp LE AddExp
  | RelExp GE AddExp
  ;

EqExp
  : RelExp
  | EqExp EQ RelExp
  | EqExp NE RelExp
  ;

LAndExp
  :EqExp
  |LAndExp LA EqExp
  ;

LOrExp
  : LAndExp
  | LOrExp LO LAndExp
  ;

ConstExp : AddExp ;


%%


void HaveFunCompiler::Parser::Parser::error(const location_type &l,const std::string &err_message){
  std::cerr << "Error: "<<err_message <<" at "<<l<<"\n";
}

