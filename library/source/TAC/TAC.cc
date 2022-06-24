#include "TAC/TAC.hh"
#include <algorithm>
#include <iostream>
#include <sstream>
#include "Exceptions.hh"
#include "MagicEnum.hh"

namespace HaveFunCompiler {
namespace ThreeAddressCode {
#define RUNTIME_EXCEPTION(msg) RuntimeException(*plocation_, msg)
#define TYPEMISMATCH_EXCEPTION(act, exp, msg) TypeMismatchException(*plocation_, act, exp, msg)
#define REDEFINITION_EXCEPTION(name) RedefinitionException(*plocation_, name)
#define NULL_EXCEPTION(file, line, msg) NullReferenceException(*plocation_, file, line, msg)

SymbolPtr TACFactory::NewSymbol(SymbolType type, std::optional<std::string> name, SymbolValue value, int offset) {
  SymbolPtr sym = std::make_shared<Symbol>();
  sym->type_ = type;
  sym->name_ = name;
  sym->offset_ = offset;
  sym->value_ = value;
  return sym;
}

ThreeAddressCodePtr TACFactory::NewTAC(TACOperationType operation, SymbolPtr a, SymbolPtr b, SymbolPtr c) {
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

ArgListPtr TACFactory::NewArgList() { return std::make_shared<ArgumentList>(); }
ParamListPtr TACFactory::NewParamList() { return std::make_shared<ParameterList>(); }
ArrayDescriptorPtr TACFactory::NewArrayDescriptor() {
  auto ret = std::make_shared<ArrayDescriptor>();
  ret->subarray = std::make_shared<std::unordered_map<size_t, std::shared_ptr<Expression>>>();
  return ret;
}

TACListPtr TACFactory::MakeFunction(const location *plocation_, SymbolPtr func_head, TACListPtr body) {
  auto tac_list = NewTACList();
  (*tac_list) += NewTAC(TACOperationType::Label, func_head);
  (*tac_list) += NewTAC(TACOperationType::FunctionBegin);
  auto params = func_head->value_.GetParameters();
  if (params == nullptr) {
    throw NULL_EXCEPTION(__FILE__, __LINE__, "'params' is null");
  }
  for (auto sym : *params) {
    (*tac_list) += NewTAC(TACOperationType::Parameter, sym);
  }
  (*tac_list) += body;
  (*tac_list) += NewTAC(TACOperationType::FunctionEnd);
  return tac_list;
}

// TACListPtr TACFactory::MakeCall(SymbolPtr func_label, ArgListPtr args) {
//   auto tac_list = NewTACList();
//   for (auto exp : *args) {
//     (*tac_list) += exp->tac;
//   }
//   for (auto exp : *args) {
//     (*tac_list) += NewTAC(TACOperationType::Argument, exp->ret);
//   }
//   (*tac_list) += NewTAC(TACOperationType::Call, nullptr, func_label);
//   return tac_list;
// }

TACListPtr TACFactory::MakeCallWithRet(const location *plocation_, SymbolPtr func_label, ArgListPtr args,
                                       SymbolPtr ret_sym) {
  if (args == nullptr) {
    throw NULL_EXCEPTION(__FILE__, __LINE__, "'args' is null");
  }
  {
    size_t nfuncparam = func_label->value_.GetParameters()->size();
    size_t narg = args->size();
    if (nfuncparam != narg) {
      throw RUNTIME_EXCEPTION("Function '" + func_label->get_name() + "' requires " + std::to_string(nfuncparam) +
                              " parameters, but " + std::to_string(narg) + " arguments provided");
    }
    auto paramit = func_label->value_.GetParameters()->begin();
    auto paramend = func_label->value_.GetParameters()->end();
    auto argit = args->begin();
    size_t ith = 0;
    for (; paramit != paramend; ++paramit, ++argit) {
      ++ith;
      if (!(*argit)->ret->value_.IsAssignableTo((*paramit)->value_)) {
        throw TYPEMISMATCH_EXCEPTION(
            (*argit)->ret->value_.TypeToString(), (*paramit)->value_.TypeToString(),
            "Function '" + func_label->get_name() + "'s " + std::to_string(ith) + "th parameter type mismatch");
      }
    }
  }
  auto tac_list = NewTACList();
  (*tac_list) += NewTAC(TACOperationType::Variable, ret_sym);
  for (auto exp : *args) {
    (*tac_list) += exp->tac;
  }
  for (auto exp : *args) {
    (*tac_list) += NewTAC(TACOperationType::Argument, exp->ret);
  }
  (*tac_list) += NewTAC(TACOperationType::Call, ret_sym, func_label);
  return tac_list;
}

bool TACFactory::CheckConditionType(ExpressionPtr cond, bool *out_const_result) {
  if (cond->ret->type_ != SymbolType::Constant) {
    return false;
  }
  if (out_const_result) {
    *out_const_result = static_cast<bool>(cond->ret);
  }
  return true;
}
TACListPtr TACFactory::MakeIf(ExpressionPtr cond, SymbolPtr label, TACListPtr stmt) {
  auto tac_list = NewTACList();
  (*tac_list) += cond->tac;
  (*tac_list) += NewTAC(TACOperationType::IfZero, label, cond->ret);
  (*tac_list) += stmt;
  (*tac_list) += NewTAC(TACOperationType::Label, label);
  return tac_list;
}

TACListPtr TACFactory::MakeIfElse(ExpressionPtr cond, SymbolPtr label_true, TACListPtr stmt_true, SymbolPtr label_false,
                                  TACListPtr stmt_false) {
  auto tac_list = NewTACList();
  (*tac_list) += cond->tac;
  (*tac_list) += NewTAC(TACOperationType::IfZero, label_true, cond->ret);
  (*tac_list) += stmt_true;
  (*tac_list) += NewTAC(TACOperationType::Goto, label_false);
  (*tac_list) += NewTAC(TACOperationType::Label, label_true);
  (*tac_list) += stmt_false;
  (*tac_list) += NewTAC(TACOperationType::Label, label_false);

  return tac_list;
}

TACListPtr TACFactory::MakeWhile(ExpressionPtr cond, SymbolPtr label_cont, SymbolPtr label_brk, TACListPtr stmt) {
  auto new_stmt = NewTACList();
  (*new_stmt) += stmt;
  (*new_stmt) += NewTAC(TACOperationType::Goto, label_cont);
  auto tac_list = NewTACList(NewTAC(TACOperationType::Label, label_cont));
  (*tac_list) += MakeIf(cond, label_brk, new_stmt);
  return tac_list;
}

TACListPtr TACFactory::MakeWhile(ExpressionPtr cond_not, SymbolPtr label_cont, SymbolPtr label_brk,
                                 SymbolPtr label_loop, TACListPtr stmt) {
  auto tac_list = NewTACList(NewTAC(TACOperationType::Goto, label_cont));
  (*tac_list) += MakeDoWhile(cond_not, label_cont, label_brk, label_loop, stmt);
  return tac_list;
}

TACListPtr TACFactory::MakeDoWhile(ExpressionPtr cond_not, SymbolPtr label_cont, SymbolPtr label_brk,
                                   SymbolPtr label_loop, TACListPtr stmt) {
  auto tac_list = NewTACList();
  (*tac_list) += NewTAC(TACOperationType::Label, label_loop);
  (*tac_list) += stmt;
  (*tac_list) += NewTAC(TACOperationType::Label, label_cont);
  (*tac_list) += cond_not->tac;
  (*tac_list) += NewTAC(TACOperationType::IfZero, label_loop, cond_not->ret);
  (*tac_list) += NewTAC(TACOperationType::Label, label_brk);
  return tac_list;
}
TACListPtr TACFactory::MakeFor(TACListPtr init, ExpressionPtr cond, TACListPtr modify, SymbolPtr label_cont,
                               SymbolPtr label_brk, TACListPtr stmt) {
  return NewTACList(*init + *MakeWhile(cond, label_cont, label_brk, NewTACList(*stmt + *modify)));
}

std::string TACFactory::ToFuncLabelName(const std::string name) { return "U_" + name; }
std::string TACFactory::ToCustomerLabelName(const std::string name) { return "U_" + name; }
std::string TACFactory::ToTempLabelName(uint64_t id) { return "SL_" + std::to_string(id); }
std::string TACFactory::ToVariableOrConstantName(const std::string name) { return "U_" + name; }
std::string TACFactory::ToTempVariableName(uint64_t id) { return "SV_" + std::to_string(id); }

TACBuilder::TACBuilder() : cur_temp_var_(0), cur_temp_label_(0), cur_symtab_id_(0) { EnterSubscope(); }

SymbolPtr TACBuilder::NewSymbol(SymbolType type, std::optional<std::string> name, SymbolValue value, int offset) {
  return TACFactory::Instance()->NewSymbol(type, name, value, offset);
}
ThreeAddressCodePtr TACBuilder::NewTAC(TACOperationType operation, SymbolPtr a, SymbolPtr b, SymbolPtr c) {
  return TACFactory::Instance()->NewTAC(operation, a, b, c);
}
ExpressionPtr TACBuilder::NewExp(TACListPtr tac, SymbolPtr ret) { return TACFactory::Instance()->NewExp(tac, ret); }

ArgListPtr TACBuilder::NewArgList() { return TACFactory::Instance()->NewArgList(); }

ParamListPtr TACBuilder::NewParamList() { return TACFactory::Instance()->NewParamList(); }

ArrayDescriptorPtr TACBuilder::NewArrayDescriptor() { return TACFactory::Instance()->NewArrayDescriptor(); }

ExpressionPtr TACBuilder::CreateAssign(SymbolPtr var, ExpressionPtr exp) {
  if (exp->tac == nullptr) {
    throw NULL_EXCEPTION(__FILE__, __LINE__, "'tac' is null");
  }
  if (!exp->ret->value_.IsAssignableTo(var->value_)) {
    throw TYPEMISMATCH_EXCEPTION(var->value_.TypeToString(), exp->ret->value_.TypeToString(),
                                 " Can't assign this actual type to expected type");
  }

  // 如果两者都是Array
  auto tac_list = NewTACList();
  if (exp->ret->value_.Type() == SymbolValue::ValueType::Array && var->value_.Type() == SymbolValue::ValueType::Array) {
    exp = NewExp(exp->tac->MakeCopy(), exp->ret);
    auto tmpSym = CreateTempVariable(exp->ret->value_.UnderlyingType());
    (*exp->tac) += NewTAC(TACOperationType::Variable, tmpSym);
    (*exp->tac) += NewTAC(TACOperationType::Assign, tmpSym, exp->ret);
    exp->ret = tmpSym;
  }
  (*tac_list) += exp->tac;
  (*tac_list) += NewTAC(TACOperationType::Assign, var, exp->ret);
  return NewExp(tac_list, var);
}

void TACBuilder::FlattenInitArrayImpl(FlattenedArray *out_result, ArrayDescriptorPtr array) {
  out_result->emplace_back(1, nullptr);
  std::vector<std::pair<size_t, ExpressionPtr>> subarray;
  for (const auto &[idx, valPtr] : *array->subarray) {
    subarray.emplace_back(idx, valPtr);
  }
  std::sort(subarray.begin(), subarray.end());
  for (const auto &[idx, valPtr] : subarray) {
    if (valPtr->ret->value_.Type() != SymbolValue::ValueType::Array) {
      out_result->emplace_back(0, valPtr);
    }
    // else if (auto arrayDescriptor = valPtr->ret->value_.GetArrayDescriptor(); arrayDescriptor->dimensions.empty()) {
    //   if (!arrayDescriptor->base_addr.expired() && arrayDescriptor->base_addr.lock()->type_ == SymbolType::Constant)
    //   {
    //     if (!arrayDescriptor->subarray->empty()) {
    //       assert(arrayDescriptor->subarray->size() == 1);
    //       out_result->emplace_back(0, arrayDescriptor->subarray->begin()->second);
    //     } else {
    //       if (arrayDescriptor->value_type == SymbolValue::ValueType::Int)
    //         out_result->emplace_back(0, CreateConstExp(0));
    //       else
    //         out_result->emplace_back(0, CreateConstExp(static_cast<float>(0)));
    //     }
    //   } else {
    //     auto tmpVar = CreateTempVariable(arrayDescriptor->value_type);
    //     (*valPtr->tac) += NewTAC(TACOperationType::Variable, tmpVar);
    //     auto tmpExp = CreateAssign(tmpVar, valPtr);
    //     out_result->emplace_back(0, tmpExp);
    //   }
    // }
    else {
      FlattenInitArrayImpl(out_result, valPtr->ret->value_.GetArrayDescriptor());
    }
  }
  out_result->emplace_back(2, nullptr);
}

TACBuilder::FlattenedArray TACBuilder::FlattenInitArray(ArrayDescriptorPtr array) {
  FlattenedArray ret;
  FlattenInitArrayImpl(&ret, array);
  return ret;
}

ExpressionPtr TACBuilder::AccessArray(ExpressionPtr array, std::vector<ExpressionPtr> pos) {
  auto arrayDescriptor = array->ret->value_.GetArrayDescriptor();
  if (pos.size() > arrayDescriptor->dimensions.size()) {
    throw RUNTIME_EXCEPTION("Array only has " + std::to_string(arrayDescriptor->dimensions.size()) +
                            " dimensions but access " + std::to_string(pos.size()) + "th dimension");
  }
  if (pos.empty()) {
    return array;
  }
  auto idx_exp = pos.front();
  pos.erase(pos.cbegin());
  if (array->ret->type_ == SymbolType::Constant && idx_exp->ret->type_ == SymbolType::Constant &&
      arrayDescriptor->base_offset->type_ == SymbolType::Constant) {
    int idx = idx_exp->ret->value_.GetInt();
    if (idx < 0) {
      throw RUNTIME_EXCEPTION("Array index must be non-negative, but encountered " + std::to_string(idx));
    }
    if (arrayDescriptor->subarray->count(idx)) {
      auto val = arrayDescriptor->subarray->at(idx);
      return AccessArray(val, pos);
    }
    auto nArrayDescriptor = NewArrayDescriptor();

    size_t size_sublen = 1;
    for (size_t i = 1; i < arrayDescriptor->dimensions.size(); i++) {
      size_sublen *= arrayDescriptor->dimensions[i];
      nArrayDescriptor->dimensions.push_back(arrayDescriptor->dimensions[i]);
    }

    nArrayDescriptor->base_addr = arrayDescriptor->base_addr;
    nArrayDescriptor->value_type = arrayDescriptor->value_type;
    ssize_t noffset = arrayDescriptor->base_offset->value_.GetInt() + idx * (ssize_t)size_sublen;
    if (noffset > (ssize_t)INT32_MAX || noffset < 0) {
      throw RUNTIME_EXCEPTION("Invalid array access at " + std::to_string(noffset));
    }
    nArrayDescriptor->base_offset = CreateConstExp(static_cast<int>(noffset))->ret;
    auto nArraySym = NewSymbol(array->ret->type_, std::nullopt, SymbolValue(nArrayDescriptor));
    arrayDescriptor->subarray->emplace(idx, NewExp(NewTACList(), nArraySym));
    return AccessArray(NewExp(array->tac, nArraySym), pos);
  } else {
    auto tac_list = NewTACList(array->tac);
    auto nArrayDescriptor = NewArrayDescriptor();

    size_t size_sublen = 1;
    for (size_t i = 1; i < arrayDescriptor->dimensions.size(); i++) {
      size_sublen *= arrayDescriptor->dimensions[i];
      nArrayDescriptor->dimensions.push_back(arrayDescriptor->dimensions[i]);
    }
    nArrayDescriptor->base_addr = arrayDescriptor->base_addr;
    nArrayDescriptor->value_type = arrayDescriptor->value_type;
    auto offset_exp = NewExp(NewTACList(), arrayDescriptor->base_offset);
    offset_exp = CreateArithmeticOperation(
        TACOperationType::Add, offset_exp,
        CreateArithmeticOperation(TACOperationType::Mul, idx_exp, CreateConstExp((int)size_sublen)));
    (*tac_list) += offset_exp->tac;
    nArrayDescriptor->base_offset = offset_exp->ret;
    return AccessArray(NewExp(tac_list, NewSymbol(array->ret->type_, std::nullopt, nArrayDescriptor)), pos);
  }
}

int TACBuilder::ArrayInitImpl(ExpressionPtr array, FlattenedArray::iterator &it, const FlattenedArray::iterator &end,
                              TACListPtr tac_list) {
  enum { OK, IGNORE };
  if (it == end) {
    return IGNORE;
  }
  auto arrayDescriptor = array->ret->value_.GetArrayDescriptor();
  if (arrayDescriptor->dimensions.empty()) {
    if (it->first == 1) {
      throw RUNTIME_EXCEPTION(
          "The depth of brace nesting in array initialization expression is too deep for the target array/subarray");
    }
    if (it->first == 2) {
      return IGNORE;
    }
    (*tac_list) += CreateAssign(array->ret, it->second)->tac;
    arrayDescriptor->subarray->emplace(0, it->second);
    ++it;
    return OK;
  }
  for (size_t i = 0; i < arrayDescriptor->dimensions[0]; i++) {
    if (it->first == 1) {
      ++it;
      ArrayInitImpl(AccessArray(array, {CreateConstExp((int)i)}), it, end, tac_list);
      if (it->first != 2) {
        throw RUNTIME_EXCEPTION("Too many elements in array initialization expression for the target array/subarray");
      }
      ++it;
      continue;
    }
    if (it->first == 2) {
      return IGNORE;
    }
    if (IGNORE == ArrayInitImpl(AccessArray(array, {CreateConstExp((int)i)}), it, end, tac_list)) {
      return IGNORE;
    }
  }
  return OK;
}

ExpressionPtr TACBuilder::CreateArrayInit(ExpressionPtr array, ExpressionPtr init_array) {
  // if (array->type_ != SymbolType::Variable && array->type_ != SymbolType::Constant) {
  //   throw LogicException("[" + std::string(__func__) + "] Variabel or Constant are expected for array, but actually "
  //   +
  //                        std::string(magic_enum::enum_name<SymbolType>(array->type_)));
  // }
  // if (init_array->type_ != SymbolType::Variable && init_array->type_ != SymbolType::Constant) {
  //   throw LogicException("[" + std::string(__func__) +
  //                        "] Variabel or Constant are expected for init_array, but actually " +
  //                        std::string(magic_enum::enum_name<SymbolType>(init_array->type_)));
  // }
  // if (array->value_.Type() != SymbolValue::ValueType::Array &&
  //     init_array->value_.Type() != SymbolValue::ValueType::Array) {
  //   throw LogicException("[" + std::string(__func__) +
  //                        "] array and init_array both are expected to be 'Array' type, but actually " +
  //                        array->value_.TypeToString() + " and " + init_array->value_.TypeToString());
  // }

  auto finit_array = FlattenInitArray(init_array->ret->value_.GetArrayDescriptor());
  for (auto &pr : finit_array) {
    if (pr.second) {
      if (array->ret->type_ == SymbolType::Constant && pr.second->ret->type_ != SymbolType::Constant) {
        throw TYPEMISMATCH_EXCEPTION(
            std::string(magic_enum::enum_name<SymbolType>(pr.second->ret->type_)), "Constant",
            pr.second->ret->get_name() + "'s type mismatched, can't initialize const array with non-constant");
      }
      if (pr.second->ret->type_ != SymbolType::Constant && pr.second->ret->type_ != SymbolType::Variable) {
        throw TYPEMISMATCH_EXCEPTION(std::string(magic_enum::enum_name<SymbolType>(pr.second->ret->type_)),
                                     "Constant or Variable", pr.second->ret->get_name() + "'s type mismatched");
      }
    }
  }
  if (array->ret->value_.GetArrayDescriptor()->value_type == SymbolValue::ValueType::Int) {
    for (auto &pr : finit_array) {
      if (pr.second != nullptr && pr.second->ret->value_.Type() != SymbolValue::ValueType::Int) {
        throw TYPEMISMATCH_EXCEPTION(
            pr.second->ret->value_.TypeToString(), "Int",
            pr.second->ret->get_name() + "'s value type mismatched, can't initialize int array with non-Int type");
      }
    }
  }
  auto init_array_it = finit_array.begin();
  ++init_array_it;
  ArrayInitImpl(array, init_array_it, finit_array.end(), array->tac);
  return array;
}
void TACBuilder::EnterSubscope() { symbol_stack_.emplace_back(cur_symtab_id_++); }

void TACBuilder::ExitSubscope() { symbol_stack_.pop_back(); }

ExpressionPtr TACBuilder::CreateConstExp(int n) { return CreateConstExp(SymbolValue(n)); }
ExpressionPtr TACBuilder::CreateConstExp(float fn) { return CreateConstExp(SymbolValue(fn)); }

ExpressionPtr TACBuilder::CreateConstExp(SymbolValue v) {
  return TACFactory::Instance()->NewExp(TACFactory::Instance()->NewTACList(),
                                        TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, v));
}

SymbolPtr TACBuilder::CreateText(const std::string &text) {
  if (auto it = text_tab_.find(text); it != text_tab_.end()) {
    return it->second;
  }
  stored_text_.push_back(text);

  auto ret =
      TACFactory::Instance()->NewSymbol(SymbolType::Text, std::nullopt, SymbolValue(stored_text_.back().c_str()));
  text_tab_[text] = ret;
  return ret;
}

std::string TACBuilder::AppendScopePrefix(const std::string &name, uint64_t scope_id) {
  if (scope_id == (uint64_t)-1) {
    scope_id = symbol_stack_.back().get_scope_id();
  }
  return std::string("S") + std::to_string(scope_id) + name;
}

SymbolPtr TACBuilder::CreateFunctionLabel(const std::string &name) {
  std::string label_name = AppendScopePrefix(TACFactory::Instance()->ToFuncLabelName(name));
  if (symbol_stack_.back().count(label_name)) {
    throw REDEFINITION_EXCEPTION(name);
  }
  auto sym = TACFactory::Instance()->NewSymbol(SymbolType::Function, label_name);
  symbol_stack_.back()[label_name] = sym;
  return sym;
}

SymbolPtr TACBuilder::CreateCustomerLabel(const std::string &name) {
  std::string label_name = AppendScopePrefix(TACFactory::Instance()->ToCustomerLabelName(name));
  if (symbol_stack_.back().count(label_name)) {
    throw REDEFINITION_EXCEPTION(name);
  }
  auto sym = TACFactory::Instance()->NewSymbol(SymbolType::Label, label_name);
  symbol_stack_.back()[label_name] = sym;
  return sym;
}

SymbolPtr TACBuilder::CreateTempLabel() {
  std::string label_name = AppendScopePrefix(TACFactory::Instance()->ToTempLabelName(cur_temp_label_++));
  auto sym = TACFactory::Instance()->NewSymbol(SymbolType::Label, label_name);
  symbol_stack_.back()[label_name] = sym;
  return sym;
}

SymbolPtr TACBuilder::CreateVariable(const std::string &name, SymbolValue::ValueType type) {
  std::string var_name = AppendScopePrefix(TACFactory::Instance()->ToVariableOrConstantName(name));
  if (symbol_stack_.back().count(var_name)) {
    throw REDEFINITION_EXCEPTION(name);
  }
  auto sym = TACFactory::Instance()->NewSymbol(SymbolType::Variable, var_name);
  sym->value_.SetType(type);
  symbol_stack_.back()[var_name] = sym;
  return sym;
}

SymbolPtr TACRebuilder::CreateVariable(const std::string &name, SymbolValue::ValueType type) {
  auto sym = TACFactory::Instance()->NewSymbol(SymbolType::Variable, name);
  sym->value_.SetType(type);
  InsertSymbol(name, sym);
  return sym;
}
SymbolPtr TACBuilder::CreateTempVariable(SymbolValue::ValueType type) {
  std::string var_name = AppendScopePrefix(TACFactory::Instance()->ToTempVariableName(cur_temp_var_++));
  auto sym = TACFactory::Instance()->NewSymbol(SymbolType::Variable, var_name);
  sym->value_.SetType(type);
  symbol_stack_.back()[var_name] = sym;
  return sym;
}

SymbolPtr TACBuilder::CreateFunctionHead(SymbolValue::ValueType ret_type, SymbolPtr func_label, ParamListPtr params) {
  params->set_return_type(ret_type);
  func_label->value_ = SymbolValue(params);
  return func_label;
}

TACListPtr TACBuilder::CreateFunction(SymbolPtr func_head, TACListPtr body) {
  return TACFactory::Instance()->MakeFunction(plocation_, func_head, body);
}

// TACListPtr TACBuilder::CreateCall(const std::string &func_name, ArgListPtr args) {
//   auto func_label = FindFunctionLabel(func_name);
//   if (func_label == nullptr) {
//     Error("Function with name '" + func_name + "' is not found!");
//     return nullptr;
//   }
//   return TACFactory::Instance()->MakeCall(func_label, args);
// }
TACListPtr TACBuilder::CreateCallWithRet(const std::string &func_name, ArgListPtr args, SymbolPtr ret_sym) {
  auto func_label = FindFunctionLabel(func_name);
  if (func_label == nullptr) {
    throw RUNTIME_EXCEPTION("Function with name '" + func_name + "' is not found");
  }
  return TACFactory::Instance()->MakeCallWithRet(plocation_, func_label, args, ret_sym);
}
TACListPtr TACBuilder::CreateIf(ExpressionPtr cond, TACListPtr stmt, SymbolPtr *out_label) {
  auto label = CreateTempLabel();
  if (out_label) {
    *out_label = label;
  }
  return TACFactory::Instance()->MakeIf(cond, label, stmt);
}
TACListPtr TACBuilder::CreateIfElse(ExpressionPtr cond, TACListPtr stmt_true, TACListPtr stmt_false,
                                    SymbolPtr *out_label_true, SymbolPtr *out_label_false) {
  auto label_true = CreateTempLabel();
  auto label_false = CreateTempLabel();
  if (out_label_true) {
    *out_label_true = label_true;
  }
  if (out_label_false) {
    *out_label_false = label_false;
  }
  return TACFactory::Instance()->MakeIfElse(cond, label_true, stmt_true, label_false, stmt_false);
}

// if型
TACListPtr TACBuilder::CreateWhileIfModel(ExpressionPtr cond, TACListPtr stmt, SymbolPtr label_cont,
                                          SymbolPtr label_brk) {
  return TACFactory::Instance()->MakeWhile(cond, label_cont, label_brk, stmt);
}

TACListPtr TACBuilder::CreateWhile(ExpressionPtr cond, TACListPtr stmt, SymbolPtr label_cont, SymbolPtr label_brk) {
  return TACFactory::Instance()->MakeWhile(CreateArithmeticOperation(TACOperationType::UnaryNot, cond), label_cont,
                                           label_brk, CreateTempLabel(), stmt);
}

TACListPtr TACBuilder::CreateDoWhile(ExpressionPtr cond, TACListPtr stmt, SymbolPtr label_cont, SymbolPtr label_brk) {
  return TACFactory::Instance()->MakeDoWhile(CreateArithmeticOperation(TACOperationType::UnaryNot, cond), label_cont,
                                             label_brk, CreateTempLabel(), stmt);
}
TACListPtr TACBuilder::CreateFor(TACListPtr init, ExpressionPtr cond, TACListPtr modify, TACListPtr stmt,
                                 SymbolPtr label_cont, SymbolPtr label_brk) {
  return TACFactory::Instance()->MakeFor(init, cond, modify, label_cont, label_brk, stmt);
}

void TACBuilder::BindConstName(const std::string &name, SymbolPtr constant) {
  std::string const_name = AppendScopePrefix(TACFactory::Instance()->ToVariableOrConstantName(name));
  if (symbol_stack_.back().count(const_name)) {
    throw REDEFINITION_EXCEPTION(name);
  }
  symbol_stack_.back()[const_name] = constant;
}

SymbolPtr TACBuilder::FindSymbolWithName(const std::string &name) {
  for (auto s_it = symbol_stack_.rbegin(); s_it != symbol_stack_.rend(); ++s_it) {
    if (auto it = s_it->find(AppendScopePrefix(name, s_it->get_scope_id())); it != s_it->end()) {
      return it->second;
    }
  }
  return nullptr;
}

SymbolPtr TACBuilder::FindVariableOrConstant(const std::string &name) {
  std::string var_name = TACFactory::Instance()->ToVariableOrConstantName(name);
  auto ret = FindSymbolWithName(var_name);
  if (ret == nullptr) {
    throw RUNTIME_EXCEPTION("Variable or Constant named '" + name + "' is not found");
  }
  return ret;
}
SymbolPtr TACBuilder::FindFunctionLabel(const std::string &name) {
  std::string func_name = TACFactory::Instance()->ToFuncLabelName(name);
  auto ret = FindSymbolWithName(func_name);
  if (ret == nullptr) {
    throw RUNTIME_EXCEPTION("Function named '" + name + "' is not found");
  }
  return ret;
}
SymbolPtr TACBuilder::FindCustomerLabel(const std::string &name) {
  std::string label_name = TACFactory::Instance()->ToCustomerLabelName(name);
  auto ret = FindSymbolWithName(label_name);
  if (ret == nullptr) {
    throw RUNTIME_EXCEPTION("Label named '" + name + "' is not found");
  }
  return ret;
}
void TACBuilder::PushLoop(SymbolPtr label_cont, SymbolPtr label_brk) {
  loop_cont_brk_stack_.emplace_back(label_cont, label_brk);
}

void TACBuilder::TopLoop(SymbolPtr *out_label_cont, SymbolPtr *out_label_brk) {
  if (loop_cont_brk_stack_.empty()) {
    throw RUNTIME_EXCEPTION("Can't break or continue, you are not in a loop");
  }
  if (out_label_cont) {
    *out_label_cont = loop_cont_brk_stack_.back().first;
  }
  if (out_label_brk) {
    *out_label_brk = loop_cont_brk_stack_.back().second;
  }
}

void TACBuilder::PopLoop() { loop_cont_brk_stack_.pop_back(); }

ExpressionPtr TACBuilder::CastFloatToInt(ExpressionPtr expF) {
  if (expF->ret->type_ == SymbolType::Constant) {
    return CreateConstExp(static_cast<int>(expF->ret->value_.GetFloat()));
  }
  if (expF->ret->type_ == SymbolType::Variable) {
    if (expF->ret->value_.UnderlyingType() != SymbolValue::ValueType::Float) {
      throw TYPEMISMATCH_EXCEPTION(expF->ret->value_.TypeToString(), "Float", "Cast fail");
    }
    auto tmpSym = CreateTempVariable(SymbolValue::ValueType::Int);
    auto tac_list = TACFactory::Instance()->NewTACList(expF->tac);
    (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpSym);
    (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::FloatToInt, tmpSym, expF->ret);
    return TACFactory::Instance()->NewExp(tac_list, tmpSym);
  }
  throw TYPEMISMATCH_EXCEPTION(std::string(magic_enum::enum_name<SymbolType>(expF->ret->type_)), "Constant or Variable",
                               "You can only cast constant or variable to int");
}
ExpressionPtr TACBuilder::CastIntToFloat(ExpressionPtr expI) {
  if (expI->ret->type_ == SymbolType::Constant) {
    return CreateConstExp(static_cast<float>(expI->ret->value_.GetInt()));
  }
  if (expI->ret->type_ == SymbolType::Variable) {
    if (expI->ret->value_.UnderlyingType() != SymbolValue::ValueType::Int) {
      throw TYPEMISMATCH_EXCEPTION(expI->ret->value_.TypeToString(), "Int", "Cast fail");
    }
    auto tmpSym = CreateTempVariable(SymbolValue::ValueType::Float);
    auto tac_list = TACFactory::Instance()->NewTACList(expI->tac);
    (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpSym);
    (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::IntToFloat, tmpSym, expI->ret);
    return TACFactory::Instance()->NewExp(tac_list, tmpSym);
  }
  throw TYPEMISMATCH_EXCEPTION(std::string(magic_enum::enum_name<SymbolType>(expI->ret->type_)), "Constant or Variable",
                               "You can only cast constant or variable to float");
}

ExpressionPtr TACBuilder::CreateArithmeticOperation(TACOperationType arith_op, ExpressionPtr exp1, ExpressionPtr exp2) {
  if (exp1->ret->type_ == SymbolType::Constant) {
    auto val1 = exp1->ret->value_;
    switch (arith_op) {
      case TACOperationType::UnaryPositive:
        return CreateConstExp(+val1);

      case TACOperationType::UnaryMinus:
        return CreateConstExp(-val1);

      case TACOperationType::UnaryNot:
        return CreateConstExp(!val1);

      default:
        break;
    }
    if (exp2->ret->type_ == SymbolType::Constant) {
      auto val2 = exp2->ret->value_;
      switch (arith_op) {
        case TACOperationType::Add:
          return CreateConstExp(val1 + val2);
        case TACOperationType::Sub:
          return CreateConstExp(val1 - val2);
        case TACOperationType::Mul:
          return CreateConstExp(val1 * val2);
        case TACOperationType::Div:
          return CreateConstExp(val1 / val2);
        case TACOperationType::Mod:
          return CreateConstExp(val1 % val2);
        case TACOperationType::LessThan:
          return CreateConstExp(val1 < val2);
        case TACOperationType::LessOrEqual:
          return CreateConstExp(val1 <= val2);
        case TACOperationType::GreaterThan:
          return CreateConstExp(val1 > val2);
        case TACOperationType::GreaterOrEqual:
          return CreateConstExp(val1 >= val2);
        case TACOperationType::Equal:
          return CreateConstExp(val1 == val2);
        case TACOperationType::NotEqual:
          return CreateConstExp(val1 != val2);
        case TACOperationType::LogicAnd:
          return CreateConstExp(val1 && val2);
        case TACOperationType::LogicOr:
          return CreateConstExp(val1 || val2);
        default:
          throw UnknownException(*plocation_, std::string(magic_enum::enum_name(arith_op)));
          return nullptr;
      }
    }
  }

  switch (arith_op) {
    case TACOperationType::UnaryPositive:
    case TACOperationType::UnaryMinus: {
      auto tmpSym = CreateTempVariable(exp1->ret->value_.UnderlyingType());
      auto tac_list = TACFactory::Instance()->NewTACList();
      (*tac_list) += exp1->tac;
      //如果是Array的话要单独抽出来
      if (exp1->ret->value_.Type() == SymbolValue::ValueType::Array) {
        auto tmpExpSym = CreateTempVariable(exp1->ret->value_.UnderlyingType());
        (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpExpSym);
        (*tac_list) += CreateAssign(tmpExpSym, exp1)->tac;
        exp1->ret = tmpExpSym;
      }
      (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpSym);
      (*tac_list) += TACFactory::Instance()->NewTAC(arith_op, tmpSym, exp1->ret);
      return TACFactory::Instance()->NewExp(tac_list, tmpSym);
    }
    case TACOperationType::UnaryNot: {
      auto tmpSym = CreateTempVariable(SymbolValue::ValueType::Int);
      auto tac_list = TACFactory::Instance()->NewTACList();
      //如果是Array的话要单独抽出来
      if (exp1->ret->value_.Type() == SymbolValue::ValueType::Array) {
        auto tmpExpSym = CreateTempVariable(exp1->ret->value_.UnderlyingType());
        (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpExpSym);
        (*tac_list) += CreateAssign(tmpExpSym, exp1)->tac;
        exp1->ret = tmpExpSym;
      } else {
        (*tac_list) += exp1->tac;
      }
      (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpSym);
      (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::UnaryNot, tmpSym, exp1->ret);
      return TACFactory::Instance()->NewExp(tac_list, tmpSym);
    }
    case TACOperationType::Add:
    case TACOperationType::Sub:
    case TACOperationType::Mul:
    case TACOperationType::Div:
    case TACOperationType::Mod:
    case TACOperationType::LessThan:
    case TACOperationType::LessOrEqual:
    case TACOperationType::GreaterThan:
    case TACOperationType::GreaterOrEqual:
    case TACOperationType::Equal:
    case TACOperationType::NotEqual:
    case TACOperationType::LogicAnd:
    case TACOperationType::LogicOr: {
      auto tac_list = TACFactory::Instance()->NewTACList();
      if (exp1->ret->value_.UnderlyingType() == SymbolValue::ValueType::Float ||
          exp2->ret->value_.UnderlyingType() == SymbolValue::ValueType::Float) {
        auto tmpSym = CreateTempVariable(SymbolValue::ValueType::Float);
        (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpSym);
        auto fexp1 = exp1;
        auto fexp2 = exp2;
        if (fexp1->ret->value_.UnderlyingType() != SymbolValue::ValueType::Float) {
          fexp1 = CastIntToFloat(fexp1);
        }
        if (fexp2->ret->value_.UnderlyingType() != SymbolValue::ValueType::Float) {
          fexp2 = CastIntToFloat(fexp2);
        }
        //如果是Array的话要单独抽出来
        if (fexp1->ret->value_.Type() == SymbolValue::ValueType::Array) {
          auto tmpExpSym = CreateTempVariable(fexp1->ret->value_.UnderlyingType());
          (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpExpSym);
          (*tac_list) += CreateAssign(tmpExpSym, fexp1)->tac;
          fexp1->ret = tmpExpSym;
        } else {
          (*tac_list) += fexp1->tac;
        }
        if (fexp2->ret->value_.Type() == SymbolValue::ValueType::Array) {
          auto tmpExpSym = CreateTempVariable(fexp2->ret->value_.UnderlyingType());
          (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpExpSym);
          (*tac_list) += CreateAssign(tmpExpSym, fexp2)->tac;
          fexp2->ret = tmpExpSym;
        } else {
          (*tac_list) += fexp2->tac;
        }
        (*tac_list) += TACFactory::Instance()->NewTAC(arith_op, tmpSym, fexp1->ret, fexp2->ret);
        return TACFactory::Instance()->NewExp(tac_list, tmpSym);
      } else {
        auto tmpSym = CreateTempVariable(SymbolValue::ValueType::Int);
        //如果是Array的话要单独抽出来
        if (exp1->ret->value_.Type() == SymbolValue::ValueType::Array) {
          auto tmpExpSym = CreateTempVariable(exp1->ret->value_.UnderlyingType());
          (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpExpSym);
          (*tac_list) += CreateAssign(tmpExpSym, exp1)->tac;
          exp1->ret = tmpExpSym;
        } else {
          (*tac_list) += exp1->tac;
        }
        if (exp2->ret->value_.Type() == SymbolValue::ValueType::Array) {
          auto tmpExpSym = CreateTempVariable(exp2->ret->value_.UnderlyingType());
          (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpExpSym);
          (*tac_list) += CreateAssign(tmpExpSym, exp2)->tac;
          exp2->ret = tmpExpSym;
        } else {
          (*tac_list) += exp2->tac;
        }
        (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpSym);
        (*tac_list) += TACFactory::Instance()->NewTAC(arith_op, tmpSym, exp1->ret, exp2->ret);
        return TACFactory::Instance()->NewExp(tac_list, tmpSym);
      }
    }
    default:
      break;
  }
  throw UnknownException(*plocation_,
                         "Not TAC operation, type is" + std::string(magic_enum::enum_name<TACOperationType>(arith_op)));
}

void TACBuilder::SetTACList(TACListPtr tac_list) { tac_list_ = tac_list; }

void TACBuilder::SetLocation(HaveFunCompiler::Parser::location *plocation) { plocation_ = plocation; }

TACListPtr TACBuilder::GetTACList() { return tac_list_; }

SymbolPtr TACRebuilder::NewSymbol(SymbolType type, std::optional<std::string> name, SymbolValue value, int offset) {
  return TACFactory::Instance()->NewSymbol(type, name, value, offset);
}
ThreeAddressCodePtr TACRebuilder::NewTAC(TACOperationType operation, SymbolPtr a, SymbolPtr b, SymbolPtr c) {
  return TACFactory::Instance()->NewTAC(operation, a, b, c);
}

void TACRebuilder::InsertSymbol(std::string name, SymbolPtr sym) { symbol_table_[name] = sym; }

SymbolPtr TACRebuilder::FindSymbol(std::string name) {
  auto it = symbol_table_.find(name);
  if (it == symbol_table_.end()) {
    return nullptr;
  }
  return it->second;
}

//没有记录名字，记得给他设置名字，如果有的话。
SymbolPtr TACRebuilder::CreateArray(SymbolValue::ValueType type, int size, bool is_const,
                                    std::optional<std::string> name) {
  auto arrayDescriptor = TACFactory::Instance()->NewArrayDescriptor();
  arrayDescriptor->dimensions.push_back(size);
  arrayDescriptor->value_type = type;
  arrayDescriptor->base_offset = CreateConstSym(0);
  auto array_sym =
      NewSymbol(is_const ? SymbolType::Constant : SymbolType::Variable, name, SymbolValue(arrayDescriptor));
  arrayDescriptor->base_addr = array_sym;
  return array_sym;
}
//返回array[index]
SymbolPtr TACRebuilder::AccessArray(SymbolPtr array, SymbolPtr index) {
  auto oriArrayDescriptor = array->value_.GetArrayDescriptor();
  assert(oriArrayDescriptor != nullptr);
  assert(oriArrayDescriptor->dimensions.size() == 1);
  auto arrayDescriptor = TACFactory::Instance()->NewArrayDescriptor();
  arrayDescriptor->dimensions.push_back(oriArrayDescriptor->dimensions[0]);
  arrayDescriptor->value_type = oriArrayDescriptor->value_type;
  arrayDescriptor->base_offset = index;
  arrayDescriptor->base_addr = oriArrayDescriptor->base_addr;
  return NewSymbol(array->type_, std::nullopt, SymbolValue(arrayDescriptor));
}

void TACRebuilder::SetTACList(TACListPtr tac_list) { tac_list_ = tac_list; }

TACListPtr TACRebuilder::GetTACList() { return tac_list_; }

void TACRebuilder::SetLocation(HaveFunCompiler::Parser::location *plocation) { plocation_ = plocation; }

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler