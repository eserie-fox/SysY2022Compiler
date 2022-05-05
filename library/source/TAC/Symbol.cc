#include "TAC/Symbol.hh"
#include <cassert>
#include <exception>
#include <string>

namespace HaveFunCompiler {
namespace ThreeAddressCode {

SymbolValue::SymbolValue() { type = ValueType::Void; }

SymbolValue::SymbolValue(const SymbolValue &other) : type(other.type), value(other.value) {}

SymbolValue::SymbolValue(float v) {
  value = v;
  type = ValueType::Float;
}
SymbolValue::SymbolValue(int v) {
  value = v;
  type = ValueType::Int;
}
SymbolValue::SymbolValue(const char *v) {
  value = v;
  type = ValueType::Str;
}
SymbolValue::SymbolValue(std::shared_ptr<ParameterList> v) {
  value = v;
  type = ValueType::Parameters;
}
float SymbolValue::GetFloat() const {
  if (type != ValueType::Float) {
    throw std::runtime_error("SymbolValue::GetFloat");
  }
  return std::get<float>(value);
}
int SymbolValue::GetInt() const {
  if (type != ValueType::Int) {
    throw std::runtime_error("Type mismatch in SymbolValue::GetInt");
  }
  return std::get<int>(value);
}
const char *SymbolValue::GetStr() const {
  if (type != ValueType::Str) {
    throw std::runtime_error("Type mismatch in SymbolValue::GetStr");
  }
  return std::get<const char *>(value);
}
std::shared_ptr<ParameterList> SymbolValue::GetParameters() const {
  if (type != ValueType::Parameters) {
    throw std::runtime_error("Type mismatch in SymbolValue::GetParameters");
  }
  return std::get<std::shared_ptr<ParameterList>>(value);
}

SymbolValue &SymbolValue::operator=(const SymbolValue &other) {
  type = other.type;
  value = other.value;
  return *this;
}

bool SymbolValue::IsNumericType() const {
  if (type == ValueType::Float || type == ValueType::Int) {
    return true;
  }
  return false;
}
SymbolValue SymbolValue::operator+(const SymbolValue &other) const {
  if (!IsNumericType() || !other.IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type) + ", " +
                             std::to_string((int)other.type));
  }
  if (type == ValueType::Float || other.type == ValueType::Float) {
    float res;
    if (type == ValueType::Float) {
      res = GetFloat();
    } else {
      res = static_cast<float>(GetInt());
    }
    if (other.type == ValueType::Float) {
      res += other.GetFloat();
    } else {
      res += static_cast<float>(other.GetInt());
    }
    return SymbolValue(res);
  }
  return SymbolValue(GetInt() + other.GetInt());
}
SymbolValue SymbolValue::operator-(const SymbolValue &other) const { return {}; }
SymbolValue SymbolValue::operator*(const SymbolValue &other) const { return {}; }
SymbolValue SymbolValue::operator/(const SymbolValue &other) const { return {}; }
SymbolValue SymbolValue::operator%(const SymbolValue &other) const { return {}; }
SymbolValue SymbolValue::operator!() const { return SymbolValue((int)(!this->operator bool())); }
SymbolValue SymbolValue::operator+() const { return *this; }
SymbolValue SymbolValue::operator-() const { return {}; }
SymbolValue SymbolValue::operator<(const SymbolValue &other) const { return {}; }
SymbolValue SymbolValue::operator>(const SymbolValue &other) const { return other < *this; }
SymbolValue SymbolValue::operator<=(const SymbolValue &other) const { return !(other < *this); }
SymbolValue SymbolValue::operator>=(const SymbolValue &other) const { return !(*this < other); }
SymbolValue SymbolValue::operator==(const SymbolValue &other) const { return {}; };
SymbolValue SymbolValue::operator!=(const SymbolValue &other) const { return !(*this == other); }
SymbolValue SymbolValue::operator&&(const SymbolValue &other) const { return {}; }
SymbolValue SymbolValue::operator||(const SymbolValue &other) const { return {}; }

SymbolValue::operator bool() const {
  if (type == ValueType::Void) {
    throw std::runtime_error("Void type can't be converted to bool!");
  }
  if (type == ValueType::Int) {
    return GetInt() != 0;
  }
  if (type == ValueType::Float) {
    return GetFloat() != 0;
  }
  if (type == ValueType::Parameters) {
    return GetParameters() != nullptr;
  }
  if (type == ValueType::Str) {
    return GetStr() != nullptr;
  }
  return false;
}

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler