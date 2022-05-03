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
  auto tac_list = NewTACList();
  (*tac_list) += NewTAC(TACOperationType::Label, label);
  (*tac_list) += NewTAC(TACOperationType::FunctionBegin);
  (*tac_list) += (*args);
  (*tac_list) += (*body);
  (*tac_list) += NewTAC(TACOperationType::FunctionEnd);
  return tac_list;
}

ExpressionPtr TACFactory::MakeAssign(SymbolPtr var, ExpressionPtr exp) {
  auto tac_list = exp->tac->MakeCopy();
  (*tac_list) += NewTAC(TACOperationType::Assign, var, exp->ret);
  return NewExp(tac_list, var);
}

TACListPtr TACFactory::MakeCall(const std::string &name, ExpListPtr args) {
  auto tac_list = NewTACList();
  for (auto exp : *args) {
    (*tac_list) += *exp->tac;
  }
  for (auto exp : *args) {
    (*tac_list) += NewTAC(TACOperationType::Argument, exp->ret);
  }
  (*tac_list) += NewTAC(TACOperationType::Call, nullptr, NewSymbol(SymbolType::Function, name));
  return tac_list;
}

ExpressionPtr TACFactory::MakeCallWithRet(const std::string &name, ExpListPtr args, SymbolPtr ret_sym) {
  auto tac_list = NewTACList();
  (*tac_list) += NewTAC(TACOperationType::Variable, ret_sym);
  for (auto exp : *args) {
    (*tac_list) += *exp->tac;
  }
  for (auto exp : *args) {
    (*tac_list) += NewTAC(TACOperationType::Argument, exp->ret);
  }
  (*tac_list) += NewTAC(TACOperationType::Call, ret_sym, NewSymbol(SymbolType::Function, name));
  return NewExp(tac_list, ret_sym);
}
TACListPtr TACFactory::MakeIf(ExpressionPtr cond, SymbolPtr label, TACListPtr stmt) {
  auto tac_list = NewTACList();
  (*tac_list) += *cond->tac;
  (*tac_list) += NewTAC(TACOperationType::IfZero, label, cond->ret);
  (*tac_list) += *stmt;
  (*tac_list) += NewTAC(TACOperationType::Label, label);
  return tac_list;
}

TACListPtr TACFactory::MakeTest(ExpressionPtr cond, SymbolPtr label_true, TACListPtr stmt_true, SymbolPtr label_false,
                                TACListPtr stmt_false) {
  auto tac_list = NewTACList();
  (*tac_list) += *cond->tac;
  (*tac_list) += NewTAC(TACOperationType::IfZero, label_true, cond->ret);
  (*tac_list) += *stmt_true;
  (*tac_list) += NewTAC(TACOperationType::Goto, label_false);
  (*tac_list) += NewTAC(TACOperationType::Label, label_true);
  (*tac_list) += *stmt_false;
  (*tac_list) += NewTAC(TACOperationType::Label, label_false);

  return tac_list;
}

TACListPtr TACFactory::MakeWhile(ExpressionPtr cond, SymbolPtr label_cont, SymbolPtr label_brk, TACListPtr stmt) {
  auto new_stmt = NewTACList();
  (*new_stmt) += *stmt;
  (*new_stmt) += NewTAC(TACOperationType::Goto, label_cont);
  auto tac_list = NewTACList(NewTAC(TACOperationType::Label, label_cont));
  (*tac_list) += *MakeIf(cond, label_brk, new_stmt);
  return tac_list;
}
TACListPtr TACFactory::MakeFor(TACListPtr init, ExpressionPtr cond, TACListPtr modify, SymbolPtr label_cont,
                               SymbolPtr label_brk, TACListPtr stmt) {
  return NewTACList(*init + *MakeWhile(cond, label_cont, label_brk, NewTACList(*stmt + *modify)));
}

TACBuilder::TACBuilder() : cur_temp_var_(0), cur_temp_label_(0) {}

void TACBuilder::EnterSubscope() { symbol_stack_.emplace_back(); }

void TACBuilder::ExitSubscope() { symbol_stack_.pop_back(); }

SymbolPtr TACBuilder::CreateConst(int n) {
  return TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(n));
}
SymbolPtr TACBuilder::CreateConst(float fn) {
  return TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(fn));
}

SymbolPtr TACBuilder::CreateText(const std::string &text) {
  if (auto it = text_tab_.find(text); it != text_tab_.end()) {
    return it->second;
  }
  stored_text_.push_back(text);

  auto ret = TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0,
                                               SymbolValue(stored_text_.back().c_str()));
  text_tab_[text] = ret;
  return ret;
}

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler