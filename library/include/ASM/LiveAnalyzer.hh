#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <list>
#include <memory>
#include <optional>
#include "ASM/Common.hh"
#include "MacroUtil.hh"

namespace HaveFunCompiler {
namespace ThreeAddressCode {
enum class TACOperationType;
}
}  // namespace HaveFunCompiler

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

using LiveInterval = std::pair<size_t, size_t>;

// 变量的活跃信息
struct SymLiveInfo
{
    // 活跃区间集合
    // 保证集合中活跃区间之间一定有断点
    std::set<LiveInterval> liveIntervalSet;

    // 活跃区间端点的最小值和最大值
    std::pair<size_t, size_t> endPoints;

    // 定值计数，使用计数
    size_t defCnt, useCnt;

    // bool operator<(const SymLiveInfo &o) const
    // {
    //     if (high > o.high)  return true;
    //     else if (high == o.high && low < o.low)  return true;
    //     else  return false;
    // }

    // 加入一个不与现有活跃区间存在覆盖范围的区间
    // 理论上由活跃分析器保证不覆盖
    // 若存在覆盖，抛出异常
    void addUncoveredLiveInterval(LiveInterval &interval);

    // 更新区间端点信息(low,high)
    void updateIntervalEndPoint();
};

class LiveAnalyzer
{
public:

    using SymPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::Symbol>;
    using iterator = std::list<std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>>::iterator;
    using const_iterator = std::list<std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>>::const_iterator;

    // 控制流图中每个结点的活跃信息
    struct NodeLiveInfo
    {
        // 入口活跃集合
        // 出口活跃集合
        std::unordered_set<SymPtr> inLive, outLive;

        // 结点活跃gen和kill集合
    };

public:

    NONCOPYABLE(LiveAnalyzer)
    LiveAnalyzer(std::shared_ptr<ControlFlowGraph> controlFlowGraph);

    iterator get_fbegin() const;
    iterator get_fend() const;

    const SymLiveInfo* get_symLiveInfo(SymPtr sym) const
    {
        if (symLiveMap.find(sym) == symLiveMap.end())
            return nullptr;
        return &symLiveMap.at(sym);
    }

    const NodeLiveInfo& get_nodeLiveInfo(size_t u) const
    {
        return nodeLiveInfo[u];
    }

    // 得到函数中出现的所有变量的集合
    const std::unordered_set<SymPtr>& get_allSymSet() const
    {
        return symSet;
    }

private:

    // 流图结点上的活跃信息
    std::vector<NodeLiveInfo> nodeLiveInfo;
    // 所有变量对应的活跃信息
    std::unordered_map<SymPtr, SymLiveInfo> symLiveMap;
    // 变量的定值点和使用点集合（集合元素是流图中结点下标）
    std::unordered_map<SymPtr, std::unordered_set<size_t>> symDefMap, symUseMap;
    // 所有出现的变量的集合
    std::unordered_set<SymPtr> symSet;  
    // 控制流图
    std::shared_ptr<ControlFlowGraph> cfg;


    // 遍历流图，得到每个变量的定值和使用集合，函数中出现的所有变量的集合
    void bfs();

};


}
}