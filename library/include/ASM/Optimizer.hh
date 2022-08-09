#pragma once

#include "ASM/Common.hh"
#include "TAC/ThreeAddressCode.hh"

namespace HaveFunCompiler{
namespace AssemblyBuilder{

class LiveAnalyzer;

// optimizer需要保证，不修改fbegin和fend
class DeadCodeOptimizer
{
public:
    DeadCodeOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend) : _fbegin(fbegin), _fend(fend), _tacls(tacList) {}
    NONCOPYABLE(DeadCodeOptimizer)

    void optimize();

private:
    TACList::iterator _fbegin, _fend;
    TACListPtr _tacls;

    bool hasSideEffect(SymbolPtr defSym, TACPtr tac);
};

//会将一些简单情况优化掉。
// 情况1，+0会被优化成assign
// 情况2，*1会被优化成assign
// 情况3，自己对自己assign会被去掉。
class SimpleOptimizer {
  NONCOPYABLE(SimpleOptimizer)
 public:
  SimpleOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend);

  void optimize();

 private:
  TACList::iterator fbegin_, fend_;
  TACListPtr tacls_;
};
}  // namespace AssemblyBuilder
}