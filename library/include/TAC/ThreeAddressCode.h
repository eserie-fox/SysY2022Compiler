#include "TAC/Symbol.h"

namespace ThreeAddressCode{
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

struct ThreeAddressCode {

};
}

