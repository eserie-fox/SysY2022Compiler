#include <stack>
#include <variant>
#include <memory>
#include "TAC/Symbol.hh"

namespace HaveFunCompiler{
struct VariantStackItem {
  std::variant<uint64_t,uint32_t,int64_t,int32_t,void*,std::shared_ptr<ThreeAddressCode::Symbol>> value;

  template <typename T>
  VariantStackItem(T *v) {
    value = (void *)v;
  }
  VariantStackItem(std::shared_ptr<ThreeAddressCode::Symbol> sym) { value = sym; }
  VariantStackItem(uint64_t v) { value = v; }
  VariantStackItem(uint32_t v) { value = v; }
  VariantStackItem(int64_t v) { value = v; }
  VariantStackItem(int32_t v) { value = v; }
  void Get(std::shared_ptr<ThreeAddressCode::Symbol> *p) {
    if (p) {
      *p = std::get<std::shared_ptr<ThreeAddressCode::Symbol>>(value);
    }
  }
  void Get(uint64_t *p) {
    if (p) {
      *p = std::get<uint64_t>(value);
    }
  }
  void Get(uint32_t *p) {
    if (p) {
      *p = std::get<uint32_t>(value);
    }
  }
  void Get(int64_t *p) {
    if (p) {
      *p = std::get<int64_t>(value);
    }
  }
  void Get(int32_t *p) {
    if (p) {
      *p = std::get<int32_t>(value);
    }
  }
  template <typename T>
  void Get(T **p) {
    if (p) {
      *p = (T *)std::get<void *>(value);
    }
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
   void Top(T *p){
     auto tp = top();
     tp.Get(p);
   }

   void Pop() {
     pop();
   }
};

}  // namespace HaveFunCompiler
