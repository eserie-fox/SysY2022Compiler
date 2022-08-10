#pragma once

#include "ASM/Common.hh"
#include "TAC/ThreeAddressCode.hh"
#include "MacroUtil.hh"

namespace HaveFunCompiler{
namespace AssemblyBuilder{

using namespace ThreeAddressCode;

class ControlFlowGraph;
class LiveAnalyzer;
class ArrivalAnalyzer;
class SymAnalyzer;

// 优化器改变代码时，通知DataFlowManager更新数据流信息
// 数据流信息的更新可以是异步的，由commit触发(数据流分析时，信息能并行计算，所以允许将多次更新合并)
// 即，不保证操作完成后，优化器读取数据流信息能够读到最新值(包括流图、各分析器结果等)，除非commit后再读
class DataFlowManager
{
public:
    DataFlowManager(TACList::iterator fbegin, TACList::iterator fend);

    // 修改了结点n对应的三地址码
    virtual void update(size_t n) = 0;

    // 删除了结点n对应的tac
    virtual void remove(size_t n) = 0;

    // 增加了一条指令，位于it位置
    virtual void add(TACList::iterator it) = 0;

    // 将之前的改动全部提交
    virtual void commit() = 0;

    std::shared_ptr<const ControlFlowGraph> get_controlFlowGraph() const
    {
        return cfg;
    }

    std::shared_ptr<const LiveAnalyzer> get_liveAnalyzer() const
    {
        return liveAnalyzer;
    }

    std::shared_ptr<const ArrivalAnalyzer> get_arrivalAnalyzer() const
    {
        return arrivalAnalyzer;
    }

protected:
    std::shared_ptr<ControlFlowGraph> cfg;
    std::shared_ptr<LiveAnalyzer> liveAnalyzer;
    std::shared_ptr<ArrivalAnalyzer> arrivalAnalyzer;
    std::shared_ptr<SymAnalyzer> symAnalyzer;
};


class DataFlowManager_simple : public DataFlowManager
{
public:
    DataFlowManager_simple(TACList::iterator fbegin, TACList::iterator fend);
    NONCOPYABLE(DataFlowManager_simple)

    void update(size_t n) override;
    void remove(size_t n) override;
    void add(TACList::iterator it) override;
    void commit() override;
};


}
}