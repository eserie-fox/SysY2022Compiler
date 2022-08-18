#pragma once

#include "ASM/ControlFlowGraph.hh"
#include "DominatorTree.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {

std::shared_ptr<DominatorTree<size_t>> BuildDomTreeFromCFG(std::shared_ptr<ControlFlowGraph> cfg);

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler