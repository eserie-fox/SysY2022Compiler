#include "TAC/TAC.hh"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "MagicEnum.hh"

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
  ret->subarray = std::make_shared<std::unordered_map<size_t, std::shared_ptr<Symbol>>>();
  return ret;
}

TACListPtr TACFactory::MakeFunction(SymbolPtr func_label, ParamListPtr params, TACListPtr body) {
  func_label->value_ = SymbolValue(params);
  auto tac_list = NewTACList();
  (*tac_list) += NewTAC(TACOperationType::Label, func_label);
  (*tac_list) += NewTAC(TACOperationType::FunctionBegin);
  for (auto sym : *params) {
    (*tac_list) += NewTAC(TACOperationType::Parameter, sym);
  }
  (*tac_list) += body;
  (*tac_list) += NewTAC(TACOperationType::FunctionEnd);
  return tac_list;
}

ExpressionPtr TACFactory::MakeAssign(SymbolPtr var, ExpressionPtr exp) {
  auto tac_list = exp->tac->MakeCopy();
  (*tac_list) += NewTAC(TACOperationType::Assign, var, exp->ret);
  return NewExp(tac_list, var);
}

SymbolPtr TACFactory::AccessArray(SymbolPtr array, std::vector<size_t> pos) {
  auto arrayDescriptor = array->value_.GetArrayDescriptor();
  assert(pos.size() <= arrayDescriptor->dimensions.size());
  if (pos.empty()) {
    return array;
  }
  size_t idx = pos.front();
  pos.erase(pos.cbegin());
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
  nArrayDescriptor->base_offset = arrayDescriptor->base_offset;
  nArrayDescriptor->base_offset += idx * size_sublen;
  arrayDescriptor->subarray->emplace(idx, NewSymbol(array->type_, std::nullopt, 0, SymbolValue(nArrayDescriptor)));
  return AccessArray(NewSymbol(array->type_, std::nullopt, 0, nArrayDescriptor), pos);
}

void TACFactory::FlattenInitArrayImpl(FlattenedArray *out_result, ArrayDescriptorPtr array) {
  out_result->emplace_back(1, nullptr);
  std::vector<std::pair<size_t, SymbolPtr>> subarray;
  for (const auto &[idx, valPtr] : *array->subarray) {
    subarray.emplace_back(idx, valPtr);
  }
  std::sort(subarray.begin(), subarray.end());
  for (const auto &[idx, valPtr] : subarray) {
    if (valPtr->value_.Type() != SymbolValue::ValueType::Array) {
      out_result->emplace_back(0, valPtr);
    } else {
      FlattenInitArrayImpl(out_result, valPtr->value_.GetArrayDescriptor());
    }
  }
  out_result->emplace_back(2, nullptr);
}

TACFactory::FlattenedArray TACFactory::FlattenInitArray(ArrayDescriptorPtr array) {
  FlattenedArray ret;
  FlattenInitArrayImpl(&ret, array);
  return ret;
}

int TACFactory::ArrayInitImpl(SymbolPtr array, FlattenedArray::iterator &it, const FlattenedArray::iterator &end) {
  enum { OK, IGNORE };
  if (it == end) {
    return IGNORE;
  }
  auto arrayDescriptor = array->value_.GetArrayDescriptor();
  if (arrayDescriptor->dimensions.empty()) {
    if (it->first == 1) {
      throw std::runtime_error("Extra left brace in array init");
    }
    if (it->first == 2) {
      return IGNORE;
    }
    arrayDescriptor->subarray->emplace(0, it->second);
    ++it;
    return OK;
  }
  for (size_t i = 0; i < arrayDescriptor->dimensions[0]; i++) {
    if (it->first == 1) {
      ++it;
      ArrayInitImpl(TACFactory::Instance()->AccessArray(array, {i}), it, end);
      if (it->first != 2) {
        throw std::runtime_error("Too many elements in sub array");
      }
      ++it;
      continue;
    }
    if (it->first == 2) {
      return IGNORE;
    }
    if (IGNORE == ArrayInitImpl(TACFactory::Instance()->AccessArray(array, {i}), it, end)) {
      return IGNORE;
    }
  }
  return OK;
}

