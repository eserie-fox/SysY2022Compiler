#include <gtest/gtest.h>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
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

std::ostream& operator<<(std::ostream& os, const SymLiveInfo& liveInfo)
{
    os << "defCnt: " << liveInfo.defCnt;
    os << ", useCnt: " << liveInfo.useCnt << '\n';
    os << "liveRanges: ";
    for (auto &e : liveInfo.liveIntervalSet)
        os << '[' << e.first << ", " << e.second << "] ";
    os << "\n";
    return os;
}

TEST(LiveAnalyzerTest, test)
{
    HaveFunCompiler::Parser::Driver driver;
    HaveFunCompiler::Parser::TACDriver tacdriver;
    TACListPtr tac_list;

    ASSERT_EQ(driver.parse("test.txt"), true);

    std::stringstream ss;
    driver.print(ss) << "\n";
    auto tss = ss.str();
    std::cout << tss << std::endl;
    std::cout << '\n';

    tacdriver.parse(ss);
    tac_list = tacdriver.get_tacbuilder()->GetTACList();

    std::shared_ptr<ControlFlowGraph> cfg = std::make_shared<ControlFlowGraph>(tac_list);
    cfg->printToDot();

    LiveAnalyzer liveAnalyzer(cfg);
    auto symSet = liveAnalyzer.get_allSymSet();

    std::cout << "LiveInfo:\n";
    for (auto sym : symSet)
    {
        std::cout << "sym \"" << sym->get_tac_name(true) << "\":\n";
        auto liveInfo = liveAnalyzer.get_symLiveInfo(sym);
        if (liveInfo)
            std::cout << *liveInfo;
    }
}