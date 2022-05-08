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
%define parse.trace

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

%type <TACListPtr> CompUnit Decls Decl FuncDef ConstDecl VarDecl ConstDef_list ConstDef ConstExp_list ConstInitVal_list VarDef_list InitVal_list Block BlockItem_list BlockItem Stmt 
%type <ExpressionPtr> ConstExp Exp Exp_list Cond AddExp LOrExp Number UnaryExp MulExp RelExp EqExp LAndExp ConstInitVal VarDef InitVal PrimaryExp
/* %type <HaveFunCompiler::parser::SymbolPtr> */
%type <ParamListPtr> FuncFParams 
%type <ArgListPtr> FuncRParams
%type <SymbolPtr> FuncFParam LVal
%type <TACOperationType> UnaryOp


%locations

%%


CompUnit
  : CompUnit Decls
  {
     $$ = tacbuilder->NewTACList((*$1) + (*$2));
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
     $$ = tacbuilder->NewTACList((*$1) + (*$3));
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
      {
        SymbolPtr sym = $3->ret;
        sym->type_ =  HaveFunCompiler::ThreeAddressCode::SymbolType::Constant;
        sym->value_ = SymbolValue((int)$3->ret->value_);
        tacbuilder->BindConstName($1,sym);
        break;
      }
      case (int)ValueType::Float:
      {
        SymbolPtr sym = $3->ret;
        sym->type_ =  HaveFunCompiler::ThreeAddressCode::SymbolType::Constant;
        sym->value_ = SymbolValue((float)$3->ret->value_);
        tacbuilder->BindConstName($1,sym);
        break;
      }
      default:
        throw std::runtime_error("Not const type : " + std::to_string((int)type));
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
  {
    $$ = $1;
  }
  | '{' '}'
  | '{' ConstInitVal_list '}'
  ;

ConstInitVal_list
  : ConstInitVal
  | ConstInitVal_list ',' ConstInitVal
  ;

VarDecl: BType VarDef_list ';' 
{
  $$ = tacbuilder->NewTACList((*$2));
}
;

VarDef_list
  : VarDef
  | VarDef_list VarDef
  {
    $$ = tacbuilder->NewTACList((*$1) + (*$2->tac));
  }
  ;

VarDef
  : IDENTIFIER
  {
    int type;
    SymbolPtr var;
    tacbuilder->Top(&type);
    var = tacbuilder->CreateVariable($1, (ValueType)type);
    $$ = tacbuilder->NewExp(nullptr, var);
  }
  | IDENTIFIER '=' InitVal
  {
    int type;
    SymbolPtr var;
    tacbuilder->Top(&type);
    var = tacbuilder->CreateVariable($1, (ValueType)type);
    $$ = tacbuilder->CreateAssign(var, $3);
  }
  | IDENTIFIER ConstExp_list
  | IDENTIFIER ConstExp_list '=' InitVal
  ;

InitVal
  : Exp
  {
    $$ = $1;
  }
  | '{' '}'
  | '{' InitVal_list '}'
  ;

InitVal_list
  : InitVal
  | InitVal_list ',' InitVal
  ;

FuncDef 
  : VOID IDENTIFIER '(' FuncFParams ')' Block 
  {
    SymbolPtr sym = tacbuilder->CreateTempVariable(ValueType::Void); 
    $4->push_back_parameter(sym);
    $$ = tacbuilder->CreateFunction($2, $4, $6);
  }
  | BType IDENTIFIER '(' FuncFParams ')' Block 
  {
    int type;
    tacbuilder->Top(&type);
    SymbolPtr sym = tacbuilder->CreateTempVariable((ValueType)type); 
    tacbuilder->Pop();
    $4->push_back_parameter(sym);
    $$ = tacbuilder->CreateFunction($2, $4, $6);
  }
  ;

FuncFParams
  : FuncFParam
  {
    ParamListPtr ls = tacbuilder->NewParamList();
    ls->push_back_parameter($1);
    $$ = ls;
  }
  | FuncFParams ',' FuncFParam
  {
    $1->push_back_parameter($3);
    $$ = $1;
  }
  ;

FuncFParam
  : BType IDENTIFIER
  {
    int type;
    SymbolPtr var;
    tacbuilder->Top(&type);
    var = tacbuilder->CreateVariable($2, (ValueType)type);
    tacbuilder->Pop();
    $$ = var;
  }
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
  {
    $$ = tacbuilder->NewTACList((*$1) + (*$2));
  }
  ;

BlockItem
  : Decl
  | Stmt
  ;

Stmt
  : LVal '=' Exp ';'
  {
    // SymbolPtr var = tacbuilder->FindVariableOrConstant($1);
    // $$ = tacbuilder->CreateAssign(var, $3)->tac;
  }
  | Exp_list ';'
  {
    $$ = $1->tac;
  }
  | Block
  | IF '(' Cond ')' Stmt 
  {
    $$ = tacbuilder->CreateIf($3, $5, nullptr);
  }
  | IF '(' Cond ')' Stmt ELSE Stmt
  {
    $$ = tacbuilder->CreateIfElse($3,$5,$7,nullptr, nullptr);
  }
  | WHILE '(' Cond ')' Stmt
  {
    SymbolPtr label_con;
    SymbolPtr label_brk;
    $$ = tacbuilder->CreateWhile($3, $5, &label_con, &label_brk);
  }
  | BREAK ';'
  {

  }
  | CONTINUE ';'
  {

  }
  | RETURN ';'
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Return));
  }
  | RETURN Exp ';'
  {
    $$ = tacbuilder->NewTACList(*$2->tac + tacbuilder->NewTAC(TACOperationType::Return, $2->ret));
  }
  ;

