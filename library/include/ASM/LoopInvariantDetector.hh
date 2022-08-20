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
    if (!is_analyzed) {
      throw std::logic_error("Not analyzed");
    }
    auto it = loop_invariants_.find(range);
    if (it == loop_invariants_.end()) {
      throw std::logic_error("No such range detected ( " + std::to_string(range.first) + ", " +
                             std::to_string(range.second) + " )");
    }
    return it->second;
  }

  void analyze();

 private:
  void analyze_impl(std::set<LoopRange> &visited, LoopRange range);

  bool is_analyzed;
  std::unordered_map<LoopRange, std::vector<size_t>> loop_invariants_;
  std::shared_ptr<ControlFlowGraph> cfg_;
  std::shared_ptr<LoopDetector> loop_detector_;
  std::shared_ptr<ArrivalAnalyzer> arrival_analyzer_;
};

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler