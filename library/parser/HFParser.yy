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

%parse-param {Scanner &scanner}
%parse-param {Driver &driver}

%code{
  #include <iostream>
  #include <cstdlib>
  #include <fstream>
  #include "Exceptions.hh"
  #include "MagicEnum.hh"
  
  #include "Driver.hh"
  

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

%type <TACListPtr> Start CompUnit Decls Decl FuncDef WHILEUP ConstDecl VarDecl ConstDef_list ConstDef VarDef_list Block BlockItem_list BlockItem Stmt LBUP RBUP
%type <ExpressionPtr> ConstExp Exp Cond AddExp LOrExp Number UnaryExp MulExp RelExp EqExp LAndExp ConstInitVal VarDef InitVal PrimaryExp
/* %type <HaveFunCompiler::parser::SymbolPtr>  */
%type <ParamListPtr> FuncFParams 
%type <ArgListPtr> FuncRParams
%type <SymbolPtr> FuncFParam LVal
%type <TACOperationType> UnaryOp  
%type <std::string> FuncIdenti
%type FuncHead


%locations

%%

Start:
  END
  {
    $$ = tacbuilder->NewTACList();
    tacbuilder->SetTACList($$);
  }
  | CompUnit END
  {
    $$=$1;
    tacbuilder->SetTACList($$);
  };

CompUnit
  : CompUnit Decls
  {
     $$ = tacbuilder->NewTACList((*$1) + (*$2));
  }
  | Decls 
  {
    $$ = $1;
  };

Decls
  : Decl
  {
    $$=$1;
  }
  | FuncDef
  ;

Decl
  : ConstDecl
  | VarDecl
  {
    $$=$1;
  }
  ;

ConstDecl: CONST BType ConstDef_list SEMI 
{
  $$ = $3;
  tacbuilder->Pop();
};

ConstDef_list
  : ConstDef
  | ConstDef_list COM ConstDef
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
  : IDENTIFIER LEQ ConstInitVal
  {
    int type;
    tacbuilder->Top(&type);
    switch(type){
      case (int)ValueType::Int:
      {
        SymbolPtr sym = $3->ret;
        sym->type_ =  HaveFunCompiler::ThreeAddressCode::SymbolType::Constant;
        int value;
        if($3->ret->value_.Type()==ValueType::Int)
        {
          value = $3->ret->value_.GetInt();
        }else{
          value = static_cast<int>($3->ret->value_.GetFloat());
        }
        sym->value_ = SymbolValue(value);
        tacbuilder->BindConstName($1,sym);
        break;
      }
      case (int)ValueType::Float:
      {
        SymbolPtr sym = $3->ret;
        sym->type_ =  HaveFunCompiler::ThreeAddressCode::SymbolType::Constant;
        float value;
        if($3->ret->value_.Type()==ValueType::Int)
        {
          value = static_cast<float>($3->ret->value_.GetInt());
        }else{
          value = $3->ret->value_.GetFloat();
        }
        sym->value_ = SymbolValue(value);
        tacbuilder->BindConstName($1,sym);
        break;
      }
      default:
        throw TypeMismatchException(std::string(magic_enum::enum_name((ValueType)type)),"Const");
        break;
    }
    
    $$ = tacbuilder->NewTACList();
  };

/* ConstExp_list
  : '[' ConstExp ']'
  | ConstExp_list '[' ConstExp ']'
  ; */

ConstInitVal
  : ConstExp
  {
    $$ = $1;
  }
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
{
  $$ = $2;
}
;

VarDef_list
  : VarDef
  {
    $$ = $1->tac;
  }
  | VarDef_list COM VarDef
  {
    $$ = tacbuilder->NewTACList((*$1) + (*$3->tac));
  }
  ;

VarDef
  : IDENTIFIER
  {
    int type;
    SymbolPtr var;
    tacbuilder->Top(&type);
    var = tacbuilder->CreateVariable($1, (ValueType)type);
    auto tac = tacbuilder->NewTAC(TACOperationType::Variable,var);
    $$ = tacbuilder->NewExp(tacbuilder->NewTACList(tac), var);
  }
  | IDENTIFIER LEQ InitVal
  {
    int type;
    SymbolPtr var;
    ExpressionPtr exp;
    tacbuilder->Top(&type);
    var = tacbuilder->CreateVariable($1, (ValueType)type);
    if($3->ret->value_.Type()!=(ValueType)type){
      if((ValueType)type == ValueType::Int){
        exp = tacbuilder->CastFloatToInt($3);
      }else{
        exp = tacbuilder->CastIntToFloat($3);
      }
      (*exp->tac) += tacbuilder->NewTAC(TACOperationType::Variable,var);
      $$ = tacbuilder->CreateAssign(var, exp);
    }else{
      (*$3->tac) += tacbuilder->NewTAC(TACOperationType::Variable,var);
      $$ = tacbuilder->CreateAssign(var, $3);
    }
  }
  /* | IDENTIFIER ConstExp_list
  | IDENTIFIER ConstExp_list '=' InitVal */
  ;

InitVal
  : Exp
  {
    $$ = $1;
  }
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
FuncIdenti
  :IDENTIFIER
  {
    auto func_label =  tacbuilder->CreateFunctionLabel($1);
    tacbuilder->PushFunc(func_label);
    tacbuilder->EnterSubscope();
    $$ = $1;
  }
  ;
FuncHead
  : VOID FuncIdenti LS FuncFParams RS
  {
    SymbolPtr func_label;
    tacbuilder->TopFunc(&func_label);
    tacbuilder->CreateFunctionHead(ValueType::Void,func_label,$4);
    tacbuilder->PushFunc(FUNC_BLOCK_IN_FLAG);
  }
  | BType FuncIdenti LS FuncFParams RS
  {
    int type;
    tacbuilder->Top(&type);
    tacbuilder->Pop();
    SymbolPtr func_label;
    tacbuilder->TopFunc(&func_label);
    tacbuilder->CreateFunctionHead((ValueType)type,func_label,$4);
    tacbuilder->PushFunc(FUNC_BLOCK_IN_FLAG);
  };

FuncDef 
  : FuncHead Block 
  {
    tacbuilder->PopFunc();
    SymbolPtr func_head;
    tacbuilder->TopFunc(&func_head);
    tacbuilder->PopFunc();
    $$ = tacbuilder->CreateFunction(func_head, $2);
    tacbuilder->ExitSubscope();
  }
  ;

FuncFParams
  : {
    $$ = tacbuilder->NewParamList();
  }
  | FuncFParam
  {
    ParamListPtr ls = tacbuilder->NewParamList();
    ls->push_back_parameter($1);
    $$ = ls;
  }
  | FuncFParams COM FuncFParam
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
  /* | BType IDENTIFIER '[' ']' Exp_list
  | BType IDENTIFIER '[' ']' */
  ;

/* Exp_list
  : '[' Exp ']'
  | Exp_list '[' Exp ']'
  ; */

Block
  : LBUP RBUP
  {
    $$ = tacbuilder->NewTACList();
  }
  | LBUP BlockItem_list RBUP
  {
    $$ = $2;
  }
  ;

LBUP: LB
{
  bool flag = true;
  int value;
  if(tacbuilder->TopFunc(&value)){
    if(value == FUNC_BLOCK_IN_FLAG){
      flag = false;
      tacbuilder->PopFunc();
      tacbuilder->PushFunc(FUNC_BLOCK_OUT_FLAG);
    }
  }
  if(flag)
  {
    tacbuilder->PushFunc(NONFUNC_BLOCK_FLAG);
    tacbuilder->EnterSubscope();
  }
  $$ = tacbuilder->NewTACList();
}
;

RBUP:RB
{
  bool flag = true;
  int value;
  if(tacbuilder->TopFunc(&value)){
    if(value==FUNC_BLOCK_OUT_FLAG){
      flag = false;
    }
  }
  if(flag)
  {
    assert(value == NONFUNC_BLOCK_FLAG);
    tacbuilder->PopFunc();
    tacbuilder->ExitSubscope();
  }
  $$ = tacbuilder->NewTACList();
}
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
  : LVal LEQ Exp SEMI
  {
    if($1->type_ == SymbolType::Constant)
    {
      throw RuntimeException("Cant assign to constant");
    }
    ExpressionPtr exp;
    if($3->ret->value_.Type()!=$1->value_.Type()){
      if($1->value_.Type() == ValueType::Int){
        exp = tacbuilder->CastFloatToInt($3);
      }else{
        exp = tacbuilder->CastIntToFloat($3);
      }
      $$ = tacbuilder->CreateAssign($1, exp)->tac;
    }else{
      $$ = tacbuilder->CreateAssign($1, $3)->tac;
    }
    
  }
  | SEMI
  {
    $$ = tacbuilder->NewTACList();
  }
  | Exp SEMI
  {
    $$ = $1->tac;
  }
  | Block
  | IF LS Cond RS Stmt %prec LOWER_THAN_ELSE
  {
    $$ = tacbuilder->CreateIf($3, $5, nullptr);
  }
  | IF LS Cond RS Stmt ELSE Stmt
  {
    $$ = tacbuilder->CreateIfElse($3,$5,$7,nullptr, nullptr);
  }
  | WHILEUP LS Cond RS Stmt
  {
    SymbolPtr label_con;
    SymbolPtr label_brk;
    tacbuilder->TopLoop(&label_con, &label_brk);
    $$ = tacbuilder->CreateWhile($3, $5, label_con, label_brk);
    tacbuilder->PopLoop();
    
  }
  | BREAK SEMI
  {
    SymbolPtr label_brk;
    tacbuilder->TopLoop(nullptr, &label_brk);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Goto,label_brk));
    // tacbuilderTopLoop
  }
  | CONTINUE SEMI
  {
    SymbolPtr label_con;
    tacbuilder->TopLoop(&label_con, nullptr);
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Goto,label_con));
  }
  | RETURN SEMI
  {
    $$ = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Return));
  }
  | RETURN Exp SEMI
  {
    $$ = tacbuilder->NewTACList(*$2->tac + tacbuilder->NewTAC(TACOperationType::Return, $2->ret));
  }
  ;

WHILEUP : WHILE
{
  SymbolPtr label_con = tacbuilder->CreateTempLabel();
  SymbolPtr label_brk = tacbuilder->CreateTempLabel();
  tacbuilder->PushLoop(label_con, label_brk);
  $$ = tacbuilder->NewTACList();
}
;

Exp : AddExp ;

Cond : LOrExp ;

/* LVal : IDENTIFIER Exp_list 
{
  $$ = tacbuilder->FindVariableOrConstant($1);
} */
LVal : IDENTIFIER
{
  $$ = tacbuilder->FindVariableOrConstant($1);
}
;

PrimaryExp
  : LS Exp RS
  {
    $$ = $2;
  }
  | LVal
  {
    $$ = tacbuilder->NewExp(tacbuilder->NewTACList(), $1);
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
  | IDENTIFIER LS RS
  {
    ParamListPtr params = tacbuilder->FindFunctionLabel($1)->value_.GetParameters();
    SymbolPtr ret_sym = tacbuilder->CreateTempVariable(params->get_return_type());
    TACListPtr tac = tacbuilder->CreateCallWithRet($1, tacbuilder->NewArgList(), ret_sym);
    $$ = tacbuilder->NewExp(tac, ret_sym);
  }
  | IDENTIFIER LS FuncRParams RS
  {
    ParamListPtr params = tacbuilder->FindFunctionLabel($1)->value_.GetParameters();
    size_t nfuncparam = params->size();
    size_t narg = $3->size();
    TACListPtr tac;
    SymbolPtr ret_sym;
    if(nfuncparam==narg)
    {
      auto paramsPtr = params->begin();
      auto argPtr = $3->begin();
      ExpressionPtr exp;
      ArgListPtr ansArg = tacbuilder->NewArgList();
      while(argPtr != $3->end()){
        if((*paramsPtr)->value_.Type()!=(*argPtr)->ret->value_.Type()){
          if((*paramsPtr)->value_.Type() == ValueType::Int){
            exp = tacbuilder->CastFloatToInt((*argPtr));
          }
          else{
            exp = tacbuilder->CastIntToFloat((*argPtr));
          }
          ansArg->push_back_argument(exp);
        }else{
          ansArg->push_back_argument((*argPtr));
        }
        argPtr++;
        paramsPtr++;
      }
      ret_sym = tacbuilder->CreateTempVariable(params->get_return_type());
      tac = tacbuilder->CreateCallWithRet($1, ansArg, ret_sym);
    }else{
      ret_sym = tacbuilder->CreateTempVariable(params->get_return_type());
      tac = tacbuilder->CreateCallWithRet($1, $3, ret_sym);
    }
    $$ = tacbuilder->NewExp(tac, ret_sym);
  }
  | UnaryOp UnaryExp
  {
    $$ = tacbuilder->CreateArithmeticOperation($1, $2);
  }
  ;

UnaryOp
  : ADD
  {
    $$ = TACOperationType:: UnaryPositive;
  }
  | SUB
  {
    $$ = TACOperationType:: UnaryMinus;
  }
  | LN
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
  | FuncRParams COM Exp
  {
    $1->push_back_argument($3);
    $$ = $1;
  }
  ;

MulExp
  : UnaryExp
  | MulExp MUL UnaryExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::Mul, $1, $3);
  }
  | MulExp DIV UnaryExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::Div, $1, $3);
  }
  | MulExp MOD UnaryExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::Mod, $1, $3);
  }
  ;

AddExp
  : MulExp
  | AddExp ADD MulExp
  {
    $$ = tacbuilder->CreateArithmeticOperation(TACOperationType::Add, $1, $3);
  }
  | AddExp SUB MulExp
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
    if($1->ret->type_==SymbolType::Constant){
      if(!(bool)$1->ret->value_){
        $$=tacbuilder->CreateConstExp(0);
      }
    }
    else
    {
      ExpressionPtr exp = tacbuilder->CreateArithmeticOperation(TACOperationType::UnaryNot, $1);
      SymbolPtr ret = tacbuilder->CreateTempVariable(ValueType::Int);
      TACListPtr tac1 = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Variable, ret));
      TACListPtr tac2 = tacbuilder->CreateAssign(ret, tacbuilder->CreateConstExp(0))->tac;

      ExpressionPtr exp1 = tacbuilder->CreateArithmeticOperation(TACOperationType::UnaryNot, $3);
      TACListPtr tac3 = tacbuilder->CreateAssign(ret, tacbuilder->CreateConstExp(1))->tac;
      TACListPtr tac4 = tacbuilder->CreateAssign(ret, tacbuilder->CreateConstExp(0))->tac;
      TACListPtr tac5 = tacbuilder->CreateIfElse(exp1,tac4,tac3);
      ExpressionPtr exp2 = tacbuilder->NewExp(tacbuilder->CreateIfElse(exp,tac2,tac5), ret);
      $$ = exp2;
    }
  }
  ;

