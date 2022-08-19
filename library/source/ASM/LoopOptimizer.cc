#include "ASM/LoopOptimizer.hh"
#include "ASM/ArrivalAnalyzer.hh"
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LoopDetector.hh"
#include "ASM/LoopInvariantDetector.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {

LoopOptimizer::LoopOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend)
    : tacls_(tacList),
      fbegin_(fbegin),
      fend_(fend),
      cfg_(std::make_shared<ControlFlowGraph>(fbegin, fend)),
      arrival_analyzer_(std::make_shared<ArrivalAnalyzer>(cfg_)),
      loop_detector_(std::make_shared<LoopDetector>(cfg_)),
      loop_invariant_detector_(std::make_shared<LoopInvariantDetector>(cfg_, loop_detector_, arrival_analyzer_)) {}

int LoopOptimizer::optimize() {
  (void)tacls_;
  return 0;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler