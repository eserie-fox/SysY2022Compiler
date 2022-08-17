#include <gtest/gtest.h>
#include "ASM/ControlFlowGraph.hh"
#include "TAC/TAC.hh"
#include <vector>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include "Driver.hh"
#include "TACDriver.hh"

using namespace HaveFunCompiler;
using namespace HaveFunCompiler::ThreeAddressCode;
using namespace HaveFunCompiler::AssemblyBuilder;

class CFGTest : public ::testing::Test
{
public:
    TACBuilder tacBuilder;
    ThreeAddressCodeList tacList;

    SymbolPtr a, b, c;  // a = b op c
    SymbolPtr x, y;   // x = op y
    std::vector<SymbolPtr> labels;

    CFGTest() 
    {
        a = tacBuilder.NewSymbol(SymbolType::Variable, "a", 1);
        b = tacBuilder.NewSymbol(SymbolType::Variable, "b", 1);
        c = tacBuilder.NewSymbol(SymbolType::Variable, "c", 1);
        x = tacBuilder.NewSymbol(SymbolType::Variable, "a", 2);
        y = tacBuilder.NewSymbol(SymbolType::Variable, "a", 2);
        tacList = tacBuilder.NewTACList();
        tacList += tacBuilder.NewTAC(TACOperationType::FunctionBegin);
        tacList += tacBuilder.NewTAC(TACOperationType::Label, tacBuilder.NewSymbol(SymbolType::Label, "funcName"));
    }
    ~CFGTest() {}
};

TEST_F(CFGTest, serialTest)
{
    for (int i = 0; i < 3; ++i)
        tacList += tacBuilder.NewTAC(TACOperationType::Add, a, b, c);
    tacList += tacBuilder.NewTAC(TACOperationType::FunctionEnd);
    ControlFlowGraph cfg(tacList.begin(), tacList.end());
    cfg.printToDot();
}

// 存在从函数入口点不可达的代码
// 同时测试Label, goto, 多个inNode
TEST_F(CFGTest, deadCode)
{
    labels.push_back(tacBuilder.NewSymbol(SymbolType::Label, ".L1"));

    tacList += tacBuilder.NewTAC(TACOperationType::Label, labels[0]);
    tacList += tacBuilder.NewTAC(TACOperationType::Add, a, b, c);
    tacList += tacBuilder.NewTAC(TACOperationType::Goto, labels[0]);

    tacList += tacBuilder.NewTAC(TACOperationType::Add, a, b, c);
    tacList += tacBuilder.NewTAC(TACOperationType::FunctionEnd);

    ControlFlowGraph cfg(tacList.begin(), tacList.end());
    cfg.printToDot();
}

// 测试if-else
TEST_F(CFGTest, ifTest)
{
    // label funcName
    // fbegin
    // ifz a goto .L1
    // a = b + c
    // goto .L2
    // label .L1
    // x = b + c
    // label .L2
    // fend

    labels.push_back(tacBuilder.NewSymbol(SymbolType::Label, ".L1"));
    labels.push_back(tacBuilder.NewSymbol(SymbolType::Label, ".L2"));

    tacList += tacBuilder.NewTAC(TACOperationType::IfZero, labels[0], a);
    tacList += tacBuilder.NewTAC(TACOperationType::Add, a, b, c);
    tacList += tacBuilder.NewTAC(TACOperationType::Goto, labels[1]);
    tacList += tacBuilder.NewTAC(TACOperationType::Label, labels[0]);
    tacList += tacBuilder.NewTAC(TACOperationType::Add, x, b, c);
    tacList += tacBuilder.NewTAC(TACOperationType::Label, labels[1]);
    tacList += tacBuilder.NewTAC(TACOperationType::FunctionEnd);

    ControlFlowGraph cfg(tacList.begin(), tacList.end());
    cfg.printToDot();
}

TEST(CFGTestUseParser, test)
{
    HaveFunCompiler::Parser::Driver driver;
    HaveFunCompiler::Parser::TACDriver tacdriver;
    TACListPtr tac_list;

    ASSERT_EQ(driver.parse("test.txt"), true);

    std::stringstream ss;
    driver.print(ss) << "\n";
    auto tss = ss.str();
    std::cout << tss << std::endl;

    tacdriver.parse(ss);
    tac_list = tacdriver.get_tacbuilder()->GetTACList();

    ControlFlowGraph cfg(tac_list);
    cfg.printToDot();
    std::cout << '\n';

    
}