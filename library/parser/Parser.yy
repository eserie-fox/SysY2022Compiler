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
using namespace HaveFunCompiler::ThreeAddressCode;
// using TACListPtr = HaveFunCompiler::ThreeAddressCode::TACListPtr;
// using ExpressionPtr = HaveFunCompiler::ThreeAddressCode::ExpressionPtr;
// using TACFactory = HaveFunCompiler::ThreeAddressCode::TACFactory;
using ValueType =  HaveFunCompiler::ThreeAddressCode::SymbolValue::ValueType;
// using TACOperationType = HaveFunCompiler::ThreeAddressCode::TACOperationType;

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
/* %define parse.trace */

%token              END 0 "end of file"
%token              UPPER
%token              LOWER
%token<std::string> WORD
%token              NEWLINE
%token              CHAR
%token INT EQ NE LT LE LEQ COM LS LB RS RB GT GE IF ELSE WHILE CONTINUE RETURN LA LO LN FLOAT CONST VOID BREAK ADD SUB MUL DIV MOD SEMI
%token <int> IntConst
%token <float> floatConst
%token <std::string> IDENTIFIER


%left LN
%left EQ NE LT LE GT GE
/* %left '+' '-' */
/* %left '*' '/' */
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

/* ConstExp_list ConstInitVal_list Exp_list InitVal_list */

/* %type <TACListPtr> CompUnit Decls Decl FuncDef ConstDecl VarDecl ConstDef_list ConstDef VarDef_list Block BlockItem_list BlockItem Stmt 
%type <ExpressionPtr> ConstExp Exp Cond AddExp LOrExp Number UnaryExp MulExp RelExp EqExp LAndExp ConstInitVal VarDef InitVal PrimaryExp
%type <HaveFunCompiler::parser::SymbolPtr> 
%type <ParamListPtr> FuncFParams 
%type <ArgListPtr> FuncRParams
%type <SymbolPtr> FuncFParam LVal
%type <TACOperationType> UnaryOp  */


%locations

%%


CompUnit
  : END
  | CompUnit Decls END
  | Decls END;

Decls
  : Decl
  | FuncDef
  ;

Decl
  : ConstDecl
  | VarDecl
  ;

ConstDecl: CONST BType ConstDef_list SEMI ;

ConstDef_list
  : ConstDef
  | ConstDef_list COM ConstDef
  ;

BType
  : INT
  | FLOAT
  ;

ConstDef
  : IDENTIFIER LEQ ConstInitVal;

/* ConstExp_list
  : '[' ConstExp ']'
  | ConstExp_list '[' ConstExp ']'
  ; */

ConstInitVal
  : ConstExp
  /* | LB RB
  | LB ConstInitVal_list RB */
  ;

/* ConstInitVal_list
  : ConstInitVal
  {
    $$ = $1->tac;
  }
  | ConstInitVal_list COM ConstInitVal
  {
    $$ = $1
  }
  ; */

VarDecl: BType VarDef_list SEMI
;

VarDef_list
  : VarDef
  | VarDef_list VarDef
  ;

VarDef
  : IDENTIFIER
  | IDENTIFIER LEQ InitVal
  /* | IDENTIFIER ConstExp_list
  | IDENTIFIER ConstExp_list '=' InitVal */
  ;

InitVal
  : Exp
  /* | LB RB
  | LB InitVal_list RB */
  ;

/* InitVal_list
  : InitVal
  {
    $$ = $1->tac;
  }
  | InitVal_list COM InitVal
  {

  }
  ; */

FuncDef 
  : VOID IDENTIFIER LS FuncFParams RS Block 
  | BType IDENTIFIER LS FuncFParams RS Block 
  ;

FuncFParams
  : FuncFParam
  | FuncFParams COM FuncFParam
  ;

FuncFParam
  : BType IDENTIFIER
  /* | BType IDENTIFIER '[' ']' Exp_list
  | BType IDENTIFIER '[' ']' */
  ;

/* Exp_list
  : '[' Exp ']'
  | Exp_list '[' Exp ']'
  ; */

Block
  : LB RB
  | LB BlockItem_list RB
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
  : LVal LEQ Exp SEMI
  | SEMI
  | Exp SEMI
  | Block
  | IF LS Cond RS Stmt %prec LOWER_THAN_ELSE
  | IF LS Cond RS Stmt ELSE Stmt
  | WHILE LS Cond RS Stmt
  | BREAK SEMI
  | CONTINUE SEMI
  | RETURN SEMI
  | RETURN Exp SEMI
  ;

Exp : AddExp ;

Cond : LOrExp ;

/* LVal : IDENTIFIER Exp_list 
{
  $$ = tacbuilder->FindVariableOrConstant($1);
} */
LVal : IDENTIFIER
;

PrimaryExp
  : LS Exp RS
  | LVal
  | Number
  ;

Number
  : IntConst
  | floatConst
  ;

UnaryExp
  : PrimaryExp
  | IDENTIFIER LS RS
  | IDENTIFIER LS FuncRParams RS
  | UnaryOp UnaryExp
  ;

UnaryOp
  : ADD
  | SUB
  | LN
  ;

FuncRParams
  : Exp
  | FuncRParams COM Exp
  ;

MulExp
  : UnaryExp
  | MulExp MUL UnaryExp
  | MulExp DIV UnaryExp
  | MulExp MOD UnaryExp
  ;

AddExp
  : MulExp
  | AddExp ADD MulExp
  | AddExp SUB MulExp
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