Exp : AddExp ;

Cond : LOrExp ;

LVal : IDENTIFIER Exp_list 
{
  $$ = tacbuilder->FindVariableOrConstant($1);
}
;

PrimaryExp
  : '(' Exp ')'
  {
    $$ = $2;
  }
  | LVal
  {
    $$ = tacbuilder->NewExp(nullptr, $1);
  }
  | Number
  ;

Number
  : IntConst
  {
    $$ = tacbuilder->CreateConstExp($1);
  }
  | floatConst
  {
    $$ = tacbuilder->CreateConstExp($1);
  }
  ;

UnaryExp
  : PrimaryExp
  | IDENTIFIER '(' ')'
  {
    ParamListPtr params = tacbuilder->FindFunctionLabel($1)->value_.GetParameters();
    auto sym = (params->end());
    sym--;
    auto it = *sym;
    SymbolPtr ret_sym = tacbuilder->CreateTempVariable(it->value_.Type());
    TACListPtr tac = tacbuilder->CreateCallWithRet($1, nullptr, ret_sym);
    $$ = tacbuilder->NewExp(tac, ret_sym);
  }
  | IDENTIFIER '(' FuncRParams ')'
  {
    ParamListPtr params = tacbuilder->FindFunctionLabel($1)->value_.GetParameters();
    auto sym = (params->end());
    sym--;
    auto it = *sym;
    SymbolPtr ret_sym = tacbuilder->CreateTempVariable(it->value_.Type());
    TACListPtr tac = tacbuilder->CreateCallWithRet($1, $3, ret_sym);
    $$ = tacbuilder->NewExp(tac, ret_sym);
  }
  | UnaryOp UnaryExp
  {
    $$ = tacbuilder->CreateArithmeticOperation($1, $2);
  }
  ;

UnaryOp
  : '+'
  {
    $$ = TACOperationType:: UnaryPositive;
  }
  | '-'
  {
    $$ = TACOperationType:: UnaryMinus;
  }
  | '!'
  {
    $$ = TACOperationType:: UnaryNot;
  }
  ;

FuncRParams
  : Exp
  {
    ArgListPtr args = tacbuilder->NewArgList();
    args->push_back_argument($1);
    $$ = args;
  }
  | FuncRParams ',' Exp
  {
    $1->push_back_argument($3);
    $$ = $1;
  }
  ;

MulExp
  : UnaryExp
  | MulExp '*' UnaryExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::Mul, $1, $3);
  }
  | MulExp '/' UnaryExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::Div, $1, $3);
  }
  | MulExp '%' UnaryExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::Mod, $1, $3);
  }
  ;

AddExp
  : MulExp
  | AddExp '+' MulExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::Add, $1, $3);
  }
  | AddExp '-' MulExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::Sub, $1, $3);
  }
  ;

RelExp
  : AddExp
  | RelExp LT AddExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::LessThan, $1, $3);
  }
  | RelExp GT AddExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::GreaterThan, $1, $3);
  }
  | RelExp LE AddExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::LessOrEqual, $1, $3);
  }
  | RelExp GE AddExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::GreaterOrEqual, $1, $3);
  }
  ;

EqExp
  : RelExp
  | EqExp EQ RelExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::Equal, $1, $3);
  }
  | EqExp NE RelExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::NotEqual, $1, $3);
  }
  ;

LAndExp
  :EqExp
  |LAndExp LA EqExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::LogicAnd, $1, $3);
  }
  ;

LOrExp
  : LAndExp
  | LOrExp LO LAndExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::LogicOr, $1, $3);
  }
  ;

ConstExp : AddExp ;


%%


void HaveFunCompiler::Parser::Parser::error(const location_type &l,const std::string &err_message){
  std::cerr << "Error: "<<err_message <<" at "<<l<<"\n";
}

