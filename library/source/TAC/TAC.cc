#include "TAC/TAC.hh"

namespace HaveFunCompiler {
namespace ThreeAddressCode {

SymbolPtr TACFactory::NewSymbol(SymbolType type, std::optional<std::string> name, int offset, SymbolValue value) {
  SymbolPtr sym = std::make_shared<Symbol>();
  sym->type_ = type;
  sym->name_ = name;
  sym->offset_ = offset;
  sym->value_ = value;
  return sym;
}

ThreeAddressCodePtr TACFactory::NewTAC(TACOperationType operation, SymbolPtr a = nullptr, SymbolPtr b, SymbolPtr c) {
  ThreeAddressCodePtr tac = std::make_shared<ThreeAddressCode>();
  tac->operation_ = operation;
  tac->a_ = a;
  tac->b_ = b;
  tac->c_ = c;
  return tac;
}

ExpressionPtr TACFactory::NewExp(TACListPtr tac, SymbolPtr ret) {
  ExpressionPtr exp = std::make_shared<Expression>();
  exp->ret = ret;
  exp->tac = tac;
  return exp;
}

TACListPtr TACFactory::MakeFunction(SymbolPtr label, TACListPtr args, TACListPtr body) {
  TACListPtr tac_list = std::make_shared<ThreeAddressCodeList>();
  (*tac_list) += NewTAC(TACOperationType::Label, label);
  (*tac_list) += NewTAC(TACOperationType::FunctionBegin);
  (*tac_list) += (*args);
  (*tac_list) += (*body);
  (*tac_list) += NewTAC(TACOperationType::FunctionEnd);
  return tac_list;
}

ExpressionPtr TACFactory::MakeAssign(SymbolPtr var, ExpressionPtr exp) {
  TACListPtr tac_list = exp->tac->MakeCopy();
  (*tac_list) += NewTAC(TACOperationType::Assign, var, exp->ret);
  return NewExp(tac_list, var);
}

TACListPtr TACFactory::MakeCall(const std::string &name, ExpListPtr args) {
  TACListPtr tac_list = std::make_shared<ThreeAddressCodeList>();
  for (auto exp : *args) {
    (*tac_list) += *exp->tac;
  }
  for (auto exp : *args) {
    (*tac_list) += NewTAC(TACOperationType::Argument, exp->ret);
  }
  (*tac_list) += NewTAC(TACOperationType::Call, nullptr, NewSymbol(SymbolType::Function, name));
  return tac_list;
}

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler