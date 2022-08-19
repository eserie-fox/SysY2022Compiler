#pragma once

#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace HaveFunCompiler {
namespace Parser {
class location;
}
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
  std::string ToString() const;

  void SetType(ValueType type) { this->type = type; }

  //检查可操作性，即要么为int要么为float，（或数组访问到最后一维），失败会抛出异常
  void CheckOperatablity(const HaveFunCompiler::Parser::location &loc);

  bool HasOperatablity();

  ValueType Type() const { return type; }
  ValueType UnderlyingType() const;

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

  bool IsAssignableTo(const SymbolValue &other) const;
  std::string TypeToString() const;
  std::string TypeToTACString() const;
  operator bool() const;

 private:
  ValueType type;
  std::variant<float, int, const char *, std::shared_ptr<ParameterList>, std::shared_ptr<ArrayDescriptor>> value;
};

extern const std::unordered_map<SymbolValue::ValueType, size_t> ValueTypeSize;

struct Symbol {
  static const int PARAM = 1;
  SymbolType type_;
  //符号的名字。可能没有名字，例如常量，所以用optional
  std::optional<std::string> name_;
  //该值等于PARAM时显示标记为形参
  int offset_;
  SymbolValue value_;
  std::string get_name() const;
  std::string get_tac_name(bool name_only = false) const;
  bool is_var() const;

  //是否是字面常量
  bool IsLiteral() const;

  //是否是全局变量
  bool IsGlobal() const;

  //是否是全局临时变量
  bool IsGlobalTemp() const;
};

class Expression;

class ArrayDescriptor {
 public:
  std::weak_ptr<Symbol> base_addr;
  std::shared_ptr<Symbol> base_offset;
  SymbolValue::ValueType value_type;
  std::vector<size_t> dimensions;
  //初始化用
  std::shared_ptr<std::unordered_map<size_t, std::shared_ptr<Expression>>> subarray;
  //获取数组占用大小
  size_t GetSizeInByte() const;
};

class ParameterList : public std::vector<std::shared_ptr<Symbol>> {
  using Base = std::vector<std::shared_ptr<Symbol>>;

 public:
  inline void push_back_parameter(std::shared_ptr<Symbol> sym) { push_back(sym); }
  inline void set_return_type(SymbolValue::ValueType type) { ret_type_ = type; }
  inline SymbolValue::ValueType get_return_type() { return ret_type_; }

 protected:
  SymbolValue::ValueType ret_type_;
};

class SymbolTable : public std::unordered_map<std::string, std::shared_ptr<Symbol>> {
 public:
  SymbolTable(uint64_t scope_id) : scope_id_(scope_id) {}

  inline uint64_t get_scope_id() const { return scope_id_; }

 private:
  const uint64_t scope_id_;
};

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler
