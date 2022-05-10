#pragma once

#include <memory>
#include <vector>

namespace HaveFunCompiler {
namespace ThreeAddressCode {
struct ThreeAddressCode;
class ThreeAddressCodeList;
struct Symbol;
struct Expression {
  std::shared_ptr<ThreeAddressCodeList> tac;
  std::shared_ptr<Symbol> ret;
};
class ArgumentList : protected std::vector<std::shared_ptr<Expression>> {
  using Base = std::vector<std::shared_ptr<Expression>>;

 public:
  inline void push_back_argument(std::shared_ptr<Expression> exp) { push_back(exp); }
  using Base::begin;
  using Base::cbegin;
  using Base::cend;
  using Base::end;
  using Base::size;
};
}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler