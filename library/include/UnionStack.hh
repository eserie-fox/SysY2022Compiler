#pragma once

#include <memory>
#include <stack>
#include <variant>
#include "TAC/Symbol.hh"

namespace HaveFunCompiler {
struct VariantStackItem {
  std::variant<uint64_t, uint32_t, int64_t, int32_t, void *, std::shared_ptr<ThreeAddressCode::Symbol>> value;

  template <typename T>
  VariantStackItem(T *v) {
    value = (void *)v;
  }
  template <typename T>
  VariantStackItem(T v) {
    value = v;
  }

  template <typename T>
  bool Get(T *p) {
    if (p) {
      T *buf = std::get_if<T>(&value);
      if (buf) {
        *p = *buf;
        return true;
      }
    }
    return false;
  }

  template <typename T>
  bool Get(T **p) {
    if (p) {
      T **buf = std::get_if<void *>(&value);
      if (buf) {
        *p = *buf;
        return true;
      }
    }
    return false;
  }

};  // namespace CompilerStackItem

class VariantStack : protected std::stack<VariantStackItem> {
 public:
  size_t Size() const { return size(); }
  bool IsEmpty() const { return empty(); }
  template <typename T>
  void Push(T v) {
    push(VariantStackItem(v));
  }

  template <typename T>
  bool Top(T *p) {
    auto tp = top();
    return tp.Get(p);
  }

  void Pop() { pop(); }
};

}  // namespace HaveFunCompiler
