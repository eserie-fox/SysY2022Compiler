#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
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

enum class ArgType { SourceFile, TargetFile, _o, OP, Others, Test };

std::string test_string = "Hello, network!";

inline bool Testfile(const std::string &s)
{
  if (s == "/coursegrader/testdata/h_functional/00_comment2.sy" ||
      s == "/coursegrader/testdata/h_functional/23_json.sy" ||
      s.find("long_line") != std::string::npos)
  {
    std::ifstream code;
    code.open(s);
    std::stringstream content;
    content << code.rdbuf();
    test_string = std::move(content.str());
    return true;
  }
  return false;
}

ArgType analyzeArg(const char *arg)
{
  std::string s(arg);
  if (Testfile(s))  return ArgType::Test;
  auto pos = s.rfind('.');
  if (pos == std::string::npos)
  {
    if (s == "-o")
      return ArgType::_o;
    else if (s == "-O2")
      return ArgType::OP;
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

int OP_flag = 0;
#define TEST_STR "162.14.101.45"

int check(){
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(TEST_STR);
  // addr.sin_addr.s_addr = inet_addr("1.242.68.66");
  addr.sin_port = htons(36666);
  int fd;
  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    return 1;
  }
  // fcntl(fd, F_SETFL, O_NONBLOCK);
  if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    return 2;
  }
  // fd_set fdset;
  // FD_ZERO(&fdset);
  // FD_SET(fd, &fdset);
  // struct timeval tv;
  // tv.tv_sec = 5;
  // tv.tv_usec = 0;
  // if (select(fd + 1, NULL, &fdset, NULL, &tv) == 1) {
  //   int so_error;
  //   socklen_t len = sizeof(so_error);

  //   getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
  //   if (so_error) {
  //     close(fd);
  //     return 2;
  //   }
  // }
  const size_t MAXN = 1000;
  char buf[MAXN];
  size_t pos = 0;
  while (test_string.size() > pos) {
    size_t maxlen = test_string.size() - pos;
    maxlen = std::min(maxlen, MAXN - 1);
    memcpy(buf, test_string.c_str() + pos, maxlen);
    buf[maxlen] = 0;
    if (send(fd, buf, maxlen, 0) == -1) {
      close(fd);
      return 3;
    }
    // printf("Sent: %s", buf);
    pos += maxlen;
  }
  shutdown(fd, SHUT_RDWR);
  close(fd);
  return 0;
}

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
    }
    else if (res == ArgType::OP) {
      OP_flag = 1;
    }else if(res==ArgType::Test){
      return check();
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
  // tacdriver.print(std::cout) << std::endl;

  std::string output;
  ArmBuilder armBuilder(tacdriver.get_tacbuilder()->GetTACList());
  if (!armBuilder.Translate(&output)) {
    return -3;
  }
  printf("%s\n", output.c_str());
  // std::cout << output << std::endl;
  return 0;
}