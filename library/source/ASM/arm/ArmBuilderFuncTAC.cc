#include <iostream>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
#include "ASM/arm/ArmBuilder.hh"
#include "ASM/arm/RegAllocator.hh"
#include "MagicEnum.hh"
#include "TAC/TAC.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {

std::string ArmBuilder::FuncTACToASMString([[maybe_unused]] TACPtr tac) { return ""; }

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler