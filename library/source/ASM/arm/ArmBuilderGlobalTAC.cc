#include <iostream>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
#include "ASM/arm/ArmBuilder.hh"
#include "ASM/arm/RegAllocator.hh"
#include "MagicEnum.hh"
#include "TAC/TAC.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {
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
  

  return ret;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler