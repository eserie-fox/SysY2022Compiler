#pragma once

#include "ASM/DataFlowAnalyzer.hh"
#include <unordered_map>
#include <unordered_set>

namespace HaveFunCompiler{
namespace AssemblyBuilder{

class ControlFlowGraph;
class SymAnalyzer;

using ArrivalInfo = std::unordered_set<size_t>;
class ArrivalAnalyzer : public DataFlowAnalyzerForward<ArrivalInfo>
{
public:
    ArrivalAnalyzer(std::shared_ptr<const ControlFlowGraph> controlFlowGraph, std::shared_ptr<const SymAnalyzer> _symAnalyzer);
    NONCOPYABLE(ArrivalAnalyzer)
    
    using UseDefChain_T = std::unordered_map<SymbolPtr, std::unordered_map<size_t, std::unordered_set<size_t>>>;

    void updateUseDefChain();
    const UseDefChain_T& get_useDefChain() const { return useDefChain; }

private:
    void transOp(size_t x, size_t y) override;
    void transFunc(size_t u) override;

    std::shared_ptr<const SymAnalyzer> symAnalyzer;

    UseDefChain_T useDefChain;
};


}
}