TACListPtr TACFactory::MakeArrayInit(SymbolPtr array, SymbolPtr init_array) {
  if (array->type_ != SymbolType::Variable && array->type_ != SymbolType::Constant) {
    throw std::runtime_error("Error symbol type(" + std::string(magic_enum::enum_name<SymbolType>(array->type_)) +
                             ") , should be variable or constant");
  }
  if (init_array->type_ != SymbolType::Variable && init_array->type_ != SymbolType::Constant) {
    throw std::runtime_error("Error symbol type(" + std::string(magic_enum::enum_name<SymbolType>(init_array->type_)) +
                             ") , should be variable or constant");
  }
  if (array->value_.Type() != SymbolValue::ValueType::Array &&
      init_array->value_.Type() != SymbolValue::ValueType::Array) {
    throw std::runtime_error("Should be array");
  }

  auto finit_array = FlattenInitArray(init_array->value_.GetArrayDescriptor());
  for (auto &pr : finit_array) {
    if (pr.second) {
      if (array->type_ == SymbolType::Constant && pr.second->type_ != SymbolType::Constant) {
        throw std::runtime_error("Only constant allowed in const array");
      }
      if (pr.second->type_ != SymbolType::Constant && pr.second->type_ != SymbolType::Variable) {
        throw std::runtime_error("Only variable or constant allowed in array init");
      }
    }
  }
  if (array->value_.GetArrayDescriptor()->value_type == SymbolValue::ValueType::Int) {
    for (auto &pr : finit_array) {
      if (pr.second != nullptr && pr.second->value_.Type() == SymbolValue::ValueType::Float) {
        throw std::runtime_error("Only int can be used to initialize int array");
      }
    }
  }
  auto init_array_it = finit_array.begin();
  ++init_array_it;
  ArrayInitImpl(array, init_array_it, finit_array.end());
  return NewTACList(NewTAC(TACOperationType::Variable, array));
}

TACListPtr TACFactory::MakeCall(SymbolPtr func_label, ArgListPtr args) {
  auto tac_list = NewTACList();
  for (auto exp : *args) {
    (*tac_list) += exp->tac;
  }
  for (auto exp : *args) {
    (*tac_list) += NewTAC(TACOperationType::Argument, exp->ret);
  }
  (*tac_list) += NewTAC(TACOperationType::Call, nullptr, func_label);
  return tac_list;
}

