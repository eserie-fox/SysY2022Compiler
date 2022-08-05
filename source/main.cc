#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <memory>
#include <cstdio>
#include <cstring>

#include "Driver.hh"
#include "TACDriver.hh"

#include "ASM/arm/ArmBuilder.hh"

using namespace HaveFunCompiler::AssemblyBuilder;

enum class ArgType
{
  SourceFile, TargetFile, _o, Others
};

ArgType analyzeArg(const char *arg)
{
  std::string s(arg);
  auto pos = s.rfind('.');
  if (pos == std::string::npos)
  {
    if (s == "-o")
      return ArgType::_o;
    return ArgType::Others;
  }
  else
  {
    auto suffix = s.substr(pos);
    if (suffix == ".sy")
      return ArgType::SourceFile;
    else if (suffix == ".s")
      return ArgType::TargetFile;
    else
      return ArgType::Others;
  }
}

int main([[maybe_unused]] const int arg, [[maybe_unused]] const char **argv) {
  HaveFunCompiler::Parser::Driver driver;
  HaveFunCompiler::Parser::TACDriver tacdriver;

  // 分析命令行参数, 目前做IO重定向
  const char *input;
  for (int i = 0; i < arg; ++i) {
    auto res = analyzeArg(argv[i]);
    if (res == ArgType::SourceFile)
      input = argv[i];
    else if (res == ArgType::_o) {
      if (analyzeArg(argv[i + 1]) == ArgType::TargetFile) {
        ++i;
        freopen(argv[i], "w", stdout);
      }
    }
  }
  if (!driver.parse(input)) {
    return -1;
  }
  std::stringstream ss;
  driver.print(ss) << "\n";
  auto tss = ss.str();
  // std::cout << tss << std::endl;
  if (!tacdriver.parse(ss)) {
    return -1;
  }
  // tacdriver.print(std::cout) << std::endl;

  std::string output;
  ArmBuilder armBuilder(tacdriver.get_tacbuilder()->GetTACList());
  if (!armBuilder.Translate(&output)) {
    return -1;
  }
  std::cout << output << std::endl;
  return 0;
}