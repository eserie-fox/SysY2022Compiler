#include "ASM/CFG2DomTreeBuilder.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {

std::shared_ptr<DominatorTree<size_t>> BuildDomTreeFromCFG(std::shared_ptr<ControlFlowGraph> cfg) {
  std::shared_ptr<DominatorTree<size_t>> ret = std::make_shared<DominatorTree<size_t>>();
  FOR_EACH_NODE(cur, cfg) {
    auto &edges = cfg->get_outNodeList(cur);
    for (auto to : edges) {
      ret->AddEdge(cur, to);
    }
  }
  ret->Build(cfg->get_startNode());
  return ret;
}

}
}