#include "ASM/LiveAnalyzer.hh"
#include "ASM/ControlFlowGraph.hh"
#include "TAC/ThreeAddressCode.hh"
#include <stdexcept>

namespace HaveFunCompiler{
namespace AssemblyBuilder{
using HaveFunCompiler::ThreeAddressCode::TACOperationType;

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

void LiveAnalyzer::SymLiveInfo::addUncoveredLiveInterval(LiveInterval &interval)
{
    using itr = std::set<LiveInterval>::iterator;

    // a < b 尝试merge a和b
    // 若能merge，返回指向merge后的元素的迭代器，否则返回b
    auto merge = [this](itr a, itr b)
    {
        if (a->second + 1 == b->first)
        {
            size_t tmpfirst = a->first;
            size_t tmpsecond = b->second;
            this->liveIntervalSet.erase(a);
            this->liveIntervalSet.erase(b);
            auto res = this->liveIntervalSet.emplace(tmpfirst, tmpsecond);
            if (res.second == false)
                throw std::runtime_error("LiveInterval merge fault");
            return res.first;
        }
        return b;
    };

    // 将新区间插入活跃区间集合
    auto cur = liveIntervalSet.insert(interval).first;

    // 如果存在前驱区间(tmp指向前驱)，则尝试合并
    auto tmp = cur;
    if (tmp != liveIntervalSet.begin())
    {
        --tmp;
        if (tmp->second >= cur->first)
            throw std::runtime_error("LiveInterval cover fault");
        cur = merge(tmp, cur);
    }

    // 同样，尝试合并后继区间(tmp指向后继)
    tmp = cur;
    ++tmp;
    if (tmp != liveIntervalSet.end())
    {
        if (tmp->first <= cur->second)
            throw std::runtime_error("LiveInterval cover fault");
        merge(cur, tmp);
    }
}


LiveAnalyzer::LiveAnalyzer(std::shared_ptr<ControlFlowGraph> controlFlowGraph) : cfg(controlFlowGraph)
{

    std::vector<bool> vis(cfg->get_nodes_number(), false);

    // 遍历流图，得到每个变量的定值和使用集合，函数中出现的所有变量的集合，局部变量的集合
    dfs(cfg->get_startNode(), vis);

    // 对每个变量，求出它的活跃区间集合
    for (auto sym : symSet)
    {

    }
}

void LiveAnalyzer::dfs(size_t n, std::vector<bool> &vis)
{
    if (vis[n])
        return;
    vis[n] = true;

    size_t dfn = cfg->get_node_dfn(n);
    auto tac = cfg->get_node_tac(n);

    // 定值变量处理：加入函数中出现的变量集合，加入该变量的定值集合，如果是声明，则加入局部变量集合
    auto defSym = tac->getDefineSym(); 
    if (defSym)
    {
        if (isDecl(tac->operation_))
            localSym.insert(defSym);
        symSet.insert(defSym);
        symDefMap[defSym].insert(dfn);
    }

    // 使用变量列表处理：对列表中每个使用变量，加入函数中出现的变量集合，加入该变量的使用集合
    auto useSymLs = tac->getUseSym();
    for (auto s : useSymLs)
    {
        symSet.insert(s);
        symUseMap[s].insert(dfn);
    }

    auto outLs = cfg->get_outNodeList(n);
    for (auto u : outLs)
        dfs(u, vis);
}

bool LiveAnalyzer::isDecl(TACOperationType op)
{
    switch (op)
    {
    case TACOperationType::Variable:
    case TACOperationType::Constant:
    case TACOperationType::Parameter:
        return true;
        break;
    
    default:
        return false;
        break;
    }
}

}
}