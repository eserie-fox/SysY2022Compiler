#pragma once

#include <vector>
#include <unordered_map>
#include <set>
#include <memory>
#include <optional>
#include "ASM/Common.hh"


namespace HaveFunCompiler{
namespace AssemblyBuilder{


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


class LiveAnalyzer
{
public:

    using SymPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::Symbol>;

    using LiveInterval = std::pair<size_t, size_t>;

    // 控制流图中每个结点的活跃信息
    struct NodeLiveInfo
    {
        // 入口活跃集合
        // 出口活跃集合

        // 结点活跃gen和kill集合
        SymPtr gen;
        std::vector<SymPtr> kill;
    };

    // 变量的活跃信息
    struct SymLiveInfo
    {
        // 活跃区间集合
        std::set<LiveInterval> liveIntervalSet;

        // 活跃区间端点的最小值和最大值
        size_t low, high;

        bool operator<(const SymLiveInfo &o) const
        {
            if (high > o.high)  return true;
            else if (high == o.high && low < o.low)  return true;
            else  return false;
        }
    };

private:



};


}
}