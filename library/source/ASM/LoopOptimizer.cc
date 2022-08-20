#include "ASM/LoopOptimizer.hh"
#include "ASM/ArrivalAnalyzer.hh"
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LoopDetector.hh"
#include "ASM/LoopInvariantDetector.hh"
#include "ASM/LiveAnalyzer.hh"
#include "ASM/CFG2DomTreeBuilder.hh"
#include "DomTreeHelper.hh"
#include <deque>

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
  auto domTree = BuildDomTreeFromCFG(cfg_);
  DomTreeHelper domTreeHelper(domTree);

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

  // 对数组定值需要特殊处理
  // 没有别名分析，则对数组任意元素的定值，都视为对整个数组的定值
  auto getDefSym = [](TACPtr tac) -> SymbolPtr
  {
    auto defSym = tac->getDefineSym();
    if (defSym)
      return defSym;
    if (tac->operation_ == TACOperationType::Assign && tac->a_->value_.Type() == SymbolValue::ValueType::Array)
      return tac->a_->value_.GetArrayDescriptor()->base_addr.lock();
    return nullptr;
  };

  // 把src处语句移动到tar之前
  auto extract = [this](size_t tar, size_t src)
  {
    auto srcTac = cfg_->get_node_tac(src);
    auto tarIt = cfg_->get_node_itr(tar);
    tacls_->insert(tarIt, srcTac);
    auto srcIt = cfg_->get_node_itr(src);
    tacls_->erase(srcIt);
  };

  int cnt = 0;

  // 由内循环到外循环，依次对循环范围进行外提
  // Begin和End：开始和结束语句的nodeid
  for (auto &loopRange : loopRanges)
  {
    auto loopBegin = loopRange.first;
    auto loopEnd = loopRange.second;

    // 检查循环开始语句和结束语句的合法性
    assert(checkLegalLoop(loopBegin, loopEnd));

    auto &invariants = loop_invariant_detector_->get_invariants_of_loop(loopRange);
    if (invariants.size() == 0)  // 没有不变量，直接跳过
      continue;

    // 保存只有1个定值的不变量，其他的不能外提
    // 如果有多个定值，外提后无法保证循环内得到正确的值
    // 对数组元素的定值，特殊处理，SymbolPtr保存数组symbol
    std::unordered_map<SymbolPtr, size_t> repDef;
    for (auto d : invariants)
    {
      auto dTac = cfg_->get_node_tac(d);
      auto invariantSym = getDefSym(dTac);
      ++repDef[invariantSym];
    }

    std::deque<size_t> extractInvariants;
    for (auto d : invariants)
    {
      if (repDef[getDefSym(cfg_->get_node_tac(d))] == 1)
        extractInvariants.push_back(d);
    }

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

    // 对每个不变量，进行其它外提条件检查
    auto uniqSiz = extractInvariants.size();
    for (size_t i = 0; i < uniqSiz; ++i)
    {
      auto d = extractInvariants.front();
      extractInvariants.pop_front();
      auto invariantSym = getDefSym(cfg_->get_node_tac(d));

      // 不变量不能在循环头的出口活跃
      // 否则，第一次循环将使用错误的值
      if (liveAnalyzer->isOutLive(invariantSym, loopHead))
        continue;
      
      // 对于出口活跃集合包含invariantSym的循环出口节点
      // d必须是该节点的必经结点
      // 否则，循环内部不一定会计算d，不能将其外提
      bool canExtract = true;
      for (auto out : loopOut)
      {
        if (liveAnalyzer->isOutLive(invariantSym, out))
        {
          if (!domTreeHelper.IsAncestorOf(d, out))
          {
            canExtract = false;
            break;
          }
        }
      }

      if (canExtract)
        extractInvariants.push_back(d);
    }

    // 现在保留在extractInvariants中的就是能外提的语句
    for (auto d : extractInvariants)
      extract(loopBegin, d);
    cnt += extractInvariants.size();
  }

  return cnt;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler