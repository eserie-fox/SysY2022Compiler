#include "TAC/Symbol.hh"

namespace HaveFunCompiler{
namespace ThreeAddressCode {
enum class OperationType {
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
  Declaration,

};

struct ThreeAddressCode {};
}  // namespace ThreeAddressCode
}
