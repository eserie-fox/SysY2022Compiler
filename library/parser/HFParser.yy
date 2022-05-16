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
%token INT EQ NE LT LE LEQ COM LS LB RS RB LM RM GT GE IF ELSE WHILE CONTINUE RETURN LA LO LN FLOAT CONST VOID BREAK ADD SUB MUL DIV MOD SEMI
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
%type <ExpressionPtr> ConstExp LVal Exp Cond AddExp LOrExp Number UnaryExp MulExp RelExp EqExp LAndExp ConstInitVal VarDef InitVal PrimaryExp
/* %type <HaveFunCompiler::parser::SymbolPtr>  */
%type <ParamListPtr> FuncFParams 
%type <ArgListPtr> FuncRParams
%type <SymbolPtr> FuncFParam
%type <TACOperationType> UnaryOp  
%type <std::string> FuncIdenti
%type FuncHead
%type <ArrayDescriptorPtr> ConstExp_list ConstInitVal_list InitVal_list
%type <std::vector<ExpressionPtr>> Exp_list


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
  }
  | IDENTIFIER ConstExp_list LEQ ConstInitVal
  {
    $2->base_offset = tacbuilder->CreateConstExp(0)->ret;
    int type;
    tacbuilder->Top(&type);
    $2->value_type = (ValueType)type;
    auto arraySym = tacbuilder->NewSymbol(SymbolType::Constant, $1, 0, SymbolValue($2));
    tacbuilder->BindConstName($1, arraySym);
    $2->base_addr = arraySym;
    auto arrayExp = tacbuilder->NewExp(tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Constant,arraySym)), arraySym);
    $$ = tacbuilder->CreateArrayInit(arrayExp, $4)->tac;
  }
  ;

ConstExp_list
  : LM ConstExp RM
  {
    auto array = tacbuilder->NewArrayDescriptor();
    int dim = $2->ret->value_.GetInt();
    array->dimensions.push_back(dim);
    $$ = array;
  }
  | ConstExp_list LM ConstExp RM
  {
    int dim = $3->ret->value_.GetInt();
    $1->dimensions.push_back(dim);
    $$ = $1;
  }
  ;

ConstInitVal
  : ConstExp
  {
    $$ = $1;
  }
  | LB RB
  {
    auto array = tacbuilder->NewArrayDescriptor();
    $$ = tacbuilder->CreateConstExp(array);
  }
  | LB ConstInitVal_list RB
  {
    $$ = tacbuilder->CreateConstExp($2);
  }
  ;

ConstInitVal_list
  : ConstInitVal
  {
    auto array = tacbuilder->NewArrayDescriptor();
    array->subarray->emplace(array->subarray->size(),$1);
    $$ = array;
  }
  | ConstInitVal_list COM ConstInitVal
  {
    $1->subarray->emplace($1->subarray->size(),$3);
    $$ = $1;
  }
  ;

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
    ExpressionPtr tempexp;
    if($3->ret->value_.Type()==ValueType::Array){
      SymbolPtr tempvar = tacbuilder->CreateTempVariable($3->ret->value_.GetArrayDescriptor()->value_type);
      (*$3->tac) += tacbuilder->NewTAC(TACOperationType::Variable,tempvar);
      tempexp = tacbuilder->CreateAssign(tempvar,$3);
    }else{
      tempexp = $3;
    }
    if(tempexp->ret->value_.Type()!=(ValueType)type){
      if((ValueType)type == ValueType::Int){
        exp = tacbuilder->CastFloatToInt(tempexp);
      }else{
        exp = tacbuilder->CastIntToFloat(tempexp);
      }
      (*exp->tac) += tacbuilder->NewTAC(TACOperationType::Variable,var);
      $$ = tacbuilder->CreateAssign(var, exp);
    }else{
      (*tempexp->tac) += tacbuilder->NewTAC(TACOperationType::Variable,var);
      $$ = tacbuilder->CreateAssign(var, tempexp);
    }
  }
  | IDENTIFIER ConstExp_list
  {
    $2->base_offset = tacbuilder->CreateConstExp(0)->ret;
    int type;
    tacbuilder->Top(&type);
    $2->value_type = (ValueType)type;
    auto arraySym = tacbuilder->CreateVariable($1, ValueType::Array);
    arraySym->value_ = SymbolValue($2);
    $2->base_addr = arraySym;
    auto arrayExp = tacbuilder->NewExp(tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Variable,arraySym)), arraySym);
    $$ = arrayExp;
  }
  | IDENTIFIER ConstExp_list LEQ InitVal
  {
    $2->base_offset = tacbuilder->CreateConstExp(0)->ret;
    int type;
    tacbuilder->Top(&type);
    $2->value_type = (ValueType)type;
    auto arraySym = tacbuilder->CreateVariable($1, ValueType::Array);
    arraySym->value_ = SymbolValue($2);
    $2->base_addr = arraySym;
    auto arrayExp = tacbuilder->NewExp(tacbuilder->NewTACList(tacbuilder->NewTAC(TACOperationType::Variable,arraySym)), arraySym);
    $$ = tacbuilder->CreateArrayInit(arrayExp, $4);
  }
  ;

