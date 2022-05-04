#pragma once

#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace HaveFunCompiler {
namespace ThreeAddressCode {
enum class SymbolType {
  Undefined,
  Variable,
  Function,
  Text,
  Constant,
  Label,
};
class ParameterList;

struct SymbolValue {
 public:
  enum class ValueType {
    Void,
    Float,
    Int,
    Str,
    Parameters,
  };
  SymbolValue();
  SymbolValue(float v);
  SymbolValue(int v);
  SymbolValue(const char *v);
  SymbolValue(std::shared_ptr<ParameterList> v);
  float GetFloat();
  int GetInt();
  const char *GetStr();
  std::shared_ptr<ParameterList> GetParameters();

  void SetType(ValueType type) { this->type = type; }
  ValueType Type() const { return type; }

 private:
  std::variant<float, int, const char *, std::shared_ptr<ParameterList>> value;
  union {
    float floatValue;
    int intValue;
    const char *strValue;
  };
  ValueType type;
};

struct Symbol {
  SymbolType type_;
  //符号的名字。可能没有名字，例如常量，所以用optional
  std::optional<std::string> name_;
  int offset_;
  SymbolValue value_;
};

class ParameterList : protected std::vector<std::shared_ptr<Symbol>> {
  using Base = std::vector<std::shared_ptr<Symbol>>;

 public:
  inline void push_back_parameter(std::shared_ptr<Symbol> sym) { push_back(sym); }
  using Base::begin;
  using Base::cbegin;
  using Base::cend;
  using Base::end;
};

class SymbolTable : public std::unordered_map<std::string, std::shared_ptr<Symbol>> {
 public:
  SymbolTable(uint64_t table_id) : table_id_(table_id) {}

  inline uint64_t get_tabel_id() const { return table_id_; }

 private:
  const uint64_t table_id_;
};

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler
