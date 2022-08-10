#pragma once

#include "ASM/Common.hh"
#include "MacroUtil.hh"
#include <unordered_map>
#include <unordered_set>

namespace HaveFunCompiler{
namespace AssemblyBuilder{

class ControlFlowGraph;

// 负责获取变量的使用点和定值点
class SymAnalyzer
{
public:
    SymAnalyzer(std::shared_ptr<ControlFlowGraph> controlFlowGraph);
    NONCOPYABLE(SymAnalyzer)

    void analyze();

    inline const std::unordered_set<size_t>& getSymDefPoints(SymbolPtr sym) const
    {
        return symDefMap.at(sym);
    }

    inline const std::unordered_set<size_t>& getSymUsePoints(SymbolPtr sym) const
    {
        return symUseMap.at(sym);
    }

private:

    // 变量的定值点和使用点集合（集合元素是流图中结点下标）
    std::unordered_map<SymbolPtr, std::unordered_set<size_t>> symDefMap, symUseMap;
    // 所有出现的变量的集合
    std::unordered_set<SymbolPtr> symSet;  
    // 控制流图
    std::shared_ptr<ControlFlowGraph> cfg;
};


}
}