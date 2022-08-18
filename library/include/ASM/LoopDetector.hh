#pragma once
#include <ostream>
#include "ASM/Common.hh"
#include "MacroUtil.hh"
#include "TAC/TAC.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {
class ControlFlowGraph;

//用于发现循环
class LoopDetector {
  NONCOPYABLE(LoopDetector)
 public:
  // LoopRange [StartNodeid,EndNodeid] inclusive
  using LoopRange = std::pair<size_t, size_t>;
  LoopDetector(TACList::iterator fbegin, TACList::iterator fend);
  LoopDetector(std::shared_ptr<ControlFlowGraph> cfg);

  const std::vector<LoopRange> &get_loop_ranges() { return loop_ranges_; }

  // For debug
  static std::ostream &PrintLoop(std::ostream &os, const ControlFlowGraph &cfg, LoopRange range);

 private:
  std::vector<LoopRange> loop_ranges_;
};
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler