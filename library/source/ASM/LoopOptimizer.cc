#include "ASM/LoopOptimizer.hh"
#include "ASM/ArrivalAnalyzer.hh"
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LoopDetector.hh"
#include "ASM/LoopInvariantDetector.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {

LoopOptimizer::LoopOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend)
  : tacls_(tacList), fbegin_(fbegin), fend_(fend)
{
  cfg_ = std::make_shared<ControlFlowGraph>(fbegin, fend);
  arrival_analyzer_ = std::make_shared<ArrivalAnalyzer>(cfg_);
  loop_detector_ = std::make_shared<LoopDetector>(cfg_);
  arrival_analyzer_->analyze();
  loop_invariant_detector_ = std::make_shared<LoopInvariantDetector>(cfg_, loop_detector_, arrival_analyzer_);
}

int LoopOptimizer::optimize() {
  loop_invariant_detector_->analyze();
  
  auto &loopRanges = loop_detector_->get_loop_ranges();
  auto &liveAnalyzer = loop_invariant_detector_->get_arrival_analyzer()->get_liveAnalyzer();

  auto checkLegalLoop = [this](size_t lbegin, size_t lend)
  {
    if (cfg_->get_node_tac(lbegin)->operation_ != TACOperationType::Label) return false;
    if (cfg_->get_inDegree(lbegin) != 1)  return false;
    if (cfg_->get_node_tac(cfg_->get_inNodeList(lbegin)[0])->operation_ != TACOperationType::IfZero)  return false;
  
    auto &endSuc = cfg_->get_outNodeList(lend);
    size_t u = 0;
    for (; u < endSuc.size(); ++u)
      if (endSuc[u] == lbegin)
        break;
    if (u == endSuc.size())  return false;
    return true;
  };

  // 由内循环到外循环，依次对循环范围进行外提
  // Begin和End：开始和结束语句的nodeid
  for (auto &loopRange : loopRanges)
  {
    auto loopBegin = loopRange.first;
    auto loopEnd = loopRange.second;

    // 检查循环开始语句和结束语句的合法性
    assert(checkLegalLoop(loopBegin, loopEnd));

    auto beginLineNo = cfg_->get_node_lino(loopBegin);
    auto endLineNo = cfg_->get_node_lino(loopEnd);

    // 得到循环前置结点和循环出口结点
    size_t loopHead = cfg_->get_inNodeList(loopBegin)[0];
    std::vector<size_t> loopOut;
    for (auto u = loopBegin; u <= loopEnd; ++u)
    {
      auto &outls = cfg_->get_outNodeList(u);
      for (auto v : outls)
        if (v < loopBegin || v > loopEnd)
        {
          loopOut.push_back(u);
          break;
        }
    }

    // 检查外提条件：对同一不变量只能有1个定值，记录对不变量的定值次数
    std::unordered_map<SymbolPtr, size_t> defCnt;
    auto &invariants = loop_invariant_detector_->get_invariants_of_loop(loopRange);
    for (auto d : invariants)
    {
      auto dTac = cfg_->get_node_tac(d);
      auto invariantSym = dTac->getDefineSym();

      // 目前dTac左侧有可能是数组
      
    }
  }

  return 0;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler