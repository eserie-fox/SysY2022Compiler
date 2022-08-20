#include "ASM/LoopInvariantDetector.hh"
#include <cassert>
#include <unordered_set>
#include "ASM/ArrivalAnalyzer.hh"
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LoopDetector.hh"
#include "ASM/SymAnalyzer.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {
LoopInvariantDetector::LoopInvariantDetector(std::shared_ptr<ControlFlowGraph> cfg,
                                             std::shared_ptr<LoopDetector> loop_detector,
                                             std::shared_ptr<ArrivalAnalyzer> arrival_analyzer)
    : is_analyzed(false),
      loop_invariants_(),
      cfg_(cfg),
      loop_detector_(loop_detector),
      arrival_analyzer_(arrival_analyzer) {}

void LoopInvariantDetector::analyze() {
  if (!is_analyzed) {
    std::set<LoopRange> visited;
    const auto &loop_ranges = loop_detector_->get_loop_ranges();
    for (auto range : loop_ranges) {
      range.first = cfg_->get_node_lino(range.first);
      range.second = cfg_->get_node_lino(range.second);
      analyze_impl(visited, range);
    }
    is_analyzed = true;
  }
}
void LoopInvariantDetector::analyze_impl(std::set<LoopRange> &visited, LoopRange range) {
  //应为无符号数。
  static_assert(static_cast<LoopRange::first_type>(-1) > static_cast<LoopRange::first_type>(0));
  const static LoopRange::first_type MAX = static_cast<LoopRange::first_type>(-1);
  //存放对于这个循环而言的循环不变量。
  std::unordered_set<size_t> ans_invariant;
  auto sym_analyzer = arrival_analyzer_->get_symAnalyzer();

  auto check_vis = [&](size_t pos) -> size_t {
    if (visited.empty()) {
      return MAX;
    }
    LoopRange query_range(pos, MAX);
    auto it = visited.upper_bound(query_range);
    if (it == visited.begin()) {
      return MAX;
    }
    --it;
    if (it != visited.end() && it->second >= pos) {
      return it->second;
    }
    return MAX;
  };
  auto insert_range = [&](LoopRange range) -> void {
    LoopRange query_range(range.first, 0);
    auto it = visited.lower_bound(query_range);
    while (it != visited.end()) {
      if (it->first <= range.second) {
        assert(it->second <= range.second);
        it = visited.erase(it);
        continue;
      }
      break;
    }
    visited.insert(range);
  };

  auto inrange = [&](const std::unordered_set<size_t> &set) -> bool {
    // size < second - first + 1
    if (set.size() + range.first < range.second + 1) {
      for (auto x : set) {
        if (x >= range.first && x <= range.second) {
          return true;
        }
      }
    } else {
      for (auto i = range.first; i <= range.second; i++) {
        if (set.count(i)) {
          return true;
        }
      }
    }
    return false;
  };

  auto is_invariant_sym = [&](SymbolPtr sym, const std::unordered_set<size_t> &reachable_defs) -> bool {
    if (sym->value_.Type() == SymbolValue::ValueType::Array) {
      sym = sym->value_.GetArrayDescriptor()->base_offset;
    }
    if (sym->IsLiteral()) {
      return true;
    }
    const auto &def_points = sym_analyzer->getSymDefPoints(sym);
    if (!inrange(def_points)) {
      return true;
    }
    const std::unordered_set<size_t> *smaller = &reachable_defs;
    const std::unordered_set<size_t> *bigger = &def_points;
    if (smaller->size() > bigger->size()) {
      swap(smaller, bigger);
    }
    std::optional<size_t> res_node_id = std::nullopt;

    for (auto node_id : *smaller) {
      if (bigger->count(node_id)) {
        //交集只能有一个的时候才有可能返回true
        if (res_node_id.has_value()) {
          return false;
        }
        res_node_id = node_id;
      }
    }
    if (!res_node_id.has_value()) {
      return false;
    }
    if (ans_invariant.count(res_node_id.value())) {
      return true;
    }
    return false;
  };
  auto is_binary_operation = [](TACOperationType type) -> bool {
    if (type >= TACOperationType::Add && type <= TACOperationType::GreaterOrEqual) {
      return true;
    }
    return false;
  };
  auto is_unary_operation = [](TACOperationType type) -> bool {
    if (type >= TACOperationType::UnaryMinus && type <= TACOperationType::UnaryPositive) {
      return true;
    }
    if (type == TACOperationType::Assign || type == TACOperationType::IntToFloat ||
        type == TACOperationType::FloatToInt) {
      return true;
    }
    return false;
  };
  auto is_invariant = [&](size_t node_id) -> bool {
    auto tac = cfg_->get_node_tac(node_id);
    if (!is_unary_operation(tac->operation_) && !is_binary_operation(tac->operation_)) {
      return false;
    }
    auto reachable_defs = arrival_analyzer_->getReachableDefs(node_id);
    if (is_unary_operation(tac->operation_)) {
      if (tac->operation_ == TACOperationType::Assign) {
        if (tac->a_->value_.Type() == SymbolValue::ValueType::Array) {
          auto sym = tac->a_->value_.GetArrayDescriptor()->base_offset;
          if (!is_invariant_sym(sym, reachable_defs)) {
            return false;
          }
        }
      }
      return is_invariant_sym(tac->b_, reachable_defs);
    }
    return is_invariant_sym(tac->b_, reachable_defs) && is_invariant_sym(tac->c_, reachable_defs);
  };

  for (auto i = range.first; i <= range.second; i++) {
    // i是行号
    auto chk = check_vis(i);
    if (chk != MAX) {
      i = chk;
      continue;
    }
    auto node_id = cfg_->get_node_id(i);
    //如果被删除就忽略
    if (cfg_->get_node_dfn(node_id) == 0) {
      continue;
    }
    if (is_invariant(node_id)) {
      ans_invariant.insert(node_id);
    }
  }
  insert_range(range);
  auto &ans = loop_invariants_[range];
  ans.insert(ans.end(), ans_invariant.begin(), ans_invariant.end());
  std::sort(ans.begin(), ans.end(),
            [&](size_t nid1, size_t nid2) -> bool { return cfg_->get_node_lino(nid1) < cfg_->get_node_lino(nid2); });
}
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler