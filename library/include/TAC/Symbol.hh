#pragma once

#include <optional>
#include <string>

namespace HaveFunCompiler{
namespace ThreeAddressCode {
enum class SymbolType {
  Undefined,
  Variable,
  Function,
  Text,
  Constant,
  Label,
};

struct SymbolValue {
  union {
    float floatValue;
    int intValue;
    const char *strValue;
  };
};

struct Symbol {
  SymbolType type_;
  //符号的名字。可能没有名字，例如常量，所以用optional
  std::optional<std::string> name_;
  int offset_;
  SymbolValue value_;
};
}  // namespace ThreeAddressCode
}
