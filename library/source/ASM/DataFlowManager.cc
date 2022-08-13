#include "ASM/DataFlowManager.hh"
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
#include "ASM/ArrivalAnalyzer.hh"
#include "ASM/SymAnalyzer.hh"

namespace HaveFunCompiler{
namespace AssemblyBuilder{

DataFlowManager::DataFlowManager(TACList::iterator fbegin, TACList::iterator fend) : fbegin_(fbegin), fend_(fend)
{
    cfg = std::make_shared<ControlFlowGraph>(fbegin, fend);
    liveAnalyzer = std::make_shared<LiveAnalyzer>(cfg);
    symAnalyzer = std::make_shared<SymAnalyzer>(cfg);
    arrivalAnalyzer = std::make_shared<ArrivalAnalyzer>(cfg, symAnalyzer);

    liveAnalyzer->analyze();
    symAnalyzer->analyze();
    arrivalAnalyzer->analyze();
}

DataFlowManager_simple::DataFlowManager_simple(TACList::iterator fbegin, TACList::iterator fend) :
    DataFlowManager(fbegin, fend)
{
}

void DataFlowManager_simple::update([[maybe_unused]] size_t n)
{
}

void DataFlowManager_simple::remove([[maybe_unused]] size_t n)
{
}

void DataFlowManager_simple::add([[maybe_unused]] TACList::iterator it)
{
}

void DataFlowManager_simple::commit()
{
    auto fbegin = cfg->get_fbegin();
    auto fend = cfg->get_fend();

    cfg = std::make_shared<ControlFlowGraph>(fbegin, fend);
    liveAnalyzer = std::make_shared<LiveAnalyzer>(cfg);
    symAnalyzer = std::make_shared<SymAnalyzer>(cfg);
    arrivalAnalyzer = std::make_shared<ArrivalAnalyzer>(cfg, symAnalyzer);

    liveAnalyzer->analyze();
    symAnalyzer->analyze();
    arrivalAnalyzer->analyze();
}

}
}