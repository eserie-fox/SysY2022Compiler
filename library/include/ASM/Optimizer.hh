#pragma once

#include "ASM/Common.hh"
#include "TAC/ThreeAddressCode.hh"
#include <unordered_map>

namespace HaveFunCompiler{
namespace AssemblyBuilder{


class OptimizeController
{
public:
    OptimizeController() = default;
    virtual ~OptimizeController() = default;
    NONCOPYABLE(OptimizeController)

    virtual void doOptimize() = 0;
};


class DeadCodeOptimizer;
class SimpleOptimizer;
class DataFlowManager;

class OptimizeController_Simple : public OptimizeController
{
public:
    OptimizeController_Simple(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend);
    NONCOPYABLE(OptimizeController_Simple)

    void doOptimize() override;

private:
    TACListPtr tacls_;
    std::shared_ptr<DataFlowManager> dataFlowManager;
    std::shared_ptr<DeadCodeOptimizer> deadCodeOp;
};

// optimizer需要保证，不修改fbegin和fend
// 死代码消除优化，当一个定值后续没有使用时，删除这个定值
class DeadCodeOptimizer
{
public:
    DeadCodeOptimizer(std::shared_ptr<DataFlowManager> dataFlowManager, TACListPtr tacList);
    NONCOPYABLE(DeadCodeOptimizer)

    int optimize();

private:
    TACList::iterator _fbegin, _fend;
    TACListPtr _tacls;
    std::shared_ptr<DataFlowManager> dfm;

    bool hasSideEffect(SymbolPtr defSym, TACPtr tac);
};

// 常量传播/复写传播优化器
class PropagationOptimizer
{
public:
    PropagationOptimizer(std::shared_ptr<DataFlowManager> dataFlowManager);
    NONCOPYABLE(PropagationOptimizer)

    int optimize();

private:
    std::shared_ptr<DataFlowManager> dfm;
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