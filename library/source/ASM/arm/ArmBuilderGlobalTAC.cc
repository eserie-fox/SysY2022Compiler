#include <iostream>
#include <string>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
#include "ASM/arm/ArmBuilder.hh"
#include "ASM/arm/RegAllocator.hh"
#include "MagicEnum.hh"
#include "TAC/TAC.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {
using namespace HaveFunCompiler::ThreeAddressCode;
std::string ArmBuilder::GlobalTACToASMString([[maybe_unused]] TACPtr tac) {
  std::string ret = "";
  auto emitln = [&ret](const std::string &inst) -> void {
    ret.append(inst);
    ret.append("\n");
  };
  //来个注释好了
  emitln("// " + tac->ToString());
  //用于检测是否是全局用的临时变量
  [[maybe_unused]]auto is_global_tmp = [&, this](SymbolPtr sym) -> bool {
    if (sym->IsGlobal()) {
      if (sym->name_.value().length() > 4) {
        if (sym->name_.value()[2] == 'S' && sym->name_.value()[3] == 'V') {
          return true;
        }
      }
      return false;
    }
    throw std::logic_error(sym->get_tac_name(true) + " is not global symbol");
  };
  switch (tac->operation_) {
    case TACOperationType::Undefined:
      throw std::runtime_error("cannot translate Undefined");
      break;
    case TACOperationType::Add:
    case TACOperationType::Sub:
    case TACOperationType::Mul:
    case TACOperationType::Div:
    case TACOperationType::Mod:
    case TACOperationType::Assign:
    case TACOperationType::UnaryMinus:
    case TACOperationType::UnaryPositive:
    case TACOperationType::Call:
    case TACOperationType::IntToFloat:
    default:
      throw std::logic_error("Unknown operation: " +
                             std::string(magic_enum::enum_name<TACOperationType>(tac->operation_)));
  }

  return ret;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler