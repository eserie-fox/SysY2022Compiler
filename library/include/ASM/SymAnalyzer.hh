#pragma once

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>
#include "ASM/Common.hh"
#include "MacroUtil.hh"

namespace HaveFunCompiler{
namespace AssemblyBuilder{

class ControlFlowGraph;

// 变量(std::shared_ptr<Symbol>)与整数下标(size_t)的映射
class SymIdxMapping
{
private:
    using SymPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::Symbol>;

    std::unordered_map<SymPtr, size_t> s2i;
    std::vector<SymPtr> i2s;

public:
    SymIdxMapping() {}
    bool insert(SymPtr var);
    std::optional<size_t> getSymIdx(SymPtr ptr) const;
    std::optional<SymPtr> getSymPtr(size_t idx) const;
};

// 负责获取变量的使用点和定值点
class SymAnalyzer
{
public:
    SymAnalyzer(std::shared_ptr<const ControlFlowGraph> controlFlowGraph);
    NONCOPYABLE(SymAnalyzer)

    void analyze();

    inline const std::set<size_t>& getSymDefPoints(SymbolPtr sym) const
    {
        return symDefMap.at(sym);
    }

    inline const std::set<size_t>& getSymUsePoints(SymbolPtr sym) const
    {
        return symUseMap.at(sym);
    }

    inline const std::unordered_set<SymbolPtr>& getSymSet() const
    {
        return symSet;
    }

    size_t getSymId(SymbolPtr sym) const
    {
        return symIdxMap.getSymIdx(sym).value();
    }

    SymbolPtr getIdSym(size_t id) const
    {
        return symIdxMap.getSymPtr(id).value();
    }

private:

    // 变量的定值点和使用点集合（集合元素是流图中结点下标）
    std::unordered_map<SymbolPtr, std::set<size_t>> symDefMap, symUseMap;
    // 所有出现的变量的集合
    std::unordered_set<SymbolPtr> symSet;  
    // 控制流图
    std::shared_ptr<const ControlFlowGraph> cfg;
    // 变量id映射
    SymIdxMapping symIdxMap;
};


}
}