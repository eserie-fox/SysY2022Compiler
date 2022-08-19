#pragma once

#include <ext/rope>
#include <vector>
#include <cassert>
#include <iostream>

namespace HaveFunCompiler {

template<typename IndexType = size_t>
class RopeContainer
{
public:
    using elementType = uint64_t;

    RopeContainer() = default;
    RopeContainer(const RopeContainer &o)  { ropeBitmap = o.ropeBitmap; }
    RopeContainer(RopeContainer &&o) { ropeBitmap = o.ropeBitmap; }
    RopeContainer(size_t elementNumber) : 
        ropeBitmap(FIR(elementNumber) + (SEC(elementNumber) ? 1 : 0), 0) {}

    RopeContainer &operator=(const RopeContainer &o) { ropeBitmap = o.ropeBitmap; return *this; }
    RopeContainer &operator=(RopeContainer &&o) { ropeBitmap = o.ropeBitmap; return *this; }

    bool operator==(const RopeContainer &o) const { return ropeBitmap == o.ropeBitmap; }
    bool operator!=(const RopeContainer &o) const { return ropeBitmap != o.ropeBitmap; } 

    RopeContainer &operator|=(const RopeContainer &o)
    {
        auto it = o.ropeBitmap.begin();
        for (size_t i = 0; i < ropeBitmap.size() && i < o.ropeBitmap.size(); ++i, ++it)
        {
            auto s1 = ropeBitmap[i], s2 = o.ropeBitmap[i];
            if (s1 != s2)
                ropeBitmap.replace(i, s1 | s2);
        }
        if (it != o.ropeBitmap.end())
        {
            ropeBitmap.append(it, o.ropeBitmap.end());
        }
        return *this;
    }

    RopeContainer &operator&=(const RopeContainer &o)
    {
        size_t i = 0;
        for (; i < ropeBitmap.size() && i < o.ropeBitmap.size(); ++i)
        {
            auto s1 = ropeBitmap[i], s2 = o.ropeBitmap[i];
            if (s1 != s2)
                ropeBitmap.replace(i, s1 & s2);
        }
        if (i < ropeBitmap.size())
        {
            ropeBitmap.erase(i, ropeBitmap.size() - i);
        }
        return *this;
    }

    void set(IndexType id)
    {
        assert(FIR(id) < ropeBitmap.size());
        auto oldS = ropeBitmap[FIR(id)];
        auto newS = set_bit(oldS, SEC(id));
        if (newS != oldS)
            ropeBitmap.replace(FIR(id), newS);
    }

    void set(const std::vector<IndexType> &ids)
    {
        std::unordered_map<elementType, elementType> oldMap, newMap;
        for (auto n : ids)
        {
            assert(FIR(n) < ropeBitmap.size());
            auto idx = FIR(n);
            if (oldMap.count(idx) == 0)
            {
                oldMap[idx] = ropeBitmap[idx];
                newMap[idx] = oldMap[idx]; 
            }
            newMap[idx] = set_bit(newMap[idx], SEC(n));
        }
        for (auto [i, newS] : newMap)
            if (newS != oldMap[i])
                ropeBitmap.replace(i, newS);
    }

    void reset(IndexType id)
    {
        assert(FIR(id) < ropeBitmap.size());
        auto oldS = ropeBitmap[FIR(id)];
        auto newS = reset_bit(oldS, SEC(id));
        if (newS != oldS)
            ropeBitmap.replace(FIR(id), newS);
    }

    void reset(const std::vector<IndexType> &ids)
    {
        std::unordered_map<elementType, elementType> oldMap, newMap;
        for (auto n : ids)
        {
            assert(FIR(n) < ropeBitmap.size());
            auto idx = FIR(n);
            if (oldMap.count(idx) == 0)
            {
                oldMap[idx] = ropeBitmap[idx];
                newMap[idx] = oldMap[idx]; 
            }
            newMap[idx] = reset_bit(newMap[idx], SEC(n));
        }
        for (auto [i, newS] : newMap)
            if (newS != oldMap[i])
                ropeBitmap.replace(i, newS);
    }

    bool test(IndexType id) const
    {
        return test_bit(ropeBitmap[FIR(id)], SEC(id));
    }

    size_t get_innerContainerSize() const
    {
        return ropeBitmap.size();
    }

    std::vector<IndexType> get_elements() const
    {
        constexpr size_t bitsOfElement = sizeof(elementType) * 8;
        std::vector<IndexType> res;
        for (size_t i = 0; i < ropeBitmap.size(); ++i)
        {
            auto e = ropeBitmap[i];
            if (e == 0)  continue;
            for (size_t j = 0; j < bitsOfElement; ++j)
            {
                if (e & (size_t)1)
                    res.push_back(static_cast<IndexType>((i << NBITLOG) + j));
                e >>= 1;
            }
        }
        return res;
    }

private:
    static const elementType NBITLOG = 6;
    static const elementType NBIT = 1 << NBITLOG;
    static const elementType NBITMASK = NBIT - 1;

    inline elementType FIR(IndexType pos) const { return static_cast<elementType>(pos >> NBITLOG); }
    inline elementType SEC(IndexType pos) const { return static_cast<elementType>(pos & NBITMASK); }

    static inline elementType set_bit(elementType x, elementType bit)  { return x | ((elementType)1 << bit); }
    static inline elementType reset_bit(elementType x, elementType bit)  { return x & ~((elementType)1 << bit); }
    static inline bool test_bit(elementType x, elementType bit)  { return x & ((elementType)1 << bit); }

    using RopeContainerType = __gnu_cxx::rope<elementType>;
    RopeContainerType ropeBitmap;
};


}