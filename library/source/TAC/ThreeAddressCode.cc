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
          ret += "&";
        }
      }
      ret += a_->get_tac_name(false);
      return ret;
      // return "Argument(" + std::string(magic_enum::enum_name<SymbolValue::ValueType>(a_->value_.Type())) +
      //        "): " + a_->get_name();
    }
    case TACOperationType::Assign:
      return a_->get_tac_name(false) + " = " + b_->get_tac_name(false);
    case TACOperationType::Call:
      return (a_ == nullptr ? "" : (a_->get_tac_name() + " = ")) + "call " + b_->get_tac_name();
    case TACOperationType::FunctionBegin: {
      return "fbegin";
      // return "FuncBegin";
    }
    case TACOperationType::FunctionEnd: {
      return "fend";
      // return "FuncEnd";
    }
    case TACOperationType::Goto:
      return "jump " + a_->get_tac_name();
    case TACOperationType::IfZero:
      return "ifz " + b_->get_tac_name() + " jump " + a_->get_tac_name();
    case TACOperationType::Label: {
      return "label " + a_->get_tac_name();
      // return a_->get_name() + ":";
    }
    case TACOperationType::Parameter: {
      std::string ret = "param " + std::string(magic_enum::enum_name<SymbolValue::ValueType>(a_->value_.Type())) + " ";
      std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
      ret += a_->name_.value_or("nullname");
      if (a_->value_.Type() == SymbolValue::ValueType::Array) {
        ret += "[]";
      }
      return ret;
      // return "Parameter(" + std::string(magic_enum::enum_name<SymbolValue::ValueType>(a_->value_.Type())) +
      //        "): " + a_->get_name();
    }

    case TACOperationType::Return:
      return "ret" + (a_ == nullptr ? "" : (std::string(" ") + a_->get_tac_name()));
    case TACOperationType::Variable:
      return a_->value_.TypeToTACString() + " " + a_->get_tac_name();
    case TACOperationType::Constant:
      return "const " + a_->value_.TypeToTACString() + " " + a_->get_tac_name();
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
  std::string ret;
  for (const auto &tac : list_) {
    ret += tac->ToString() + "\n";
  }
  return ret;
}
}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler