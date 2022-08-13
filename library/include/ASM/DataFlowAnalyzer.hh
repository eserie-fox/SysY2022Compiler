#pragma once

#include <memory>
#include <vector>
#include <set>
#include <queue>
#include <unordered_map>
#include "MacroUtil.hh"
#include "ASM/ControlFlowGraph.hh"

namespace HaveFunCompiler{
namespace AssemblyBuilder{

// class TopSorter
// {
// public:
//     TopSorter(std::shared_ptr<const ControlFlowGraph> _cfg) : cfg(_cfg) {}
//     NONCOPYABLE(TopSorter)

//     enum Dir {FORWARD, BACKWARD};

//     std::vector<size_t> sort(Dir dir) const;

// private:
//     std::shared_ptr<const ControlFlowGraph> cfg;
// };

template <typename S>
class DataFlowAnalyzer
{
public:
    DataFlowAnalyzer(std::shared_ptr<const ControlFlowGraph> controlFlowGraph) : 
        cfg(controlFlowGraph) 
    {
        auto siz = controlFlowGraph->get_nodes_number();
        _in.reserve(siz);
        _out.reserve(siz);
    }
    virtual ~DataFlowAnalyzer() = default;

    NONCOPYABLE(DataFlowAnalyzer)

    // 从工作集workList开始数据流分析，直到收敛
    virtual void analyze(std::vector<size_t> &workList) = 0;
    virtual void analyze() = 0;  // 默认所有可达结点为工作集

    const S& getIn(size_t n) const
    {
        return _in.at(n);
    }

    const S& getOut(size_t n) const
    {
        return _out.at(n);
    }

protected:

    // 结点间传递的算子
    virtual void transOp(size_t x, size_t y) = 0;

    // 结点内的传递函数
    virtual void transFunc(size_t u) = 0;

    std::shared_ptr<const ControlFlowGraph> cfg;
    std::unordered_map<size_t, S> _in, _out;   // 每个结点的数据流信息
};

// 前向数据流分析框架
template <typename S>
class DataFlowAnalyzerForward : public DataFlowAnalyzer<S>
{
public:
    DataFlowAnalyzerForward(std::shared_ptr<const ControlFlowGraph> controlFlowGraph) :
        DataFlowAnalyzer<S>(controlFlowGraph)
    {
        // 以dfs序为伪拓扑序
        for (auto i = this->cfg->beginIdx(); i != this->cfg->endIdx(); i = this->cfg->nextIdx(i))
            priority.emplace(i, this->cfg->get_node_dfn(i));
    }

    NONCOPYABLE(DataFlowAnalyzerForward)

    void analyze(std::vector<size_t> &workList) override
    {
        std::set<std::pair<size_t, size_t>> workSet;
        for (auto n : workList)
            workSet.emplace(priority[n], n);
        do_analyze(workSet);
    }

    void analyze() override
    {
        std::set<std::pair<size_t, size_t>> workSet;
        for (auto &e : priority)
            workSet.emplace(e.second, e.first);
        do_analyze(workSet);
    }

private:
    std::unordered_map<size_t, size_t> priority;   // 结点n的计算优先级（拓扑序）map<n, pri>

    void do_analyze(std::set<std::pair<size_t, size_t>> &workSet)
    {
        while (!workSet.empty())
        {
            auto node = (*workSet.begin()).second;
            workSet.erase(workSet.begin());
            auto old = std::move(this->_out[node]);
            auto &inls = this->cfg->get_inNodeList(node);
            for (auto pre : inls)
                this->transOp(node, pre);
            this->transFunc(node);
            if (this->_out[node] != old)
            {
                auto &outls = this->cfg->get_outNodeList(node);
                for (auto suc : outls)
                {
                    auto tmp = std::make_pair(priority[suc], suc);
                    if (workSet.count(tmp) == 0)
                        workSet.insert(tmp);
                }
            }
        }
    }

};

// 后向数据流分析框架
template <typename S>
class DataFlowAnalyzerBackWard : public DataFlowAnalyzer<S>
{
public:
    DataFlowAnalyzerBackWard(std::shared_ptr<const ControlFlowGraph> controlFlowGraph) : 
        DataFlowAnalyzer<S>(controlFlowGraph)
    {
        // 以逆dfs序为伪拓扑序
        for (auto i = this->cfg->beginIdx(); i != this->cfg->endIdx(); i = this->cfg->nextIdx(i))
            priority.emplace(i, SIZE_MAX - this->cfg->get_node_dfn(i));
    }

    NONCOPYABLE(DataFlowAnalyzerBackWard)

    void analyze(std::vector<size_t> &workList) override
    {
        std::set<std::pair<size_t, size_t>> workSet;
        for (auto n : workList)
            workSet.emplace(priority[n], n);
        do_analyze(workSet);
    }

    void analyze() override
    {
        std::set<std::pair<size_t, size_t>> workSet;
        for (auto &e : priority)
            workSet.emplace(e.second, e.first);
        do_analyze(workSet);
    }

private:
    std::unordered_map<size_t, size_t> priority;

    void do_analyze(std::set<std::pair<size_t, size_t>> &workSet)
    {
        while (!workSet.empty())
        {
            auto node = (*workSet.begin()).second;
            workSet.erase(workSet.begin());
            auto old = std::move(this->_in[node]);
            auto &outls = this->cfg->get_outNodeList(node);
            for (auto suc : outls)
                this->transOp(node, suc);
            this->transFunc(node);
            if (this->_in[node] != old)
            {
                auto &inls = this->cfg->get_inNodeList(node);
                for (auto pre : inls)
                {
                    auto tmp = std::make_pair(priority[pre], pre);
                    if (workSet.count(tmp) == 0)
                        workSet.insert(tmp);
                }
            }
        }
    }

};

}
}