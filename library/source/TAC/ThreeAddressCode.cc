#include "TAC/ThreeAddressCode.hh"
#include <unordered_map>
#include "MagicEnum.hh"
#include "TAC/Symbol.hh"

namespace HaveFunCompiler {
namespace ThreeAddressCode {

std::string ThreeAddressCode::ToString() const {
  std::unordered_map<TACOperationType, std::string> OpStr = {
      {TACOperationType::Add, "+"},         {TACOperationType::Div, "-"},
      {TACOperationType::Equal, "=="},      {TACOperationType::GreaterOrEqual, ">="},
      {TACOperationType::GreaterThan, ">"}, {TACOperationType::LessOrEqual, "<="},
      {TACOperationType::LessThan, "<"},    {TACOperationType::LogicAnd, "&&"},
      {TACOperationType::LogicOr, "||"},    {TACOperationType::Mod, "%"},
      {TACOperationType::Mul, "*"},         {TACOperationType::NotEqual, "!="},
      {TACOperationType::Sub, "-"},         {TACOperationType::UnaryMinus, "-"},
      {TACOperationType::UnaryNot, "!"},    {TACOperationType::UnaryPositive, "+"},
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
      return a_->name_.value() + " = " + b_->name_.value() + " " + OpStr[operation_] + " " + c_->name_.value();
    case TACOperationType::UnaryMinus:
    case TACOperationType::UnaryNot:
    case TACOperationType::UnaryPositive:
      return a_->name_.value() + " = " + OpStr[operation_] + b_->name_.value();
    case TACOperationType::Argument:
      return "Argument(" + std::string(magic_enum::enum_name<SymbolValue::ValueType>(a_->value_.Type())) +
             "): " + a_->name_.value();
    case TACOperationType::Assign:
      return a_->name_.value() + " = " + b_->name_.value();
    case TACOperationType::Call:
      return (a_ == nullptr ? "" : (a_->name_.value() + " = ")) + "Call " + b_->name_.value();
    case TACOperationType::FunctionBegin:
      return "FuncBegin";
    case TACOperationType::FunctionEnd:
      return "FuncEnd";
    case TACOperationType::Goto:
      return "Goto " + a_->name_.value();
    case TACOperationType::IfZero:
      return "IfZero " + b_->name_.value() + " Goto " + a_->name_.value();
    case TACOperationType::Label:
      return a_->name_.value() + ":";
    case TACOperationType::Parameter:
      return "Parameter(" + std::string(magic_enum::enum_name<SymbolValue::ValueType>(a_->value_.Type())) +
             "): " + a_->name_.value();
    case TACOperationType::Return:
      return "Return" + (a_ == nullptr ? "" : (std::string(" ") + a_->name_.value()));
    case TACOperationType::Variable:
      return std::string(magic_enum::enum_name<SymbolValue::ValueType>(a_->value_.Type())) + ": " + a_->name_.value();
    default:
      return "Undefined";
  }
}

ThreeAddressCodeList::ThreeAddressCodeList(ThreeAddressCodeList &&move_obj) { list_.swap(move_obj.list_); }

ThreeAddressCodeList::ThreeAddressCodeList(const ThreeAddressCodeList &other) : list_(other.list_) {}

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