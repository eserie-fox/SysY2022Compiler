#pragma once
#include <unistd.h>
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

class ControlFlowGraph;
class LiveAnalyzer;
class SymAnalyzer;
class ArrivalAnalyzer;
class ArrivalExprAnalyzer;
class UseDefAnalyzer;
class CommExpAnalyzer;

class DeadCodeOptimizer_UseDef;
class SimpleOptimizer;
class ConstantFoldingOptimizer;
class PropagationOptimizer;
class DeadCodeOptimizer;
class CommExpOptimizer;
class LoopOptimizer;
class DataFlowManager;

class OptimizeController_Simple : public OptimizeController
{
public:
    OptimizeController_Simple(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend);
    NONCOPYABLE(OptimizeController_Simple)

    void doOptimize() override;

private:
    std::shared_ptr<SimpleOptimizer> simpleOp;
    std::shared_ptr<PropagationOptimizer> PropagationOp;
    std::shared_ptr<DeadCodeOptimizer> deadCodeOp;
    std::shared_ptr<ConstantFoldingOptimizer> constFoldOp;
    std::shared_ptr<CommExpOptimizer> commExpOp;
    std::shared_ptr<LoopOptimizer> loopOp;

    const size_t MAX_ROUND = 4;
    const ssize_t MIN_OP_THRESHOLD = 2;
};

// optimizer需要保证，不修改fbegin和fend
// 死代码消除优化，当一个定值后续没有使用时，删除这个定值
class DeadCodeOptimizer
{
public:
    DeadCodeOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend);
    NONCOPYABLE(DeadCodeOptimizer)

    int optimize();

private:
    TACList::iterator _fbegin, _fend;
    TACListPtr _tacls;
    
    std::shared_ptr<ControlFlowGraph> cfg;
    std::shared_ptr<LiveAnalyzer> liveAnalyzer;

    bool hasSideEffect(SymbolPtr defSym, TACPtr tac);
};

// 使用UD和DU链分析器进行优化
class DeadCodeOptimizer_UseDef
{
public:
    DeadCodeOptimizer_UseDef(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend);
    NONCOPYABLE(DeadCodeOptimizer_UseDef)

    int optimize();

private:
    TACList::iterator _fbegin, _fend;
    TACListPtr _tacls;
    
    std::shared_ptr<ControlFlowGraph> cfg;
    std::shared_ptr<UseDefAnalyzer> useDefAnalyzer;

    bool hasSideEffect(SymbolPtr defSym, TACPtr tac);
};


// 常量传播/复写传播优化器
class PropagationOptimizer
{
public:
    PropagationOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend);
    NONCOPYABLE(PropagationOptimizer)

    int optimize();

private:
    std::shared_ptr<ControlFlowGraph> cfg;
    std::shared_ptr<ArrivalAnalyzer> arrivalValAnalyzer;

    TACListPtr tacLs_;
    TACList::iterator fbegin_, fend_;
};


// 公共子表达式删除优化器
class CommExpOptimizer
{
public:
    CommExpOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend);
    NONCOPYABLE(CommExpOptimizer)

    int optimize();

private:
    std::shared_ptr<ControlFlowGraph> cfg;
    std::shared_ptr<CommExpAnalyzer> commExpAnalyzer;

    TACListPtr tacLs_;
    TACList::iterator fbegin_, fend_;
};


//会将一些简单情况优化掉。
// 情况1，+0会被优化成assign
// 情况2，*1会被优化成assign
// 情况3，自己对自己assign会被去掉。
class SimpleOptimizer {
  NONCOPYABLE(SimpleOptimizer)
 public:
  SimpleOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend);

  int optimize();

 private:
  TACList::iterator fbegin_, fend_;
  TACListPtr tacls_;
};

class ConstantFoldingOptimizer {
  NONCOPYABLE(ConstantFoldingOptimizer)
 public:
  ConstantFoldingOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend);

  int optimize();

 private:
  TACList::iterator fbegin_, fend_;
  TACListPtr tacls_;
};

}  // namespace AssemblyBuilder
}