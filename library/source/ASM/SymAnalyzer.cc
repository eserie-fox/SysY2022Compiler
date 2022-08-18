#include "ASM/SymAnalyzer.hh"
#include "ASM/ControlFlowGraph.hh"
#include <queue>

namespace HaveFunCompiler{
namespace AssemblyBuilder{

bool SymIdxMapping::insert(SymPtr var)
{
    if (i2s.size() == i2s.max_size())
        throw std::runtime_error("too many variables!");
    if (s2i.find(var) != s2i.end())
        return false;
    s2i.emplace(var, i2s.size());
    i2s.push_back(var);
    return true;
}

std::optional<size_t> SymIdxMapping::getSymIdx(SymPtr ptr) const
{
    auto it = s2i.find(ptr);
    if (it == s2i.end())
        return std::nullopt;
    else
        return it->second;
}

std::optional<SymIdxMapping::SymPtr> SymIdxMapping::getSymPtr(size_t idx) const
{
    if (idx >= i2s.size())
        return std::nullopt;
    else
        return i2s[idx];
}

SymAnalyzer::SymAnalyzer(std::shared_ptr<const ControlFlowGraph> controlFlowGraph) : cfg(controlFlowGraph)
{  }

void SymAnalyzer::analyze()
{
    FOR_EACH_NODE(n, cfg)
    {
        auto tac = cfg->get_node_tac(n);

        // 定值变量处理：加入函数中出现的变量集合，加入该变量的定值集合，如果是声明，则加入局部变量集合
        auto defSym = tac->getDefineSym(); 
        if (defSym)
        {
            symSet.insert(defSym);
            symDefMap[defSym].insert(n);
        }

        // 使用变量列表处理：对列表中每个使用变量，加入函数中出现的变量集合，加入该变量的使用集合
        auto useSymLs = tac->getUseSym();
        for (auto s : useSymLs)
        {
            symSet.insert(s);
            symUseMap[s].insert(n);
        }
    }

    // bug: 全局变量可能不在函数内被定值，Defmap.at抛出异常
    // 所有变量都加入集合
    for (auto sym : symSet)
    {
        if (symDefMap.count(sym) == 0)
            symDefMap.emplace(sym, std::unordered_set<size_t>());
        if (symUseMap.count(sym) == 0)
            symUseMap.emplace(sym, std::unordered_set<size_t>());
        // 添加变量id映射
        symIdxMap.insert(sym);
    }
}

}
}