LOrExp
  : LAndExp
  | LOrExp LO LAndExp
  {
    if($1->ret->type_==SymbolType::Constant){
      if((bool)$1->ret->value_){
        $$=tacbuilder->CreateConstExp(1);
      }
    }
    else
    {
      SymbolPtr ret = tacbuilder->CreateTempVariable(ValueType::Int);
      TACListPtr tac1 = tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Variable, ret));
      TACListPtr tac2 = tacbuilder->CreateAssign(ret, tacbuilder->CreateConstExp(1))->tac;

      TACListPtr tac3 = tacbuilder->CreateAssign(ret, tacbuilder->CreateConstExp(0))->tac;
      TACListPtr tac4 = tacbuilder->CreateAssign(ret, tacbuilder->CreateConstExp(1))->tac;
      TACListPtr tac5 = tacbuilder->CreateIfElse($3,tac4,tac3);
      ExpressionPtr exp2 = tacbuilder->NewExp(tacbuilder->CreateIfElse($1,tac2,tac5), ret);
      $$ = exp2;
    }
  }  
  ;

ConstExp : AddExp ;


%%


void HaveFunCompiler::Parser::Parser::error(const location_type &l,const std::string &err_message){
  std::cerr << "Error: "<<err_message <<" at "<<l<<"\n";
}

