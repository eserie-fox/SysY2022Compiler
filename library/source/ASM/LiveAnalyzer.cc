#include "ASM/LiveAnalyzer.hh"
#include <stdexcept>

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


}
}