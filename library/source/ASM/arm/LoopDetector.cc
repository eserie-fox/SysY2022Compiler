#include "ASM/LoopDetector.hh"
#include <algorithm>
#include <unordered_set>
#include "ASM/CFG2DomTreeBuilder.hh"
#include "ASM/ControlFlowGraph.hh"
#include "DomTreeHelper.hh"
#include "DominatorTree.hh"

using namespace HaveFunCompiler;
using namespace AssemblyBuilder;

LoopDetector::LoopDetector(TACList::iterator fbegin, TACList::iterator fend)
    : LoopDetector(std::make_shared<ControlFlowGraph>(fbegin, fend)) {}

LoopDetector::LoopDetector(std::shared_ptr<ControlFlowGraph> cfg) {
  auto dt = BuildDomTreeFromCFG(cfg);
  DomTreeHelper helper(dt);
  std::stack<std::pair<size_t, bool>> stk;
  stk.emplace(dt->get_root(), false);
  stk.emplace(dt->get_root(), true);
  std::unordered_set<size_t> ancestors;
  const auto &tree = helper.get_tree();
  while (!stk.empty()) {
    auto [node_id, in] = stk.top();
    stk.pop();
    if (!in) {
      ancestors.erase(node_id);
      continue;
    }
    ancestors.insert(node_id);
    for (auto to : cfg->get_outNodeList(node_id)) {
      if (ancestors.count(to)) {
        loop_ranges_.emplace_back(to, node_id);
      }
    }
    for (auto to : tree.at(node_id)) {
      stk.emplace(to, false);
      stk.emplace(to, true);
    }
  }
  std::sort(loop_ranges_.begin(), loop_ranges_.end(), [&](LoopRange lhs, LoopRange rhs) {
    lhs.first = cfg->get_node_lino(lhs.first);
    lhs.second = cfg->get_node_lino(lhs.second);
    rhs.first = cfg->get_node_lino(rhs.first);
    rhs.second = cfg->get_node_lino(rhs.second);
    if (rhs.second + lhs.first == lhs.second + rhs.first) {
      return lhs.first < rhs.first;
    }
    return (rhs.second + lhs.first < lhs.second + rhs.first);
  });
}

std::ostream &LoopDetector::PrintLoop(std::ostream &os, const ControlFlowGraph &cfg, LoopRange range) {
  range.first = cfg.get_node_lino(range.first);
  range.second = cfg.get_node_lino(range.second);
  for (auto i = range.first; i <= range.second; i++) {
    auto nodeid = cfg.get_node_id(i);
    auto tac = cfg.get_node_tac(nodeid);
    os << tac->ToString() << '\n';
  }
  return os;
}
// void dfs(std::vector<std::pair<size_t, size_t>> &loop_ranges, std::unordered_set<size_t> &path, ControlFlowGraph &cfg,
//          DomTreeHelper<size_t> &dt, int node_id) {
//   path.insert(node_id);

//   for (auto to : cfg.get_outNodeList(node_id)) {
//     if(path.count(to)){
//       loop_ranges.emplace_back(to, node_id);
//     }
//   }
//   for (auto to : dt.get_tree().at(node_id)) {
//     dfs(loop_ranges, path, cfg, dt, to);
//   }

//   path.erase(node_id);
// }