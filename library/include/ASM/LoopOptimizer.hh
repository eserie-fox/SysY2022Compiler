#pragma once
#include <unistd.h>
#include "ASM/Common.hh"
#include "TAC/ThreeAddressCode.hh"

namespace HaveFunCompiler{
namespace AssemblyBuilder {

class LoopDetector;
class ArrivalAnalyzer;
class ControlFlowGraph;
class LoopInvariantDetector;

class LoopOptimizer {
  NONCOPYABLE(LoopOptimizer)
 public:
  LoopOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend);

  int optimize();

 private:
  TACListPtr tacls_;
  TACList::iterator fbegin_, fend_;
  std::shared_ptr<ControlFlowGraph> cfg_;
  std::shared_ptr<ArrivalAnalyzer> arrival_analyzer_;
  std::shared_ptr<LoopDetector> loop_detector_;
  std::shared_ptr<LoopInvariantDetector> loop_invariant_detector_;
};

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler