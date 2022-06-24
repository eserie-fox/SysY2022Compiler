#include <iostream>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
#include "ASM/arm/ArmBuilder.hh"
#include "ASM/arm/RegAllocator.hh"
#include "MagicEnum.hh"
#include "TAC/TAC.hh"

namespace HaveFunCompiler {
using namespace ThreeAddressCode;
namespace AssemblyBuilder {
std::string ArmBuilder::FuncTACToASMString([[maybe_unused]] TACPtr tac) {
  std::string ret = "";
  [[maybe_unused]] auto emitln = [&ret](const std::string &inst) -> void {
    ret.append(inst);
    ret.append("\n");
  };

  auto binary_operation = [&, this]() -> void {

  };
  auto unary_operation = [&, this]() -> void {

  };
  auto assignment = [&, this]() -> void {

  };
  auto branch = [&, this]() -> void {

  };
  auto functionality = [&, this]() -> void {

  };
  switch (tac->operation_) {
    case TACOperationType::Add:
    case TACOperationType::Div:
    case TACOperationType::LessOrEqual:
    case TACOperationType::LessThan:
    case TACOperationType::Mod:
    case TACOperationType::Mul:
    case TACOperationType::NotEqual:
    case TACOperationType::Sub:
    case TACOperationType::GreaterOrEqual:
    case TACOperationType::GreaterThan:
    case TACOperationType::Equal:
      binary_operation();
      break;
    case TACOperationType::UnaryMinus:
    case TACOperationType::UnaryPositive:
    case TACOperationType::UnaryNot:
      unary_operation();
      break;
    case TACOperationType::Assign:
      assignment();
      break;
    case TACOperationType::Goto:
    case TACOperationType::IfZero:
      branch();
      break;
    case TACOperationType::Parameter:
    case TACOperationType::Argument:
    case TACOperationType::ArgumentAddress:
    case TACOperationType::Call:
    case TACOperationType::Return:
      functionality();
      break;
    case TACOperationType::Label:
    case TACOperationType::Variable:
    case TACOperationType::Constant:
    case TACOperationType::BlockBegin:
    case TACOperationType::BlockEnd:
      // ignore
      break;
    default:
      throw std::runtime_error("Unexpected operation: " +
                               std::string(magic_enum::enum_name<TACOperationType>(tac->operation_)));
      break;
  }

  return ret;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler