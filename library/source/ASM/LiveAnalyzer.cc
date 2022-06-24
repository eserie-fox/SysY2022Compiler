#include "ASM/LiveAnalyzer.hh"
#include "ASM/ControlFlowGraph.hh"
#include <stdexcept>
#include <queue>

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

void SymLiveInfo::addUncoveredLiveInterval(LiveInterval &interval)
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

void SymLiveInfo::updateIntervalEndPoint()
{
    // dfn从1开始，用0标识该变量不活跃
    if (liveIntervalSet.empty())
        endPoints.first = endPoints.second = 0;
    else
    {
        endPoints.first = liveIntervalSet.begin()->first;
        endPoints.second = liveIntervalSet.end()->second;
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
        std::unordered_set<size_t> &useSet = symUseMap[sym], &defSet = symDefMap[sym];
        for (size_t i = 0; i < vis.size(); ++i)
            vis[i] = false;
        
        // 队列中存放所有可能作为活跃区间终点的结点下标
        std::queue<size_t> startQueue;
        for (auto n : useSet)
        {
            startQueue.push(n);
            vis[n] = true;
        }

        // 一次循环，由一个终点(endDfn)，求出一个连续的活跃区间[startDfn, endDfn]
        while (!startQueue.empty())
        {
            // 从一个活跃点开始逆dfs序遍历
            // 遇到已经处理过的结点则停止（处理过说明该结点已经包含在其他活跃区间）
            // 遇到定值结点，更新interval的first为该定值结点后停止
            // 对于dfn不连续的前驱结点（该前驱结点存在多个后继时会出现），将该结点加入队列，但不从该结点继续向上遍历
            size_t cur = startQueue.front();
            startQueue.pop();

            LiveInterval interval(cfg->get_node_dfn(cur), cfg->get_node_dfn(cur));
            bool flag;

            do
            {
                flag = false;
                auto inNodeLs = cfg->get_inNodeList(cur);
                for (auto u : inNodeLs)
                {
                    if (cfg->get_node_dfn(u) + 1 == cfg->get_node_dfn(cur))  // u是dfn连续的前驱
                    {
                        if (!vis[u])
                        {
                            vis[u] = true;  
                            interval.first = cfg->get_node_dfn(u);  // 更新interval的first为dfn(u)
                            cur = u;

                            if (defSet.find(u) == defSet.end())
                                flag = true;  // 只有存在一个u没有被访问，且u不是定值点时，才继续向前遍历
                        }
                    }
                    else if (!vis[u])  // u不是dfn连续的前驱，而sym必在u活跃，所以u加入队列
                    {
                        vis[u] = true;
                        startQueue.push(u);
                    }
                }
            } while (flag);

            // 将本次求得的活跃区间合并进该变量的活跃区间集合
            symLiveMap[sym].addUncoveredLiveInterval(interval);
        }

        // 更新该变量的活跃区间端点(寄存器分配排序使用)
        symLiveMap[sym].updateIntervalEndPoint();
    }
}

void LiveAnalyzer::dfs(size_t n, std::vector<bool> &vis)
{
    if (vis[n])
        return;
    vis[n] = true;

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

    auto outLs = cfg->get_outNodeList(n);
    for (auto u : outLs)
        dfs(u, vis);
}

LiveAnalyzer::iterator LiveAnalyzer::get_fbegin() const
{
    return cfg->get_fbegin();
}

LiveAnalyzer::iterator LiveAnalyzer::get_fend() const
{
    return cfg->get_fend();
}

}
}