#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <memory>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "Driver.hh"
#include "TACDriver.hh"

#include "ASM/arm/ArmBuilder.hh"

using namespace HaveFunCompiler::AssemblyBuilder;

enum class ArgType { SourceFile, TargetFile, _o, OP, ThreeAddressCode, Others };

ArgType analyzeArg(const char *arg)
{
  std::string s(arg);
  auto pos = s.rfind('.');
  if (pos == std::string::npos) {
    if (s == "-o")
      return ArgType::_o;
    else if (s == "-O2")
      return ArgType::OP;
    else if (s == "-M")
      return ArgType::ThreeAddressCode;
    return ArgType::Others;
  } else {
    auto suffix = s.substr(pos);
    if (suffix == ".sy")
      return ArgType::SourceFile;
    else if (suffix == ".s")
      return ArgType::TargetFile;
    else
      return ArgType::Others;
  }
}

int OP_flag = 0;
bool tac_only = false;

int main(const int arg, const char **argv) {
  HaveFunCompiler::Parser::Driver driver;
  HaveFunCompiler::Parser::TACDriver tacdriver;


  // 分析命令行参数, 目前做IO重定向
  const char *input = nullptr;
  for (int i = 0; i < arg; ++i) {
    auto res = analyzeArg(argv[i]);
    if (res == ArgType::SourceFile)
      input = argv[i];
    else if (res == ArgType::_o) {
      if (analyzeArg(argv[i + 1]) == ArgType::TargetFile) {
        ++i;
        freopen(argv[i], "w", stdout);
      }
    } else if (res == ArgType::OP) {
      OP_flag = 1;
    } else if (res == ArgType::ThreeAddressCode) {
      tac_only = true;
    }
  }
  if (input == nullptr || !driver.parse(input)) {
    return -1;
  }
  std::stringstream ss;
  driver.print(ss) << "\n";
  // auto tss = ss.str();
  // std::cout << tss << std::endl;
  if (!tacdriver.parse(ss)) {
    return -2;
  }

  std::string output;
  ArmBuilder armBuilder(tacdriver.get_tacbuilder()->GetTACList());
  if (!armBuilder.Translate(&output)) {
    return -3;
  }
  // printf("%s\n", output.c_str());
  std::cout << output << std::endl;
  return 0;
}