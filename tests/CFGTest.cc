#include <gtest/gtest.h>
#include "ASM/ControlFlowGraph.hh"
#include "TAC/TAC.hh"
#include <vector>

using namespace HaveFunCompiler;
using namespace HaveFunCompiler::ThreeAddressCode;

// class CFGTest : public ::testing::Test
// {
// public:
//     TACFactory tacFactory;

//     SymbolPtr a, b, c;  // a = b op c
//     SymbolPtr x, y;   // x = op y
//     std::vector<SymbolPtr> labels;

//     CFGTest() 
//     {
        
//     }
//     ~CFGTest() {}
// };