#pragma once

#include <ext/rope>
#include <vector>

template<typename IndexType = size_t>
class RopeContainer
{
public:
    RopeContainer() = default;
    RopeContainer(const RopeContainer &) = default;
    RopeContainer(RopeContainer &&) = default;
    RopeContainer(size_t elementNumber) : 
        ropeBitmap(FIR(elementNumber) + (SEC(elementNumber) ? 1 : 0)) {}

    RopeContainer &operator=(const RopeContainer &) = default;
    RopeContainer &operator=(RopeContainer &&) = default;

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
            ropeBitmap.append(it, o.ropeBitmap.end());
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
           ropeBitmap.erase(i, ropeBitmap.size() - i);
        return *this;
    }

    void set(IndexType id)
    {
        auto oldS = ropeBitmap[FIR(id)];
        auto newS = set_bit(oldS, SEC(id));
        if (newS != oldS)
            ropeBitmap[FIR(id)] = newS;
    }

    void set(const std::vector<IndexType> &ids)
    {
        std::unordered_map<size_t, size_t> oldMap, newMap;
        for (auto n : ids)
        {
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
        auto oldS = ropeBitmap[FIR(id)];
        auto newS = reset_bit(oldS, SEC(id));
        if (newS != oldS)
            ropeBitmap[FIR(id)] = newS;
    }

    void reset(const std::vector<IndexType> &ids)
    {
        std::unordered_map<size_t, size_t> oldMap, newMap;
        for (auto n : ids)
        {
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

private:
    static const size_t NBITLOG = 8;
    static const size_t NBIT = 1 << NBITLOG;
    static const size_t NBITMASK = NBIT - 1;

    inline size_t FIR(IndexType pos) const { return static_cast<size_t>(pos >> NBITLOG); }
    inline size_t SEC(IndexType pos) const { return static_cast<size_t>(pos & NBITMASK); }

    static inline size_t set_bit(size_t x, size_t bit)  { return x | ((size_t)1 << bit); }
    static inline size_t reset_bit(size_t x, size_t bit)  { return x & ~((size_t)1 << bit); }
    static inline bool test_bit(size_t x, size_t bit)  { return x & ((size_t)1 << bit); }

    using RopeContainerType = __gnu_cxx::rope<size_t>;
    RopeContainerType ropeBitmap;
};