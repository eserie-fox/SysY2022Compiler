#include <gtest/gtest.h>
#include <cassert>
#include <iostream>
#include <memory>
#include "ASM/ArrivalAnalyzer.hh"
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LoopDetector.hh"
#include "ASM/LoopInvariantDetector.hh"
#include "Driver.hh"
#include "TACDriver.hh"
#include "Utility.hh"

using namespace HaveFunCompiler;
using namespace AssemblyBuilder;

class LoopTest : public ::testing::Test {
 public:
  std::shared_ptr<ControlFlowGraph> cfg_;
  LoopTest() {
    HaveFunCompiler::Parser::Driver driver;
    HaveFunCompiler::Parser::TACDriver tacdriver;
    TACListPtr tac_list;
    bool suc = driver.parse("test.txt");
    assert(suc);
    std::stringstream ss;
    driver.print(ss) << "\n";
    tacdriver.parse(ss);
    tac_list = tacdriver.get_tacbuilder()->GetTACList();
    cfg_ = std::make_shared<ControlFlowGraph>(tac_list);
  }
  ~LoopTest() {}
};

TEST_F(LoopTest, LoopDetectorTest) {
  auto ld_ = std::make_shared<LoopDetector>(cfg_);
  auto loop_ranges = ld_->get_loop_ranges();
  for (auto range : loop_ranges) {
    std::cout << "Range ( " << range.first << " " << range.second << " )" << std::endl;
    LoopDetector::PrintLoop(std::cout, *cfg_, range) << std::endl;
  }
}


TEST_F(LoopTest,LoopInvariantDetectorTest){
  auto ld_ = std::make_shared<LoopDetector>(cfg_);
  auto aa = std::make_shared<ArrivalAnalyzer>(cfg_);
  aa->analyze();
  auto lid = std::make_shared<LoopInvariantDetector>(cfg_, ld_, aa);
  lid->analyze();
  auto loop_ranges = ld_->get_loop_ranges();
  for (auto range : loop_ranges) {
    std::cout << range << std::endl;
    const auto &invar = lid->get_invariants_of_loop(range);
    for (auto node_id : invar) {
      std::cout << cfg_->get_node_tac(node_id)->ToString() << std::endl;
    }
  }
}


