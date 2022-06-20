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
%token INT EQ NE LT LE LEQ LS  RS AND LM CALL RM GT GE IFZ RET LN FLOAT CONST ADD SUB MUL DIV MOD LABEL GOTO STRING FBEGIN FEND ARG PARAM
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
  : CONST INT LM INT RM IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateArray(SymbolValue::ValueType::Int, $4, true, $6);
    tacbuilder->InsertSymbol($6, sym);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Constant, sym));
  }
  | CONST INT LM FLOAT RM IDENTIFIER
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
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Variable,sym));
  }
  | FLOAT IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateVariable($2, SymbolValue::ValueType::Float);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Variable,sym));
  }
  | INT LM INT RM IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateArray(SymbolValue::ValueType::Int, $4, false, $6);
    tacbuilder->InsertSymbol($6, sym);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Variable, sym));
  }
  | FLOAT LM INT RM IDENTIFIER 
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateArray(SymbolValue::ValueType::Float, $4, false, $6);
    tacbuilder->InsertSymbol($6, sym);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Variable, sym));
  }
  | STRING ARITHIDENT
  | FBEGIN
  | FEND
  | LABEL IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->NewSymbol(SymbolType::Label, $2);
    tacbuilder->InsertSymbol($2, sym);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Label, sym));
  }
  | ARG ARITHIDENT
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Argument, $2));
  }
  | ARG IDENTIFIER LM IntConst RM
  {
    SymbolPtr sym;
    sym = tacbuilder->FindSymbol($2);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Argument, 
                                tacbuilder->AccessArray(sym, tacbuilder->CreateConstSym($4)));
  }
  | ARG AND IDENTIFIER LM IntConst RM
  {
    SymbolPtr sym;
    sym = tacbuilder->FindSymbol($2);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::ArgumentAddress, 
                                tacbuilder->AccessArray(sym, tacbuilder->CreateConstSym($4)));
  }
  | PARAM INT IDENTIFIER
  {
    SymbolPtr sym;
    sym = tacbuilder->CreateVariable($3, SymbolValue::ValueType::Int);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Parameter, sym));
  }
  | PARAM INT LM RM IDENTIFIER
  {

  }
  | PARAM FLOAT ARITHIDENT
  | PARAM FLOAT LM RM IDENTIFIER
  | ARITHIDENT LEQ ARITHIDENT ADD ARITHIDENT
  | ARITHIDENT LEQ ARITHIDENT SUB ARITHIDENT
  | ARITHIDENT LEQ ARITHIDENT MUL ARITHIDENT
  | ARITHIDENT LEQ ARITHIDENT DIV ARITHIDENT
  | ARITHIDENT LEQ ARITHIDENT MOD ARITHIDENT
  | ARITHIDENT LEQ ARITHIDENT GT ARITHIDENT
  | ARITHIDENT LEQ ARITHIDENT LT ARITHIDENT
  | ARITHIDENT LEQ ARITHIDENT GE ARITHIDENT
  | ARITHIDENT LEQ ARITHIDENT GT ARITHIDENT
  | ARITHIDENT LEQ ARITHIDENT LE ARITHIDENT
  | ARITHIDENT LEQ ARITHIDENT LEQ ARITHIDENT
  | ARITHIDENT LEQ ARITHIDENT NE ARITHIDENT
  | ARITHIDENT LEQ ADD ARITHIDENT
  | ARITHIDENT LEQ SUB ARITHIDENT
  | ARITHIDENT LEQ LN ARITHIDENT
  | ARITHIDENT LEQ LS FLOAT RS ARITHIDENT
  | ARITHIDENT LEQ LS INT RS ARITHIDENT
  | ARITHIDENT LEQ ARITHIDENT
  | ARITHIDENT LM INT RM LEQ ARITHIDENT
  | ARITHIDENT LEQ ARITHIDENT LM INT RM
  | CALL IDENTIFIER
  | ARITHIDENT LEQ CALL IDENTIFIER
  | RET 
  | RET ARITHIDENT
  | GOTO IDENTIFIER
  | IFZ ARITHIDENT GOTO IDENTIFIER
  ;

ARITHIDENT
  : IDENTIFIER LM ARITHIDENT RM
  {

  }
  | IDENTIFIER
  {
    $$ = tacbuilder->
  }
  | IntConst
  {

  }
  | floatConst
  {
    
  }

%%


void HaveFunCompiler::Parser::TACParser::error(const location_type &l,const std::string &err_message){
  std::cerr << "Error: "<<err_message <<" at "<<l<<"\n";
}

