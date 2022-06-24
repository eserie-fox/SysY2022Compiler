#include "TAC/ThreeAddressCode.hh"
#include <algorithm>
#include <unordered_map>
#include "MagicEnum.hh"
#include "TAC/Symbol.hh"

namespace HaveFunCompiler {
namespace ThreeAddressCode {

std::string ThreeAddressCode::ToString() const {
  std::unordered_map<TACOperationType, std::string> OpStr = {
      {TACOperationType::Add, "+"},
      {TACOperationType::Div, "/"},
      {TACOperationType::Equal, "=="},
      {TACOperationType::GreaterOrEqual, ">="},
      {TACOperationType::GreaterThan, ">"},
      {TACOperationType::LessOrEqual, "<="},
      {TACOperationType::LessThan, "<"},
      {TACOperationType::LogicAnd, "&&"},
      {TACOperationType::LogicOr, "||"},
      {TACOperationType::Mod, "%"},
      {TACOperationType::Mul, "*"},
      {TACOperationType::NotEqual, "!="},
      {TACOperationType::Sub, "-"},
      {TACOperationType::UnaryMinus, "-"},
      {TACOperationType::UnaryNot, "!"},
      {TACOperationType::UnaryPositive, "+"},
      {TACOperationType::FloatToInt, "(int)"},
      {TACOperationType::IntToFloat, "(float)"},
  };
  switch (operation_) {
    case TACOperationType::Add:
    case TACOperationType::Sub:
    case TACOperationType::Mul:
    case TACOperationType::Div:
    case TACOperationType::Mod:
    case TACOperationType::Equal:
    case TACOperationType::NotEqual:
    case TACOperationType::GreaterThan:
    case TACOperationType::GreaterOrEqual:
    case TACOperationType::LessThan:
    case TACOperationType::LessOrEqual:
    case TACOperationType::LogicAnd:
    case TACOperationType::LogicOr:
      return a_->get_tac_name() + " = " + b_->get_tac_name() + " " + OpStr[operation_] + " " + c_->get_tac_name();
    case TACOperationType::UnaryMinus:
    case TACOperationType::UnaryNot:
    case TACOperationType::UnaryPositive:
    case TACOperationType::FloatToInt:
    case TACOperationType::IntToFloat:
      return a_->get_tac_name() + " = " + OpStr[operation_] + b_->get_tac_name();
    case TACOperationType::Argument: {
      std::string ret = "arg ";
      bool is_array = a_->value_.Type() == SymbolValue::ValueType::Array;
      if (is_array) {
        if (!a_->value_.GetArrayDescriptor()->dimensions.empty()) {
          ret += "& ";
        }
      }
      ret += a_->get_tac_name();
      return ret;
      // return "Argument(" + std::string(magic_enum::enum_name<SymbolValue::ValueType>(a_->value_.Type())) +
      //        "): " + a_->get_name();
    }
    case TACOperationType::Assign:
      return a_->get_tac_name() + " = " + b_->get_tac_name();
    case TACOperationType::Call:
      return (a_ == nullptr ? "" : (a_->get_tac_name(true) + " = ")) + "call " + b_->get_tac_name(true);
    case TACOperationType::FunctionBegin: {
      return "fbegin";
      // return "FuncBegin";
    }
    case TACOperationType::FunctionEnd: {
      return "fend";
      // return "FuncEnd";
    }
    case TACOperationType::Goto:
      return "goto " + a_->get_tac_name();
    case TACOperationType::IfZero:
      return "ifz " + b_->get_tac_name() + " goto " + a_->get_tac_name();
    case TACOperationType::Label: {
      return "label " + a_->get_tac_name();
      // return a_->get_name() + ":";
    }
    case TACOperationType::Parameter: {
      std::string ret = "param ";
      if (a_->value_.Type() == SymbolValue::ValueType::Array) {
        ret += std::string(magic_enum::enum_name<SymbolValue::ValueType>(a_->value_.GetArrayDescriptor()->value_type)) +
               "[] ";
      } else {
        ret += std::string(magic_enum::enum_name<SymbolValue::ValueType>(a_->value_.Type())) + " ";
      }
      std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
      ret += a_->get_tac_name(true);
      return ret;
      // return "Parameter(" + std::string(magic_enum::enum_name<SymbolValue::ValueType>(a_->value_.Type())) +
      //        "): " + a_->get_name();
    }

    case TACOperationType::Return:
      return "ret" + (a_ == nullptr ? "" : (std::string(" ") + a_->get_tac_name()));
    case TACOperationType::Variable:
      return a_->value_.TypeToTACString() + " " + a_->get_tac_name(true);
    case TACOperationType::Constant:
      return "const " + a_->value_.TypeToTACString() + " " + a_->get_tac_name(true);
    case TACOperationType::BlockBegin:
      return "bbegin";
    case TACOperationType::BlockEnd:
      return "bend";
    case TACOperationType::ArgumentAddress:
      return "arg & " + a_->get_tac_name();
    default:
      return "Undefined";
  }
}

ThreeAddressCodeList::ThreeAddressCodeList(ThreeAddressCodeList &&move_obj) { list_.swap(move_obj.list_); }

ThreeAddressCodeList::ThreeAddressCodeList(const ThreeAddressCodeList &other) : list_(other.list_) {}

ThreeAddressCodeList::ThreeAddressCodeList(std::shared_ptr<ThreeAddressCodeList> other_ptr) {
  if (other_ptr != nullptr) {
    list_ = other_ptr->list_;
  }
}
ThreeAddressCodeList::ThreeAddressCodeList(std::shared_ptr<ThreeAddressCode> tac) { list_.push_back(tac); }

ThreeAddressCodeList &ThreeAddressCodeList::operator=(const ThreeAddressCodeList &other) {
  list_ = other.list_;
  return *this;
}

ThreeAddressCodeList ThreeAddressCodeList::operator+(const ThreeAddressCodeList &other) const {
  ThreeAddressCodeList ret;
  ret.list_ = list_;
  ret.list_.insert(ret.list_.end(), other.list_.cbegin(), other.list_.cend());
  return ret;
}
ThreeAddressCodeList &ThreeAddressCodeList::operator+=(const ThreeAddressCodeList &other) {
  list_.insert(list_.end(), other.list_.cbegin(), other.list_.cend());
  return *this;
}

ThreeAddressCodeList &ThreeAddressCodeList::operator+=(std::shared_ptr<ThreeAddressCodeList> other) {
  if (other == nullptr) {
    return *this;
  }
  return (*this += *other);
}

ThreeAddressCodeList &ThreeAddressCodeList::operator+=(std::shared_ptr<ThreeAddressCode> other) {
  list_.push_back(other);
  return *this;
}

std::shared_ptr<ThreeAddressCodeList> ThreeAddressCodeList::MakeCopy() const {
  std::shared_ptr<ThreeAddressCodeList> tac_list = std::make_shared<ThreeAddressCodeList>();
  tac_list->list_ = list_;
  return tac_list;
}

std::string ThreeAddressCodeList::ToString() const {
  ThreeAddressCodeList func_list;
  ThreeAddressCodeList glob_list;
  int infunc = 0;
  for (auto it = list_.begin(); it != list_.end(); it++) {
    if ((*it)->operation_ == TACOperationType::Label) {
      auto nxt = it;
      ++nxt;
      if (nxt != list_.end()) {
        if ((*nxt)->operation_ == TACOperationType::FunctionBegin) {
          func_list += (*it);
          func_list += (*nxt);
          infunc++;
          it = nxt;
          continue;
        }
      }
    }
    if ((*it)->operation_ == TACOperationType::FunctionEnd) {
      func_list += (*it);
      infunc--;
      continue;
    }
    if (infunc) {
      func_list += (*it);
    } else {
      glob_list += (*it);
    }
  }

  std::string ret;
  for (const auto &tac : glob_list) {
    ret += tac->ToString() + "\n";
  }
  for (const auto &tac : func_list) {
    ret += tac->ToString() + "\n";
  }
  return ret;
}

std::shared_ptr<Symbol> ThreeAddressCode::getDefineSym()
{
  auto isArith = [](TACOperationType op)
  {
    if (op < TACOperationType::Assign)  return true;
    return false;
  };

  auto isDecl = [](TACOperationType op)
  {
    switch (op)
    {
    case TACOperationType::Variable:
    case TACOperationType::Constant:
    case TACOperationType::Parameter:
      return true;
      break;
    
    default:
      return false;
      break;
    }
  };

  std::shared_ptr<Symbol> res;

  if (isArith(operation_) || isDecl(operation_))
  {
    if (a_)
      res = a_;
  }
  else
  {
    switch (operation_)
    {
    case TACOperationType::FloatToInt:
    case TACOperationType::IntToFloat:
      if (a_)
        res = a_;
      break;

    case TACOperationType::Call:
      if (a_)
        res = a_;
      break;

    case TACOperationType::Assign:
      if(a_ && a_->value_.IsNumericType())
        res = a_;
      break;

    default:
      break;
    }
  }
  return res;
}

std::list<std::shared_ptr<Symbol>> ThreeAddressCode::getUseSym()
{
  auto isBinaryArith = [](TACOperationType op)
  {
    if (op <= TACOperationType::LogicOr)  return true;
    return false;
  };

  auto isUnaryArith = [](TACOperationType op)
  {
    if (op > TACOperationType::LogicOr && op <= TACOperationType::UnaryPositive)  return true;
    return false;
  };

  if (isBinaryArith(operation_))
    return {b_, c_};
  else if (isUnaryArith(operation_))
    return {b_};
  else
  {
    switch (operation_)
    {
    case TACOperationType::Argument:
    case TACOperationType::ArgumentAddress:
      if (a_->value_.Type() == SymbolValue::ValueType::Array)
        return {a_->value_.GetArrayDescriptor()->base_addr.lock(), a_->value_.GetArrayDescriptor()->base_offset};
      else
        return {a_};
      break;

    case TACOperationType::FloatToInt:
    case TACOperationType::IntToFloat:
      return {b_};
      break;
    
    case TACOperationType::Assign:
      if (b_->value_.IsNumericType())
        return {b_};
      else
        return {};
      break;
    
    case TACOperationType::Return:
      if (a_)
        return {a_};
      else
        return {};

    default:
      return {};
      break;
    }
  }
}

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler