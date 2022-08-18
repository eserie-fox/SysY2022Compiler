#include <queue>
#include "DominatorTree.hh"

namespace HaveFunCompiler {
using IndexType = size_t;
// template <typename IndexType = size_t>
class DomTreeHelper {
  NONCOPYABLE(DomTreeHelper)
 public:
  using TreeType = std::unordered_map<IndexType, std::vector<IndexType>>;
  using DepthType = std::unordered_map<IndexType, IndexType>;
  DomTreeHelper(std::shared_ptr<DominatorTree<IndexType>> dom_tree) : dom_tree_(dom_tree), has_aux_info_(false) {
    all_nodes_ = dom_tree->get_all_nodes();
    BuildTree(dom_tree, all_nodes_);
    auto max_dep = BuildDepthInfo(tree_, dom_tree->get_root());
    max_depth_level_ = 1;
    while (((static_cast<IndexType>(1)) << static_cast<IndexType>(max_depth_level_)) < max_dep) {
      ++max_depth_level_;
    }
  }

  void CalcAuxInfo() {
    if (!has_aux_info_) {
      has_aux_info_ = true;
      CalcAuxInfo(dom_tree_, tree_, all_nodes_);
      dom_tree_ = nullptr;
      all_nodes_.clear();
    }
  }

  // Check if the 'ancestor' is the real ancestor of the 'offsprint'. One is its own ancestor.
  bool IsAncestorOf(IndexType ancestor, IndexType offspring) {
    if (!has_aux_info_) {
      CalcAuxInfo();
      // throw std::logic_error("Haven't calc aux info yet");
    }
    auto dep1 = depth_info_[ancestor];
    auto dep2 = depth_info_[offspring];
    if (dep1 > dep2) {
      return false;
    }
    for (int i = max_depth_level_; i >= 0; i--) {
      if (dep2 < dep1 + (static_cast<IndexType>(1) << static_cast<IndexType>(i))) {
        continue;
      }
      offspring = aux_info_[offspring][i];
      dep2 -= (static_cast<IndexType>(1) << static_cast<IndexType>(i));
    }
    return ancestor == offspring;
  }

  const TreeType &get_tree() const { return tree_; }

  IndexType get_depth(IndexType node) const { return depth_info_.at(node); }

 private:
  using AuxiliaryInfoType = std::unordered_map<IndexType, std::vector<IndexType>>;
  void BuildTree(std::shared_ptr<DominatorTree<IndexType>> dom_tree, const std::vector<IndexType> &all_nodes) {
    tree_.clear();
    for (auto x : all_nodes) {
      tree_[dom_tree->get_dominator(x)].push_back(x);
    }
  }

  // return max depth
  IndexType BuildDepthInfo(const TreeType &tree, IndexType root) {
    depth_info_.clear();
    depth_info_[root] = 1;
    std::queue<IndexType> Q;
    IndexType max_dep = 1;
    Q.push(root);
    while (!Q.empty()) {
      auto now = Q.front();
      Q.pop();
      auto selfdep = depth_info_[now];
      max_dep = std::max(max_dep, selfdep);
      auto it = tree.find(now);
      if (it != tree.end()) {
        auto &edges = it->second;
        for (auto to : edges) {
          depth_info_[to] = selfdep + 1;
          Q.push(to);
        }
      }
    }
    return max_dep;
  }

  void CalcAuxInfo(std::shared_ptr<DominatorTree<IndexType>> dom_tree, const TreeType &tree,
                   const std::vector<IndexType> &all_nodes) {
    auto root = dom_tree->get_root();
    {
      auto &rootinfo = aux_info_[root];
      rootinfo.resize(max_depth_level_ + 1);
      for (int i = 0; i <= max_depth_level_; i++) {
        rootinfo[i] = root;
      }
    }
    for (auto x : all_nodes) {
      auto &info = aux_info_[x];
      info.resize(max_depth_level_ + 1);
      info[0] = dom_tree->get_dominator(x);
    }
    std::queue<IndexType> Q;
    Q.push(root);
    while (!Q.empty()) {
      auto now = Q.front();
      Q.pop();
      auto it = tree.find(now);
      if (it != tree.end()) {
        auto &edges = it->second;
        for (auto to : edges) {
          Q.push(to);
          auto &info = aux_info_[to];
          for (int i = 1; i <= max_depth_level_; i++) {
            info[i] = aux_info_[info[i - 1]][i - 1];
          }
        }
      }
    }
  }
  std::shared_ptr<DominatorTree<IndexType>> dom_tree_;
  bool has_aux_info_;
  std::vector<size_t> all_nodes_;
  TreeType tree_;
  DepthType depth_info_;
  int max_depth_level_;
  AuxiliaryInfoType aux_info_;
};
}  // namespace HaveFunCompiler