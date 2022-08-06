#pragma once

#include "ASM/Common.hh"
#include "TAC/ThreeAddressCode.hh"

namespace HaveFunCompiler{
namespace AssemblyBuilder{

// optimizer需要保证，不修改fbegin和fend
class DeadCodeOptimizer
{
public:
    DeadCodeOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend) : _fbegin(fbegin), _fend(fend), _tacls(tacList) {}
    NONCOPYABLE(DeadCodeOptimizer)

    void optimize();

private:
    TACList::iterator _fbegin, _fend;
    TACListPtr _tacls;
};

}
}