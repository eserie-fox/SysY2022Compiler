#pragma once

#include "ASM/Common.hh"
#include "ASM/arm/RegAllocator.hh"
#include "stdint.h"

namespace HaveFunCompiler {
namespace AssemblyBuilder {

namespace ArmUtil {

struct GlobalContext {
  static const int INT_REG_NUM = 16;
  static const int FLOAT_REG_NUM = 32;
  static const int USE_INT_REG_NUM = 13;

  //新函数开始时SetUp
  void SetUp();

  //函数解析结束后TearDown
  void TearDown();
};

//保证FunctionContext被正确初始化和销毁
struct GlobalContextGuard {
 public:
  ~GlobalContextGuard();

  GlobalContextGuard(GlobalContext &glob_context);

 private:
  GlobalContext &glob_context_;
};

}  // namespace ArmUtil

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler