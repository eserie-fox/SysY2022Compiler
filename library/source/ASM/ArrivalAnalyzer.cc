#include "ASM/ArrivalAnalyzer.hh"
#include "ASM/SymAnalyzer.hh"

namespace HaveFunCompiler{
namespace AssemblyBuilder{


ArrivalAnalyzer::ArrivalAnalyzer(std::shared_ptr<ControlFlowGraph> controlFlowGraph, std::shared_ptr<SymAnalyzer> _symAnalyzer) : 
    DataFlowAnalyzerForward<ArrivalInfo>(controlFlowGraph), symAnalyzer(_symAnalyzer)
{
}

// 到达定值信息结点间传递
// 定值到达结点u的出口，则到达它每一个后继的入口
// 框架调用时, x为y的后继，信息从y传递到x
void ArrivalAnalyzer::transOp(size_t x, size_t y)
{
    for (auto &e : _out[y])
        _in[x].insert(e);
}

// 到达定值信息结点内传递
// out = gen ∪ (in - kill)
// gen = 若当前有定值，则为当前结点id
// kill = 所有其他对当前变量定值的结点id
void ArrivalAnalyzer::transFunc(size_t u)
{
    _out[u] = _in[u];
    auto def = cfg->get_node_tac(u)->getDefineSym();
    if (def)
    {
        auto defSet = symAnalyzer->getSymDefPoints(def);
        for (auto n : defSet)
            _out[u].erase(n);
        _out[u].emplace(u);
    }
}

}
}