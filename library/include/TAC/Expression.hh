#pragma once

#include <memory>
#include <list>

namespace HaveFunCompiler {
namespace ThreeAddressCode {
struct ThreeAddressCode;
class ThreeAddressCodeList;
struct Symbol;
struct Expression {
  std::shared_ptr<ThreeAddressCodeList> tac;
  std::shared_ptr<Symbol> ret;
};
class ExpressionList : public std::list<std::shared_ptr<Expression>> {};
}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler