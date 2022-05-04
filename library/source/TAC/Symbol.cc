#include "TAC/Symbol.hh"
#include <cassert>
#include <exception>

namespace HaveFunCompiler {
namespace ThreeAddressCode {

SymbolValue::SymbolValue() { type = ValueType::Void; }

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
float SymbolValue::GetFloat() {
  if (type != ValueType::Float) {
    throw std::runtime_error("SymbolValue::GetFloat");
  }
  return std::get<float>(value);
}
int SymbolValue::GetInt() {
  if (type != ValueType::Int) {
    throw std::runtime_error("Type mismatch in SymbolValue::GetInt");
  }
  return std::get<int>(value);
}
const char *SymbolValue::GetStr() {
  if (type != ValueType::Str) {
    throw std::runtime_error("Type mismatch in SymbolValue::GetStr");
  }
  return std::get<const char *>(value);
}
std::shared_ptr<ParameterList> SymbolValue::GetParameters() {
  if (type != ValueType::Parameters) {
    throw std::runtime_error("Type mismatch in SymbolValue::GetParameters");
  }
  return std::get<std::shared_ptr<ParameterList>>(value);
}
}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler