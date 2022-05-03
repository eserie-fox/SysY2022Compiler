#pragma once

#include <memory>
#include <list>
#include "MacroUtil.hh"

namespace HaveFunCompiler {
namespace ThreeAddressCode {
struct Symbol;
enum class TACOperationType {
  Undefined,
  Add,
  Sub,
  Mul,
  Div,
  Equal,
  NotEqual,
  LessThan,
  LessOrEqual,
  GreaterThan,
  GreaterOrEqual,
  UnaryMinus,
  Assign,
  Goto,
  IfZero,
  FunctionBegin,
  FunctionEnd,
  Label,
  Variable,
  // formal
  Parameter,
  // actual
  Argument,
  Call,
  Return,
};

struct ThreeAddressCode {
  TACOperationType operation_;
  std::shared_ptr<Symbol> a_;
  std::shared_ptr<Symbol> b_;
  std::shared_ptr<Symbol> c_;
};

class ThreeAddressCodeList {
  using list_t = std::list<std::shared_ptr<ThreeAddressCode>>;

 public:
  using iterator = list_t::iterator;
  using const_iterator = list_t::const_iterator;
  ThreeAddressCodeList() = default;
  //复制构造
  ThreeAddressCodeList(const ThreeAddressCodeList &other);
  //移动
  ThreeAddressCodeList(ThreeAddressCodeList &&move_obj);
  //单语句
  ThreeAddressCodeList(std::shared_ptr<ThreeAddressCode> tac);

  ThreeAddressCodeList operator+(const ThreeAddressCodeList &other) const;

  ThreeAddressCodeList &operator=(const ThreeAddressCodeList &other);

  ThreeAddressCodeList &operator+=(const ThreeAddressCodeList &other);

  ThreeAddressCodeList &operator+=(std::shared_ptr<ThreeAddressCode> other);

  std::shared_ptr<ThreeAddressCodeList> MakeCopy() const;

  // iterators
  iterator begin() { return list_.begin(); }
  iterator end() { return list_.end(); }
  const_iterator begin() const { return list_.begin(); }
  const_iterator end() const { return list_.end(); }
  const_iterator cbegin() const { return list_.cbegin(); }
  const_iterator cend() const { return list_.cend(); }

 private:
  list_t list_;
};

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler
