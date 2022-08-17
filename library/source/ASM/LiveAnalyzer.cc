#include "ASM/LiveAnalyzer.hh"
#include "ASM/ControlFlowGraph.hh"
#include "ASM/SymAnalyzer.hh"
#include <stdexcept>
#include <queue>
#include <array>

namespace HaveFunCompiler{
namespace AssemblyBuilder{
using HaveFunCompiler::ThreeAddressCode::TACOperationType;

// bool SymIdxMapping::insert(SymPtr var)
// {
//     if (i2s.size() == i2s.max_size())
//         throw std::runtime_error("too many variables!");
//     if (s2i.find(var) != s2i.end())
//         return false;
//     s2i.emplace(var, i2s.size());
//     i2s.push_back(var);
//     return true;
// }

// std::optional<size_t> SymIdxMapping::getSymIdx(SymPtr ptr) const
// {
//     auto it = s2i.find(ptr);
//     if (it == s2i.end())
//         return std::nullopt;
//     else
//         return it->second;
// }

// std::optional<SymIdxMapping::SymPtr> SymIdxMapping::getSymPtr(size_t idx) const
// {
//     if (idx >= i2s.size())
//         return std::nullopt;
//     else
//         return i2s[idx];
// }

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
        throw std::runtime_error("LiveIntervalAnalyzer logic error: liveIntervalSet of sym is empty");
    else
    {
        endPoints.first = liveIntervalSet.begin()->first;
        endPoints.second = liveIntervalSet.end()->second;
    }
}


LiveAnalyzer::LiveAnalyzer(std::shared_ptr<const ControlFlowGraph> controlFlowGraph) : DataFlowAnalyzerBackWard<LiveInfo>(controlFlowGraph)
{
}

// 活跃信息结点间传递
// 在结点u的入口活跃，则在结点u的所有前驱出口活跃
// 框架调用时，y为x的一个后继，活跃信息从y.in传递到x.out
void LiveAnalyzer::transOp(size_t x, size_t y)
{
    for (auto &e : _in[y])
        _out[x].insert(e);
}

// 活跃信息在结点u内传递 out->in
// in = gen ∪ (out - kill)
// gen = useSet
// kill = defSet
void LiveAnalyzer::transFunc(size_t u)
{
    auto tac = cfg->get_node_tac(u);
    auto gen = tac->getUseSym();
    auto kill = tac->getDefineSym();
    
    _in[u] = _out[u];
    if (_out[u].count(kill))
        _in[u].erase(kill);
    for (auto sym : gen)
        _in[u].insert(sym);
}

UseDefAnalyzer::UseDefAnalyzer(std::shared_ptr<const ControlFlowGraph> controlFlowGraph)
{
    cfg = controlFlowGraph;
    symAnalyzer = std::make_shared<SymAnalyzer>(cfg);
}

const std::unordered_set<SymbolPtr>& UseDefAnalyzer::get_symSet() const
{
    return symAnalyzer->getSymSet();
}

void UseDefAnalyzer::analyze()
{
    auto getPriority = [this](size_t n)
    {
        return SIZE_MAX - cfg->get_node_dfn(n);
    };

    symAnalyzer->analyze();

    auto &symSet = symAnalyzer->getSymSet();
    for (auto sym : symSet)
    {
        // 每个结点的数据流信息，0为入口，1为出口
        // useInfo: 变量sym的使用点集合
        std::unordered_map<size_t, std::array<UseInfo, 2>> nodeUseInfo;

        // 工作集：<结点优先级，结点号>
        std::set<std::pair<size_t, size_t>> workSet;

        // 得到变量的定值集合，使用集合
        auto &defSet = symAnalyzer->getSymDefPoints(sym);
        auto &useSet = symAnalyzer->getSymUsePoints(sym);

        // 初始化工作集
        for (auto n : useSet)
            workSet.emplace(getPriority(n), n);
    
        while (!workSet.empty())
        {
            auto node = workSet.begin()->second;
            workSet.erase(workSet.begin());

            auto &in = nodeUseInfo[node][0];
            auto &out = nodeUseInfo[node][1];

            // 结点间传递
            for (auto sucNode : cfg->get_outNodeList(node))
            {
                auto &sucIn = nodeUseInfo[sucNode][0];
                for (auto n : sucIn)
                    out.insert(n);
            }

            // 结点内传递
            UseInfo oldIn = std::move(in);
            in = out;
            // 如果是一个定值点
            // 杀死sym的所有到达的使用点
            if (defSet.count(node))
                in.clear(); 
            // 如果是一个使用点，添加上
            if (useSet.count(node))
                in.insert(node);
            
            if (in != oldIn)
                for (auto preNode : cfg->get_inNodeList(node))
                    workSet.emplace(getPriority(preNode), preNode);
        }

        for (auto def : defSet)
        {
            UseInfo &usePoints = nodeUseInfo[def][1];
            defUseChain[sym][def];
            for (auto use : usePoints)
            {
                defUseChain[sym][def].insert(use);
                useDefChain[sym][use].insert(def);
            }
        }
    }
}


LiveIntervalAnalyzer::LiveIntervalAnalyzer(std::shared_ptr<ControlFlowGraph> controlFlowGraph) : cfg(controlFlowGraph)
{
    // 遍历流图，得到每个变量的定值和使用集合，函数中出现的所有变量的集合，局部变量的集合
    bfs();

    nodeLiveInfo.resize(cfg->get_nodes_number());

    // 对每个变量，求出它的活跃区间集合
    for (auto sym : symSet)
    {
        std::unordered_set<size_t> &useSet = symUseMap[sym], &defSet = symDefMap[sym];
        auto &symLiveInfo = symLiveMap[sym];
        std::unordered_set<size_t> vis;
        
        // 队列中存放所有可能作为活跃区间终点的结点下标
        std::queue<size_t> startQueue;
        for (auto n : useSet)
            startQueue.push(n);
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

            if (vis.find(cur) != vis.end())  continue;

            LiveInterval interval;
            // 将活跃区间尾更新为当前活跃点，然后开始向上逆dfn序遍历
            interval.second = cfg->get_node_dfn(cur);

            bool flag;

            do
            {
                flag = false;
                vis.emplace(cur);

                // 将活跃区间头更新成当前结点cur
                interval.first = cfg->get_node_dfn(cur);

                // 当前结点cur不是定值，或同时是定值和使用，则应当继续向上遍历
                // 并且将sym加入到cur的入口活跃集合中
                if (defSet.find(cur) == defSet.end() || useSet.find(cur) != useSet.end())
                {
                    size_t nxtcur;
                    auto inNodeLs = cfg->get_inNodeList(cur);
                    nodeLiveInfo[cur].inLive.insert(sym);

                    for (auto u : inNodeLs)
                    {
                        // sym在cur入口活跃，自然在前驱的出口活跃
                        nodeLiveInfo[u].outLive.insert(sym);
                        if (cfg->get_node_dfn(u) + 1 == cfg->get_node_dfn(cur))  // 找到dfn连续的前驱u
                        {
                            // 如果前驱u没有被访问过，继续向上遍历
                            // 同时将sym加入到u的出口活跃集合中
                            if (vis.find(u) == vis.end())
                            {
                                nxtcur = u;
                                flag = true;
                            }
                        }
                        // u不是dfn连续的前驱，而sym必在u活跃
                        // 本次循环只求1个连续区间，u不连续
                        // 所以，当u没访问过时，将其加入活跃点，后续的循环中再求从它开始的活跃区间
                        else if (vis.find(u) == vis.end())
                            startQueue.push(u);
                    }
                    cur = nxtcur;
                }
            } while (flag);

            // 将本次求得的活跃区间合并进该变量的活跃区间集合
            symLiveInfo.addUncoveredLiveInterval(interval);
        }

        // 更新该变量的活跃区间端点(寄存器分配优先级可能使用)
        // symLiveInfo.updateIntervalEndPoint();

        // 保存该变量的定值和引用计数(寄存器分配优先级使用)
        symLiveInfo.defCnt = defSet.size();
        symLiveInfo.useCnt = useSet.size();
    }
}

void LiveIntervalAnalyzer::bfs()
{
    std::vector<bool> vis(cfg->get_nodes_number(), false);
    std::queue<size_t> q;
    q.push(cfg->get_startNode());
    while (!q.empty())
    {
        size_t n = q.front();
        q.pop();

        if (vis[n])  continue;
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

        auto& outLs = cfg->get_outNodeList(n);
        for (auto u : outLs)
            if (!vis[u])
                q.push(u);
    }
}

LiveIntervalAnalyzer::iterator LiveIntervalAnalyzer::get_fbegin() const
{
    return cfg->get_fbegin();
}

LiveIntervalAnalyzer::iterator LiveIntervalAnalyzer::get_fend() const
{
    return cfg->get_fend();
}

}
}