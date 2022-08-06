#include "ASM/Optimizer.hh"
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
#include "TAC/Symbol.hh"
#include <vector>

namespace HaveFunCompiler{
namespace AssemblyBuilder{

void DeadCodeOptimizer::optimize()
{
    auto cfg = std::make_shared<ControlFlowGraph>(_fbegin, _fend);
    LiveAnalyzer liveAnalyzer(cfg);
    std::vector<TACList::iterator> deadCodes;

    auto tacNum = cfg->get_nodes_number();
    for (size_t i = 0; i < tacNum; ++i)
    {
        auto tac = cfg->get_node_tac(i);
        auto defSym =  tac->getDefineSym();
        if (defSym && !defSym->IsGlobal())
        {
            auto& nodeLiveInfo = liveAnalyzer.get_nodeLiveInfo(i);
            if (nodeLiveInfo.outLive.find(defSym) == nodeLiveInfo.outLive.end() && tac->operation_ != ThreeAddressCode::TACOperationType::Parameter)
                deadCodes.push_back(cfg->get_node_itr(i));
        }
    }

    for (auto it : deadCodes)
        _tacls->erase(it);
}

}
}