#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <memory>

#include "Driver.hh"
#include "TACDriver.hh"

#include "ASM/arm/ArmBuilder.hh"

using namespace HaveFunCompiler::AssemblyBuilder;

int main([[maybe_unused]] const int arg, [[maybe_unused]] const char **argv) {
  HaveFunCompiler::Parser::Driver driver;
  HaveFunCompiler::Parser::TACDriver tacdriver;
  if (driver.parse("test.txt")) {
    std::stringstream ss;
    driver.print(ss) << "\n";
    auto tss = ss.str();
//    std::cout << tss << std::endl;
    tacdriver.parse(ss);
    tacdriver.print(std::cout) << std::endl;

    std::string output;
    ArmBuilder armBuilder(tacdriver.get_tacbuilder()->GetTACList());
    armBuilder.Translate(&output);
    std::cout << output << std::endl;
  }
  return 0;
}