InitVal
  : Exp
  {
    if($1->ret->value_.Type() == ValueType::Array)
    {
      auto arrayDescriptor = $1->ret->value_.GetArrayDescriptor();
      if($1->ret->type_ == SymbolType::Constant)
      {
        if (!arrayDescriptor->subarray->empty()) {
          assert(arrayDescriptor->subarray->size() == 1);
          $$ = arrayDescriptor->subarray->begin()->second;
        } else {
          if (arrayDescriptor->value_type == SymbolValue::ValueType::Int){
            $$ = tacbuilder->CreateConstExp(0);
          }
          else{
            $$ = tacbuilder->CreateConstExp(static_cast<float>(0));
          }
        }
      }else{
        SymbolPtr tmpVar = tacbuilder->CreateTempVariable(arrayDescriptor->value_type);
        (*$1->tac) += tacbuilder->NewTAC(TACOperationType::Variable,tmpVar);
        $$ = tacbuilder->CreateAssign(tmpVar,$1);
      }
    }else{
      $$ = $1;
    }
  }
  | LB RB
  {
    auto array = tacbuilder->NewArrayDescriptor();
    $$ = tacbuilder->NewExp(tacbuilder->NewTACList(),tacbuilder->NewSymbol(SymbolType::Variable,std::nullopt,0,SymbolValue(array)));
  }
  | LB InitVal_list RB
  {
    $$ = tacbuilder->NewExp(tacbuilder->NewTACList(),tacbuilder->NewSymbol(SymbolType::Variable,std::nullopt,0,SymbolValue($2)));
  }
  ;

InitVal_list
  : InitVal
  {
    auto array = tacbuilder->NewArrayDescriptor();
    array->subarray->emplace(array->subarray->size(),$1);
    $$ = array;
  }
  | InitVal_list COM InitVal
  {
    $1->subarray->emplace($1->subarray->size(),$3);
    $$ = $1;
  }
  ;

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
  /* | BType IDENTIFIER LM RM Exp_list
  | BType IDENTIFIER LM RM */
  ;

Exp_list
  : LM Exp RM
  {
    std::vector<ExpressionPtr> vec;
    vec.push_back($2);
    $$ = vec;
  }
  | Exp_list LM Exp RM
  {
    $1.push_back($3);
    $$ = $1;
  }
  ;

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
    if($1->ret->type_ == SymbolType::Constant)
    {
      throw RuntimeException("Cant assign to constant");
    }
    ExpressionPtr exp;
    if($3->ret->value_.Type()!=$1->ret->value_.Type()){
      if($1->ret->value_.Type() == ValueType::Int){
        exp = tacbuilder->CastFloatToInt($3);
      }else{
        exp = tacbuilder->CastIntToFloat($3);
      }
      $$ = tacbuilder->CreateAssign($1->ret, exp)->tac;
    }else{
      $$ = tacbuilder->CreateAssign($1->ret, $3)->tac;
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

LVal : IDENTIFIER
{
  $$ = tacbuilder->NewExp(tacbuilder->NewTACList(), tacbuilder->FindVariableOrConstant($1));
}
| IDENTIFIER Exp_list 
{
  auto arrayExp = tacbuilder->NewExp(tacbuilder->NewTACList(), tacbuilder->FindVariableOrConstant($1));
  $$ = tacbuilder->AccessArray(arrayExp, $2);
}
;

PrimaryExp
  : LS Exp RS
  {
    $$ = $2;
  }
  | LVal
  {
    $$ = $1;
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

