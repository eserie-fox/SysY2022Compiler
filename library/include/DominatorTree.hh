#pragma once
#include <cassert>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include "MacroUtil.hh"

namespace HaveFunCompiler {
    
template <typename IndexType = std::size_t>
class DominatorTree {
  NONCOPYABLE(DominatorTree)

 public:
  DominatorTree() : isbuilt_(false) {
    for (uint8_t i = 0; i < static_cast<uint8_t>(EdgeType::SIZE); i++) {
      edge_sets_[i].clear();
    }
  }

  // Add an edge from u to v (directed edge)
  void AddEdge(IndexType u, IndexType v) {
    if (isbuilt_) {
      throw std::logic_error("AddEdge to a built dominator tree");
    }
    AddEdgeImpl(EdgeType::OriginalMap, u, v);
    AddEdgeImpl(EdgeType::ReversedMap, v, u);
  }

  // Build the dominator tree with specific root
  void Build(IndexType root_idx) {
    if (isbuilt_) {
      throw std::logic_error("Rebuild a built dominator tree");
    }
    root_ = root_idx;
    BuildImpl();
    isbuilt_ = true;
  }

  IndexType get_root() {
    if (!isbuilt_) {
      throw std::logic_error("Query root from a unbuilt dominator tree");
    }
    return root_;
  }

  IndexType get_dominator(IndexType idx) {
    if (!isbuilt_) {
      throw std::logic_error("Query dominator from a unbuilt dominator tree");
    }
    auto it = idom_.find(idx);
    if (it == idom_.end()) {
      throw std::runtime_error("No dominator for " + std::to_string(idx));
    }
    return it->second;
  }

 private:
  using EdgeContainerType = std::vector<IndexType>;
  using EdgeContainerIndexType = std::unordered_map<IndexType, EdgeContainerType>;
  using IndexMapType = std::unordered_map<IndexType, IndexType>;

  // As if we intialized it with arr[x]=x for all x
  struct IndexMapWithIdenticalInitialValueType : public IndexMapType {
    IndexType &operator[](const IndexType &key) {
      auto it = IndexMapType::find(key);
      if (it == IndexMapType::end()) {
        it = IndexMapType::emplace(key, key).first;
      }
      return it->second;
    }
    IndexType &at(const IndexType &key) { return IndexMapWithIdenticalInitialValueType::operator[](key); }
    IndexType &operator[](IndexType &&key) = delete;
    IndexType &at(IndexType &&key) = delete;
  };

  enum class EdgeType : uint8_t { OriginalMap = 0, ReversedMap, SemiDominatorTree, SIZE };

  void AddEdgeImpl(EdgeType type, IndexType u, IndexType v) {
    assert(EdgeType::OriginalMap <= type && type < EdgeType::SIZE);
    uint8_t tid = static_cast<uint8_t>(type);
    auto &edge_set = edge_sets_[tid];
    edge_set[u].push_back(v);
  }

  void BuildImpl() {
    cur_dfn_ = 0;
    dfn_.clear();
    ord_.clear();
    parent_.clear();
    idom_.clear();
    sdom_.clear();
    uni_.clear();
    mn_.clear();
    Dfs(root_);
    for (IndexType i = cur_dfn_; i >= 2; --i) {
      IndexType cur_x = ord_[i];
      for (auto to : edge_sets_[static_cast<uint8_t>(EdgeType::ReversedMap)][cur_x]) {
        if (!dfn_.count(to)) {
          continue;
        }
        QueryUni(to);
        if (dfn_[sdom_[mn_[to]]] < dfn_[sdom_[cur_x]]) {
          sdom_[cur_x] = sdom_[mn_[to]];
        }
      }
      uni_[cur_x] = parent_[cur_x];
      AddEdgeImpl(EdgeType::SemiDominatorTree, sdom_[cur_x], cur_x);
      cur_x = parent_[cur_x];
      for (auto to : edge_sets_[static_cast<uint8_t>(EdgeType::SemiDominatorTree)][cur_x]) {
        QueryUni(to);
        idom_[to] = cur_x == sdom_[mn_[to]] ? cur_x : mn_[to];
      }
      edge_sets_[static_cast<uint8_t>(EdgeType::SemiDominatorTree)][cur_x].clear();
    }
    for (IndexType i = 2; i <= cur_dfn_; ++i) {
      IndexType cur_x = ord_[i];
      if (idom_[cur_x] != sdom_[cur_x]) {
        idom_[cur_x] = idom_[idom_[cur_x]];
      }
    }
  }

  void Dfs(IndexType x) {
    ++cur_dfn_;
    dfn_[x] = cur_dfn_;
    ord_[cur_dfn_] = x;
    auto &edge_set = edge_sets_[static_cast<uint8_t>(EdgeType::OriginalMap)];
    auto &edges = edge_set[x];
    for (auto to : edges) {
      if (!dfn_.count(to)) {
        parent_[to] = x;
        Dfs(to);
      }
    }
  }

  IndexType QueryUni(IndexType x) {
    if (x == uni_[x]) {
      return x;
    }
    IndexType ret = QueryUni(uni_[x]);
    if (dfn_[sdom_[mn_[uni_[x]]]] < dfn_[sdom_[mn_[x]]]) {
      mn_[x] = mn_[uni_[x]];
    }
    uni_[x] = ret;
    return ret;
  }

  bool isbuilt_;

  EdgeContainerIndexType edge_sets_[static_cast<uint8_t>(EdgeType::SIZE)];

  IndexType root_;

  IndexType cur_dfn_;
  IndexMapType dfn_;
  IndexMapType ord_;
  IndexMapType parent_;
  IndexMapType idom_;
  IndexMapWithIdenticalInitialValueType sdom_;
  IndexMapWithIdenticalInitialValueType uni_;
  IndexMapWithIdenticalInitialValueType mn_;
};

}  // namespace HaveFunCompiler