TACListPtr TACFactory::MakeCallWithRet(SymbolPtr func_label, ArgListPtr args, SymbolPtr ret_sym) {
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
TACListPtr TACFactory::MakeFor(TACListPtr init, ExpressionPtr cond, TACListPtr modify, SymbolPtr label_cont,
                               SymbolPtr label_brk, TACListPtr stmt) {
  return NewTACList(*init + *MakeWhile(cond, label_cont, label_brk, NewTACList(*stmt + *modify)));
}

std::string TACFactory::ToFuncLabelName(const std::string name) { return "U_" + name; }
std::string TACFactory::ToCustomerLabelName(const std::string name) { return "UL_" + name; }
std::string TACFactory::ToTempLabelName(uint64_t id) { return "SL_" + std::to_string(id); }
std::string TACFactory::ToVariableOrConstantName(const std::string name) { return "U_" + name; }
std::string TACFactory::ToTempVariableName(uint64_t id) { return "SV_" + std::to_string(id); }

TACBuilder::TACBuilder() : cur_temp_var_(0), cur_temp_label_(0), cur_symtab_id_(0) { EnterSubscope(); }

SymbolPtr TACBuilder::NewSymbol(SymbolType type, std::optional<std::string> name, int offset, SymbolValue value) {
  return TACFactory::Instance()->NewSymbol(type, name, offset, value);
}
ThreeAddressCodePtr TACBuilder::NewTAC(TACOperationType operation, SymbolPtr a, SymbolPtr b, SymbolPtr c) {
  return TACFactory::Instance()->NewTAC(operation, a, b, c);
}
ExpressionPtr TACBuilder::NewExp(TACListPtr tac, SymbolPtr ret) { return TACFactory::Instance()->NewExp(tac, ret); }

ArgListPtr TACBuilder::NewArgList() { return TACFactory::Instance()->NewArgList(); }

ParamListPtr TACBuilder::NewParamList() { return TACFactory::Instance()->NewParamList(); }

ArrayDescriptorPtr TACBuilder::NewArrayDescriptor() { return TACFactory::Instance()->NewArrayDescriptor(); }

ExpressionPtr TACBuilder::CreateAssign(SymbolPtr var, ExpressionPtr exp) {
  return TACFactory::Instance()->MakeAssign(var, exp);
}

SymbolPtr TACBuilder::AccessArray(SymbolPtr array, std::vector<size_t> pos) {
  return TACFactory::Instance()->AccessArray(array, pos);
}
TACListPtr TACBuilder::MakeArrayInit(SymbolPtr array, SymbolPtr init_array) {
  return TACFactory::Instance()->MakeArrayInit(array, init_array);
}

void TACBuilder::EnterSubscope() { symbol_stack_.emplace_back(cur_symtab_id_++); }

void TACBuilder::ExitSubscope() { symbol_stack_.pop_back(); }

ExpressionPtr TACBuilder::CreateConstExp(int n) { return CreateConstExp(SymbolValue(n)); }
ExpressionPtr TACBuilder::CreateConstExp(float fn) { return CreateConstExp(SymbolValue(fn)); }

ExpressionPtr TACBuilder::CreateConstExp(SymbolValue v) {
  return TACFactory::Instance()->NewExp(nullptr,
                                        TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, v));
}

SymbolPtr TACBuilder::CreateText(const std::string &text) {
  if (auto it = text_tab_.find(text); it != text_tab_.end()) {
    return it->second;
  }
  stored_text_.push_back(text);

  auto ret =
      TACFactory::Instance()->NewSymbol(SymbolType::Text, std::nullopt, 0, SymbolValue(stored_text_.back().c_str()));
  text_tab_[text] = ret;
  return ret;
}
void TACBuilder::Error(const std::string &message) {
  std::cerr << message << std::endl;
  // exit(EXIT_FAILURE);
}
std::string TACBuilder::AppendScopePrefix(const std::string &name) {
  return std::string("S") + std::to_string(symbol_stack_.back().get_tabel_id()) + name;
}

SymbolPtr TACBuilder::CreateFunctionLabel(const std::string &name) {
  std::string label_name = AppendScopePrefix(TACFactory::Instance()->ToFuncLabelName(name));
  if (symbol_stack_.back().count(label_name)) {
    Error("Already exist identifier named '" + name + "' in current scope!");
    return nullptr;
  }
  auto sym = TACFactory::Instance()->NewSymbol(SymbolType::Function, label_name);
  symbol_stack_.back()[label_name] = sym;
  return sym;
}

SymbolPtr TACBuilder::CreateCustomerLabel(const std::string &name) {
  std::string label_name = AppendScopePrefix(TACFactory::Instance()->ToCustomerLabelName(name));
  if (symbol_stack_.back().count(label_name)) {
    Error("Already exist label named '" + name + "' in current scope!");
    return nullptr;
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
    Error("Already exist identifier named '" + name + "' in current scope!");
    return nullptr;
  }
  auto sym = TACFactory::Instance()->NewSymbol(SymbolType::Variable, var_name);
  sym->value_.SetType(type);
  symbol_stack_.back()[var_name] = sym;
  return sym;
}
SymbolPtr TACBuilder::CreateTempVariable(SymbolValue::ValueType type) {
  std::string var_name = AppendScopePrefix(TACFactory::Instance()->ToTempVariableName(cur_temp_var_++));
  auto sym = TACFactory::Instance()->NewSymbol(SymbolType::Variable, var_name);
  sym->value_.SetType(type);
  symbol_stack_.back()[var_name] = sym;
  return sym;
}

TACListPtr TACBuilder::CreateFunction(const std::string &name, ParamListPtr params, TACListPtr body) {
  auto func_label = CreateFunctionLabel(name);
  return TACFactory::Instance()->MakeFunction(func_label, params, body);
}

SymbolPtr TACBuilder::FindSymbolWithName(const std::string &name) {
  for (auto s_it = symbol_stack_.rbegin(); s_it != symbol_stack_.rend(); ++s_it) {
    if (auto it = s_it->find(name); it != s_it->end()) {
      return it->second;
    }
  }
  return nullptr;
}

TACListPtr TACBuilder::CreateCall(const std::string &func_name, ArgListPtr args) {
  auto func_label = FindFunctionLabel(func_name);
  if (func_label == nullptr) {
    Error("Function with name '" + func_name + "' is not found!");
    return nullptr;
  }
  return TACFactory::Instance()->MakeCall(func_label, args);
}
TACListPtr TACBuilder::CreateCallWithRet(const std::string &func_name, ArgListPtr args, SymbolPtr ret_sym) {
  auto func_label = FindFunctionLabel(func_name);
  if (func_label == nullptr) {
    Error("Function with name '" + func_name + "' is not found!");
    return nullptr;
  }
  return TACFactory::Instance()->MakeCallWithRet(func_label, args, ret_sym);
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
TACListPtr TACBuilder::CreateWhile(ExpressionPtr cond, TACListPtr stmt, SymbolPtr *out_label_cont,
                                   SymbolPtr *out_label_brk) {
  auto label_cont = CreateTempLabel();
  auto label_brk = CreateTempLabel();
  if (out_label_cont) {
    *out_label_cont = label_cont;
  }
  if (out_label_brk) {
    *out_label_brk = label_brk;
  }
  return TACFactory::Instance()->MakeWhile(cond, label_cont, label_brk, stmt);
}
TACListPtr TACBuilder::CreateFor(TACListPtr init, ExpressionPtr cond, TACListPtr modify, TACListPtr stmt,
                                 SymbolPtr *out_label_cont, SymbolPtr *out_label_brk) {
  auto label_cont = CreateTempLabel();
  auto label_brk = CreateTempLabel();
  if (out_label_cont) {
    *out_label_cont = label_cont;
  }
  if (out_label_brk) {
    *out_label_brk = label_brk;
  }
  return TACFactory::Instance()->MakeFor(init, cond, modify, label_cont, label_brk, stmt);
}

bool TACBuilder::BindConstName(const std::string &name, SymbolPtr constant) {
  std::string const_name = AppendScopePrefix(TACFactory::Instance()->ToVariableOrConstantName(name));
  if (symbol_stack_.back().count(const_name)) {
    Error("Already exist identifier named '" + name + "' in current scope!");
    return false;
  }
  symbol_stack_.back()[const_name] = constant;
  return true;
}

SymbolPtr TACBuilder::FindVariableOrConstant(const std::string &name) {
  std::string var_name = AppendScopePrefix(TACFactory::Instance()->ToVariableOrConstantName(name));
  return FindSymbolWithName(var_name);
}
SymbolPtr TACBuilder::FindFunctionLabel(const std::string &name) {
  std::string func_name = AppendScopePrefix(TACFactory::Instance()->ToFuncLabelName(name));
  return FindSymbolWithName(func_name);
}
SymbolPtr TACBuilder::FindCustomerLabel(const std::string &name) {
  std::string label_name = AppendScopePrefix(TACFactory::Instance()->ToCustomerLabelName(name));
  return FindSymbolWithName(label_name);
}

ExpressionPtr TACBuilder::CastFloatToInt(ExpressionPtr expF) {
  if (expF->ret->type_ == SymbolType::Constant) {
    return CreateConstExp(static_cast<int>(expF->ret->value_.GetFloat()));
  }
  if (expF->ret->type_ == SymbolType::Variable) {
    auto tmpSym = CreateTempVariable(SymbolValue::ValueType::Int);
    auto tac_list = TACFactory::Instance()->NewTACList(expF->tac);
    (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpSym);
    (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::FloatToInt, tmpSym, expF->ret);
    return TACFactory::Instance()->NewExp(tac_list, tmpSym);
  }
  throw std::runtime_error("Expression has error type(" +
                           std::string(magic_enum::enum_name<SymbolType>(expF->ret->type_)) + ") to casting to int");
}
ExpressionPtr TACBuilder::CastIntToFloat(ExpressionPtr expI) {
  if (expI->ret->type_ == SymbolType::Constant) {
    return CreateConstExp(static_cast<float>(expI->ret->value_.GetInt()));
  }
  if (expI->ret->type_ == SymbolType::Variable) {
    auto tmpSym = CreateTempVariable(SymbolValue::ValueType::Float);
    auto tac_list = TACFactory::Instance()->NewTACList(expI->tac);
    (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpSym);
    (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::IntToFloat, tmpSym, expI->ret);
    return TACFactory::Instance()->NewExp(tac_list, tmpSym);
  }
  throw std::runtime_error("Expression has error type(" +
                           std::string(magic_enum::enum_name<SymbolType>(expI->ret->type_)) + ") to casting to float");
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
          Error("Unknown operation type : " + std::to_string((int)arith_op));
          return nullptr;
      }
    }
  }

  switch (arith_op) {
    case TACOperationType::UnaryPositive:
    case TACOperationType::UnaryMinus:
    case TACOperationType::UnaryNot: {
      auto tmpSym = CreateTempVariable(exp1->ret->value_.Type());
      auto tac_list = TACFactory::Instance()->NewTACList();
      (*tac_list) += exp1->tac;
      (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpSym);
      (*tac_list) += TACFactory::Instance()->NewTAC(arith_op, tmpSym, exp1->ret);
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
      (*tac_list) += exp1->tac;
      (*tac_list) += exp2->tac;
      if (exp1->ret->value_.Type() == SymbolValue::ValueType::Float ||
          exp2->ret->value_.Type() == SymbolValue::ValueType::Float) {
        auto tmpSym = CreateTempVariable(SymbolValue::ValueType::Float);
        (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpSym);
        auto fexp1 = exp1;
        auto fexp2 = exp2;
        if (fexp1->ret->value_.Type() != SymbolValue::ValueType::Float) {
          fexp1 = CastIntToFloat(fexp1);
        }
        if (fexp2->ret->value_.Type() != SymbolValue::ValueType::Float) {
          fexp2 = CastIntToFloat(fexp2);
        }
        (*tac_list) += TACFactory::Instance()->NewTAC(arith_op, tmpSym, fexp1->ret, fexp2->ret);
        return TACFactory::Instance()->NewExp(tac_list, tmpSym);
      } else {
        auto tmpSym = CreateTempVariable(SymbolValue::ValueType::Int);
        (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, tmpSym);
        (*tac_list) += TACFactory::Instance()->NewTAC(arith_op, tmpSym, exp1->ret, exp2->ret);
        return TACFactory::Instance()->NewExp(tac_list, tmpSym);
      }
    }
    default:
      break;
  }
  throw std::runtime_error("Not TAC operation type is" +
                           std::string(magic_enum::enum_name<TACOperationType>(arith_op)));
}

void TACBuilder::SetTACList(TACListPtr tac_list) { tac_list_ = tac_list; }

TACListPtr TACBuilder::GetTACList() { return tac_list_; }

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler