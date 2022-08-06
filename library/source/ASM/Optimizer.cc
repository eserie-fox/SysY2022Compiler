#include "ASM/Optimizer.hh"
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
#include "TAC/Symbol.hh"
#include <vector>

namespace HaveFunCompiler{
namespace AssemblyBuilder{

using namespace ThreeAddressCode;

void DeadCodeOptimizer::optimize()
{
    auto cfg = std::make_shared<ControlFlowGraph>(_fbegin, _fend);
    LiveAnalyzer liveAnalyzer(cfg);
    std::vector<TACList::iterator> deadCodes;

    // test
    cfg->printToDot();

    auto tacNum = cfg->get_nodes_number();
    for (size_t i = 0; i < tacNum; ++i)
    {
        auto tac = cfg->get_node_tac(i);
        auto defSym = tac->getDefineSym();
        if (defSym)
        {
            auto& nodeLiveInfo = liveAnalyzer.get_nodeLiveInfo(i);
            if (nodeLiveInfo.outLive.find(defSym) == nodeLiveInfo.outLive.end() && !hasSideEffect(defSym, tac))
                deadCodes.push_back(cfg->get_node_itr(i));
        }
    }

    for (auto it : deadCodes)
        _tacls->erase(it);
}

bool DeadCodeOptimizer::hasSideEffect(SymbolPtr defSym, TACPtr tac)
{
    if (defSym->IsGlobal())  
        return true;
    if (tac->operation_ == TACOperationType::Call || tac->operation_ == TACOperationType::Parameter)
        return true;
    // if (tac->operation_ == TACOperationType::Constant || tac->operation_ == TACOperationType::Variable)
    // {
    //     auto liveInfo = la.get_symLiveInfo(defSym);
    //     if (liveInfo->useCnt != 0)
    //         return true;
    //     else
    //         return false;
    // }
    return false;
}

}
}