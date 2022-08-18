#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LoopDetector.hh"
#include "Driver.hh"
#include "TACDriver.hh"

using namespace HaveFunCompiler;
using namespace AssemblyBuilder;




TEST(LoopDetector, SimpleTest) {

    HaveFunCompiler::Parser::Driver driver;
    HaveFunCompiler::Parser::TACDriver tacdriver;
    TACListPtr tac_list;

    ASSERT_EQ(driver.parse("test.txt"), true);

    std::stringstream ss;
    driver.print(ss) << "\n";
    tacdriver.parse(ss);
    tac_list = tacdriver.get_tacbuilder()->GetTACList();

    std::shared_ptr<ControlFlowGraph> cfg = std::make_shared<ControlFlowGraph>(tac_list);
    LoopDetector ld(cfg);
    auto loop_ranges = ld.get_loop_ranges();
    for (auto range : loop_ranges) {
      std::cout << "Range ( " << range.first << " " << range.second << " )" << std::endl;
      LoopDetector::PrintLoop(std::cout, *cfg, range) << std::endl;
    }
}
