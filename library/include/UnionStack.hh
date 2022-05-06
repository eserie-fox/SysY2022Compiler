#include <stack>

namespace HaveFunCompiler{
union UnionStackItem {
  uint64_t u64;
  uint32_t u32;
  int64_t i64;
  int32_t i32;
  void *ptr;
  template <typename T>
  UnionStackItem(T *v) {
    ptr = (void *)v;
  }
  UnionStackItem(uint64_t v) { u64 = v; }
  UnionStackItem(uint32_t v) { u32 = v; }
  UnionStackItem(int64_t v) { i64 = v; }
  UnionStackItem(int32_t v) { i32 = v; }
  void Get(uint64_t *p) {
    if (p) {
      *p = u64;
    }
  }
  void Get(uint32_t *p) {
    if (p) {
      *p = u32;
    }
  }
  void Get(int64_t *p) {
    if (p) {
      *p = i64;
    }
  }
  void Get(int32_t *p) {
    if (p) {
      *p = u64;
    }
  }
  template <typename T>
  void Get(T **p) {
    if (p) {
      *p = (T *)ptr;
    }
  }
  
};  // namespace CompilerStackItem

class UnionStack : protected std::stack<UnionStackItem> {
  public:
   size_t Size() const { return size(); }
   bool IsEmpty() const { return empty(); }
   template <typename T>
   void Push(T v) {
     push(UnionStackItem(v));
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
