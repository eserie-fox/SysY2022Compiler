#include "TAC/Symbol.hh"
#include <cassert>
#include <exception>

namespace HaveFunCompiler {
namespace ThreeAddressCode {

SymbolValue::SymbolValue(float v) {
  floatValue = v;
  type = ValueType::Float;
}
SymbolValue::SymbolValue(int v) {
  intValue = v;
  type = ValueType::Int;
}
SymbolValue::SymbolValue(const char *v) {
  strValue = v;
  type = ValueType::Str;
}
float SymbolValue::GetFloat() {
  if (type != ValueType::Float) {
    throw std::runtime_error("SymbolValue::GetFloat");
  }
  return floatValue;
}
int SymbolValue::GetInt() {
  if (type != ValueType::Int) {
    throw std::runtime_error("Type mismatch in SymbolValue::GetInt");
  }
  return intValue;
}
const char *SymbolValue::GetStr() {
  if (type != ValueType::Str) {
    throw std::runtime_error("Type mismatch in SymbolValue::GetStr");
  }
  return strValue;
}
}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler