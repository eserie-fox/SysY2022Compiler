%skeleton "lalr1.cc"
%require "3.0"
%debug
%defines
%define api.namespace {HaveFunCompiler::Parser}
%define api.parser.class {TACParser}
%define parse.error detailed

%code requires {#include <../src_parser/location.hh> }
%define api.location.type {HaveFunCompiler::Parser::location}

%code requires{
  namespace HaveFunCompiler{
    namespace Parser{
      class TACDriver;
      class TACScanner;
    }
  }

#include "TAC/TAC.hh"
using namespace HaveFunCompiler::ThreeAddressCode;
using namespace HaveFunCompiler;
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

%parse-param {TACScanner &scanner}
%parse-param {TACDriver &driver}

%code{
  #include <iostream>
  #include <cstdlib>
  #include <fstream>
  #include "Exceptions.hh"
  #include "MagicEnum.hh"
  
  #include "TACDriver.hh"
  

#undef yylex
#define yylex scanner.yylex
#define tacbuilder driver.get_tacbuilder()
#define tacfactory TACFactory::Instance()

const int FUNC_BLOCK_IN_FLAG = 2022;
const int FUNC_BLOCK_OUT_FLAG = 2023;
const int NONFUNC_BLOCK_FLAG = 2024;

}

%define api.value.type variant
%define parse.assert
/* %define parse.trace */

%token              END 0 "end of file"
%token INT EQ NE LT LE LEQ LS  RS AND LM CALL RM GT GE IFZ RET LN FLOAT CONST ADD SUB MUL DIV MOD LABEL GOTO STRING FBEGIN FEND ARG PARAM SEM
%token <int> IntConst
%token <float> floatConst
%token <std::string> IDENTIFIER


%left LN
%left EQ NE LT LE GT GE
/* %left '+' '-' */
/* %left '*' '/' */
%nonassoc LOWER_THAN_LM
%nonassoc LM

/* ConstExp_list ConstInitVal_list Exp_list InitVal_list */

%type <TACListPtr> Start CompUnit CompUnit_list 
%type <SymbolPtr> ARITHIDENT
/* %type <ExpressionPtr> ConstExp LVal Exp Cond AddExp LOrExp Number UnaryExp MulExp RelExp EqExp LAndExp ConstInitVal VarDef InitVal PrimaryExp */
/* %type <HaveFunCompiler::parser::SymbolPtr>  */
/* %type <ParamListPtr> FuncFParams 
%type <ArgListPtr> FuncRParams
%type <SymbolPtr> FuncFParam
%type <TACOperationType> UnaryOp  
%type <std::string> FuncIdenti
%type FuncHead
%type <ArrayDescriptorPtr> ConstExp_list ConstInitVal_list InitVal_list
%type <std::vector<ExpressionPtr>> Exp_list */


%locations

%%

Start:
  END
  {
    $$ = tacbuilder->NewTACList();
    tacbuilder->SetTACList($$);
  }
  | CompUnit_list END
  {
    $$=$1;
    tacbuilder->SetTACList($$);
  };

CompUnit_list
  : CompUnit
  | CompUnit_list CompUnit
  {
    $$ = tacbuilder->NewTACList((*$1) + (*$2));
  }
  ;

CompUnit
  : CONST INT LM IntConst RM IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateArray(SymbolValue::ValueType::Int, $4, true, $6);
    tacbuilder->InsertSymbol($6, sym);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Constant, sym));
  }
  | CONST FLOAT LM IntConst RM IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateArray(SymbolValue::ValueType::Float, $4, true, $6);
    tacbuilder->InsertSymbol($6, sym);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Constant, sym));
  }
  | INT IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateVariable($2, SymbolValue::ValueType::Int);
    tacbuilder->InsertSymbol($2, sym);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Variable,sym));
  }
  | FLOAT IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateVariable($2, SymbolValue::ValueType::Float);
    tacbuilder->InsertSymbol($2, sym);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Variable,sym));
  }
  | INT LM IntConst RM IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateArray(SymbolValue::ValueType::Int, $3, false, $5);
    tacbuilder->InsertSymbol($5, sym);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Variable, sym));
  }
  | FLOAT LM IntConst RM IDENTIFIER 
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateArray(SymbolValue::ValueType::Float, $3, false, $5);
    tacbuilder->InsertSymbol($5, sym);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Variable, sym));
  }
  | FBEGIN
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::FunctionBegin));
  }
  | FEND
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::FunctionEnd));
  }
  | LABEL IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->FindSymbol($2);
    if(sym==nullptr){
      sym = tacbuilder->NewSymbol(SymbolType::Label, $2);
      tacbuilder->InsertSymbol($2, sym);
    }
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Label, sym));
  }
  | ARG ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Argument, $2));
  }
  | ARG AND IDENTIFIER LM IntConst RM
  {
    SymbolPtr sym;
    sym = tacbuilder->FindSymbol($3);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::ArgumentAddress, 
                                tacbuilder->AccessArray(sym, tacbuilder->CreateConstSym($5))));
  }
  | ARG AND IDENTIFIER LM IDENTIFIER RM
  {
    SymbolPtr sym;
    sym = tacbuilder->FindSymbol($3);
    SymbolPtr sym2;
    sym2 = tacbuilder->FindSymbol($5);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::ArgumentAddress, 
                                tacbuilder->AccessArray(sym, sym2)));
  }
  | PARAM INT IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateVariable($3, SymbolValue::ValueType::Int);
    tacbuilder->InsertSymbol($3, sym);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Parameter, sym));
  }
  | PARAM INT LM RM IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateArray(SymbolValue::ValueType::Int, 0, false, $5);
    tacbuilder->InsertSymbol($5, sym);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Parameter, sym));
  }
  | PARAM FLOAT IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateVariable($3, SymbolValue::ValueType::Float);
    tacbuilder->InsertSymbol($3, sym);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Parameter, sym));
  }
  | PARAM FLOAT LM RM IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateArray(SymbolValue::ValueType::Float, 0, false, $5);
    tacbuilder->InsertSymbol($5, sym);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Parameter, sym));
  }
  | ARITHIDENT LEQ ARITHIDENT ADD ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Add,$1, $3, $5));
  }
  | ARITHIDENT LEQ ARITHIDENT SUB ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Sub,$1, $3, $5));
  }
  | ARITHIDENT LEQ ARITHIDENT MUL ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Mul,$1, $3, $5));
  }
  | ARITHIDENT LEQ ARITHIDENT DIV ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Div,$1, $3, $5));
  }
  | ARITHIDENT LEQ ARITHIDENT MOD ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Mod,$1, $3, $5));
  }
  | ARITHIDENT LEQ ARITHIDENT GT ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::GreaterThan,$1, $3, $5));
  }
  | ARITHIDENT LEQ ARITHIDENT LT ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::LessThan,$1, $3, $5));
  }
  | ARITHIDENT LEQ ARITHIDENT GE ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::GreaterOrEqual,$1, $3, $5));
  }
  | ARITHIDENT LEQ ARITHIDENT LE ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::LessOrEqual,$1, $3, $5));
  }
  | ARITHIDENT LEQ ARITHIDENT EQ ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Equal,$1, $3, $5));
  }
  | ARITHIDENT LEQ ARITHIDENT NE ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::NotEqual,$1, $3, $5));
  }
  | ARITHIDENT LEQ ADD ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::UnaryPositive,$1, $4));
  }
  | ARITHIDENT LEQ SUB ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::UnaryMinus,$1, $4));
  }
  | ARITHIDENT LEQ LN ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::UnaryNot,$1, $4));
  }
  | ARITHIDENT LEQ LS FLOAT RS ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::IntToFloat,$1, $6));
  }
  | ARITHIDENT LEQ LS INT RS ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::FloatToInt,$1, $6));
  }
  | ARITHIDENT LEQ ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Assign,$1, $3));
  }
  | CALL IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->FindSymbol($2);
    if(sym==nullptr){
      sym = tacbuilder->NewSymbol(SymbolType::Label, $2);
      tacbuilder->InsertSymbol($2, sym);
    }
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Call, nullptr, sym));
  }
  | ARITHIDENT LEQ CALL IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->FindSymbol($4);
    if(sym==nullptr){
      sym = tacbuilder->NewSymbol(SymbolType::Label, $4);
      tacbuilder->InsertSymbol($4, sym);
    }
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Call, $1, sym));
  }
  | RET SEM
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Return));
  }
  | RET ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Return, $2));
  }
  | GOTO IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->FindSymbol($2);
    if(sym==nullptr){
      sym = tacbuilder->NewSymbol(SymbolType::Label, $2);
      tacbuilder->InsertSymbol($2, sym);
    }
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Goto, sym));
  }
  | IFZ ARITHIDENT GOTO IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->FindSymbol($4);
    if(sym==nullptr){
      sym = tacbuilder->NewSymbol(SymbolType::Label, $4);
      tacbuilder->InsertSymbol($4, sym);
    }
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::IfZero, sym,$2));
  }
  ;

ARITHIDENT
  : IDENTIFIER LM ARITHIDENT RM
  {
    SymbolPtr sym;
    sym = tacbuilder->FindSymbol($1);
    $$ = tacbuilder->AccessArray(sym, $3);
  }
  | IDENTIFIER %prec LOWER_THAN_LM
  {
    $$ = tacbuilder->FindSymbol($1);
  }
  | IntConst
  {
    $$ = tacbuilder->CreateConstSym($1);
  }
  | floatConst
  {
    $$ = tacbuilder->CreateConstSym($1);
  }

%%


void HaveFunCompiler::Parser::TACParser::error(const location_type &l,const std::string &err_message){
  std::cerr << "Error: "<<err_message <<" at "<<l<<"\n";
}

