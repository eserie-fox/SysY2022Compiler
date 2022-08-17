#pragma once
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include "MacroUtil.hh"

namespace HaveFunCompiler {

template <typename IndexType = size_t>
class BitContainer {
 public:
  BitContainer() = default;

  BitContainer(const BitContainer &) = default;

  BitContainer(BitContainer &&) = default;

  BitContainer &operator=(const BitContainer &) = default;

  BitContainer &operator=(BitContainer &&) = default;

  bool operator==(const BitContainer &rhs) const { return container_ == rhs.container_; }

  bool operator!=(const BitContainer &rhs) const { return container_ != rhs.container_; }

  BitContainer operator&(const BitContainer &rhs) const {
    BitContainer const *self;
    BitContainer const *other;
    if (container_.size() < rhs.container_.size()) {
      self = this;
      other = &rhs;
    } else {
      self = &rhs;
      other = this;
    }
    BitContainer ret;
    for (auto it = self->container_.cbegin(); it != self->container_.cend(); ++it) {
      auto it2 = other->container_.find(it->first);
      if (it2 == other->container_.end()) {
        continue;
      }
      auto andres = it->second & it2->second;
      if (!andres.none()) {
        ret.container_.emplace(it->first, andres);
      }
    }
    return ret;
  }

  BitContainer operator|(const BitContainer &rhs) const {
    BitContainer const *self;
    BitContainer const *other;
    if (container_.size() > rhs.container_.size()) {
      self = this;
      other = &rhs;
    } else {
      self = &rhs;
      other = this;
    }
    BitContainer ret(*self);

    for (auto it = other->container_.cbegin(); it != other->container_.cend(); ++it) {
      auto it2 = ret.container_.find(it->first);
      if (it2 == ret.container_.end()) {
        ret.container_.insert(*it);
        continue;
      }
      it2->second |= it->second;
    }
    return ret;
  }

  BitContainer &operator&=(const BitContainer &rhs) {
    std::vector<InnerIteratorType> rmls;
    for (auto it = container_.begin(); it != container_.end(); ++it) {
      auto it2 = rhs.container_.find(it->first);
      if (it2 == rhs.container_.end()) {
        rmls.push_back(it);
        continue;
      }
      it->second &= it2->second;
    }
    for (auto it : rmls) {
      container_.erase(it);
    }
    return *this;
  }
  BitContainer &operator|=(const BitContainer &rhs) {
    std::vector<InnerPairType> addls;
    for (auto it = rhs.container_.cbegin(); it != rhs.container_.cend(); ++it) {
      auto it2 = container_.find(it->first);
      if (it2 == container_.end()) {
        addls.push_back(*it);
        continue;
      }
      it2->second |= it->second;
    }
    for (const auto &pr : addls) {
      container_.insert(pr);
    }
    return *this;
  }

  void set(IndexType pos) { container_[FIR(pos)]._Unchecked_set(SEC(pos)); }

  void reset(IndexType pos) { container_[FIR(pos)]._Unchecked_reset(SEC(pos)); }

  bool test(IndexType pos) {
    auto it = container_.find(FIR(pos));
    if (it == container_.end()) {
      return false;
    }
    return it->second._Unchecked_test(SEC(pos));
  }

 private:
  using size_t = std::size_t;
  static const size_t NBITLOG = 8;
  static const size_t NBIT = 1 << NBITLOG;
  static const size_t NBITMASK = NBIT - 1;

  inline size_t FIR(IndexType pos) { return static_cast<size_t>(pos >> NBITLOG); }
  inline size_t SEC(IndexType pos) { return static_cast<size_t>(pos & NBITMASK); }

  using BitGroup = std::bitset<NBIT>;
  using InnerContainerType = std::unordered_map<IndexType, BitGroup>;
  using InnerIteratorType = typename InnerContainerType::iterator;
  using InnerPairType = std::pair<IndexType, BitGroup>;
  InnerContainerType container_;
};
}  // namespace HaveFunCompiler