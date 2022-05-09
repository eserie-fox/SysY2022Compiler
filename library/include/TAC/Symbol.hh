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

class ArrayDescriptor;

struct SymbolValue {
 public:
  enum class ValueType {
    Void,
    Float,
    Int,
    Str,
    Parameters,
    Array,
  };
  SymbolValue(const SymbolValue &other);
  SymbolValue();
  SymbolValue(float v);
  SymbolValue(int v);
  SymbolValue(const char *v);
  SymbolValue(std::shared_ptr<ParameterList> v);
  SymbolValue(std::shared_ptr<ArrayDescriptor> v);
  float GetFloat() const;
  int GetInt() const;
  const char *GetStr() const;
  std::shared_ptr<ParameterList> GetParameters() const;
  std::shared_ptr<ArrayDescriptor> GetArrayDescriptor() const;

  void SetType(ValueType type) { this->type = type; }
  ValueType Type() const { return type; }
  //是否是数值类型
  bool IsNumericType() const;

  SymbolValue &operator=(const SymbolValue &other);

  SymbolValue operator+(const SymbolValue &other) const;
  SymbolValue operator-(const SymbolValue &other) const;
  SymbolValue operator*(const SymbolValue &other) const;
  SymbolValue operator/(const SymbolValue &other) const;
  SymbolValue operator%(const SymbolValue &other) const;
  SymbolValue operator!() const;
  SymbolValue operator+() const;
  SymbolValue operator-() const;
  SymbolValue operator<(const SymbolValue &other) const;
  SymbolValue operator>(const SymbolValue &other) const;
  SymbolValue operator<=(const SymbolValue &other) const;
  SymbolValue operator>=(const SymbolValue &other) const;
  SymbolValue operator==(const SymbolValue &other) const;
  SymbolValue operator!=(const SymbolValue &other) const;
  SymbolValue operator&&(const SymbolValue &other) const;
  SymbolValue operator||(const SymbolValue &other) const;

  operator bool() const;

 private:
  ValueType type;
  std::variant<float, int, const char *, std::shared_ptr<ParameterList>, std::shared_ptr<ArrayDescriptor>> value;
};

extern const std::unordered_map<SymbolValue::ValueType, size_t> ValueTypeSize;

struct Symbol {
  SymbolType type_;
  //符号的名字。可能没有名字，例如常量，所以用optional
  std::optional<std::string> name_;
  int offset_;
  SymbolValue value_;
};

class ArrayDescriptor {
 public:
  std::weak_ptr<Symbol> base_addr;
  size_t base_offset;
  SymbolValue::ValueType value_type;
  std::vector<size_t> dimensions;
  std::shared_ptr<std::unordered_map<size_t, std::shared_ptr<Symbol>>> subarray;
};

class ParameterList : protected std::vector<std::shared_ptr<Symbol>> {
  using Base = std::vector<std::shared_ptr<Symbol>>;

 public:
  inline void push_back_parameter(std::shared_ptr<Symbol> sym) { push_back(sym); }
  inline void set_return_type(SymbolValue::ValueType type) { ret_type_ = type; }
  inline SymbolValue::ValueType get_return_type() { return ret_type_; }
  using Base::begin;
  using Base::cbegin;
  using Base::cend;
  using Base::end;

 protected:
  SymbolValue::ValueType ret_type_;
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
