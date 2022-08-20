#pragma once
#include <ostream>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include "ASM/Common.hh"
#include "LoopDetector.hh"
#include "MacroUtil.hh"
#include "TAC/TAC.hh"
#include "Utility.hh"

namespace HaveFunCompiler{
namespace AssemblyBuilder {

class ArrivalAnalyzer;

class LoopInvariantDetector {
  NONCOPYABLE(LoopInvariantDetector)
 public:
  using LoopRange = LoopDetector::LoopRange;
  LoopInvariantDetector(std::shared_ptr<ControlFlowGraph> cfg, std::shared_ptr<LoopDetector> loop_detector,
                        std::shared_ptr<ArrivalAnalyzer> arrival_analyzer);

  const std::vector<size_t> &get_invariants_of_loop(LoopRange range) const {
    auto it = loop_invariants_.find(range);
    if (it == loop_invariants_.end()) {
      throw std::logic_error("No such range detected ( " + std::to_string(range.first) + ", " +
                             std::to_string(range.second) + " )");
    }
    return it->second;
  }

  void analyze();

  const std::shared_ptr<ArrivalAnalyzer> get_arrival_analyzer() const
  {
    return arrival_analyzer_;
  }

 private:
  void analyze_impl(std::set<LoopRange> &visited, LoopRange range);

  std::unordered_map<LoopRange, std::vector<size_t>> loop_invariants_;
  std::shared_ptr<ControlFlowGraph> cfg_;
  std::shared_ptr<LoopDetector> loop_detector_;
  std::shared_ptr<ArrivalAnalyzer> arrival_analyzer_;
};

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler