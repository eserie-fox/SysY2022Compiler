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
    if (liveIntervalSet.empty())
        throw std::runtime_error("LiveAnalyzer logic error: liveIntervalSet of sym is empty");
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
    bfs(cfg->get_startNode(), vis);

    // 对每个变量，求出它的活跃区间集合
    for (auto sym : symSet)
    {
        std::unordered_set<size_t> &useSet = symUseMap[sym], &defSet = symDefMap[sym];
        auto &symLiveInfo = symLiveMap[sym];
        for (size_t i = 0; i < vis.size(); ++i)
            vis[i] = false;
        
        // 队列中存放所有可能作为活跃区间终点的结点下标
        std::queue<size_t> startQueue;
        for (auto n : useSet)
            startQueue.push(n);
        // 目前先将单独的定值点(定值后没有使用的定值)作为一个活跃区间处理（优化后就不存在了）
        for (auto n : defSet)  
            startQueue.push(n);

        // 一次循环，从一个终点(endDfn)开始，向上遍历，求出一个连续的活跃区间[startDfn, endDfn]
        while (!startQueue.empty())
        {
            // 从一个活跃点开始逆dfs序遍历
            // dfn连续的前驱结点已经处理过则停止（处理过说明该结点已经包含在其他活跃区间）
            // 遇到定值结点，如果该点不同时是使用结点，则停止
            // 对于dfn不连续的前驱结点（该前驱结点存在多个后继时会出现），将该结点加入队列，但不从该结点继续向上遍历
            size_t cur = startQueue.front();
            startQueue.pop();

            if (vis[cur])  continue;

            LiveInterval interval;
            // 将活跃区间尾更新为当前活跃点，然后开始向上逆dfn序遍历
            interval.second = cfg->get_node_dfn(cur);

            bool flag;

            do
            {
                flag = false;
                vis[cur] = true;

                // 将活跃区间头更新成当前结点cur
                interval.first = cfg->get_node_dfn(cur);

                // 当前结点cur不是定值，或同时是定值和使用，则应当继续向上遍历
                if (defSet.find(cur) == defSet.end() || useSet.find(cur) != useSet.end())
                {
                    size_t nxtcur;
                    auto inNodeLs = cfg->get_inNodeList(cur);
                    for (auto u : inNodeLs)
                    {
                        if (cfg->get_node_dfn(u) + 1 == cfg->get_node_dfn(cur))  // 找到dfn连续的前驱u
                        {
                            if (!vis[u])  // 如果前驱没有被访问过，继续向上遍历
                            {
                                nxtcur = u;
                                flag = true;
                            }
                        }
                        // u不是dfn连续的前驱，而sym必在u活跃
                        // 本次循环只求1个连续区间，u不连续
                        // 所以，当u没访问过时，将其加入活跃点，后续的循环中再求从它开始的活跃区间
                        else if (!vis[u])
                            startQueue.push(u);
                    }
                    cur = nxtcur;
                }
            } while (flag);

            // 将本次求得的活跃区间合并进该变量的活跃区间集合
            symLiveInfo.addUncoveredLiveInterval(interval);
        }

        // 更新该变量的活跃区间端点(寄存器分配优先级可能使用)
        symLiveInfo.updateIntervalEndPoint();

        // 保存该变量的定值和引用计数(寄存器分配优先级可能使用)
        symLiveInfo.defCnt = defSet.size();
        symLiveInfo.useCnt = useSet.size();
    }
}

void LiveAnalyzer::bfs(size_t start, std::vector<bool> &vis)
{
    std::queue<size_t> q;
    q.push(start);
    while (!q.empty())
    {
        size_t n = q.front();
        q.pop();
        vis[n] = 1;

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
            if (!vis[u])
                q.push(u);
    }
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