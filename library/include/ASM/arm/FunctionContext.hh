#pragma once

#include "ASM/Common.hh"
#include "stdint.h"

namespace HaveFunCompiler {
namespace AssemblyBuilder {

class RegAllocator;

namespace ArmUtil {

struct FunctionContext {
  //寄存器是否是可用状态的位图
  uint32_t intregs_;
  uint32_t floatregs_;

  SymbolPtr int_freereg1;
  SymbolPtr int_freereg2;

  SymbolPtr float_freereg1;
  SymbolPtr float_freereg2;

  //为寄存器保存用的栈空间，字节单位
  uint32_t stack_size_for_regsave_;

  //为额外参数分配的栈大小
  uint32_t stack_size_for_args_;

  uint32_t stack_size_for_vars_;

  //当前函数形参数量
  uint32_t nparam_;

  //当前函数的reg allocator
  RegAllocator *reg_alloc_;

  //新函数开始时SetUp
  void SetUp();

  //函数解析结束后TearDown
  void TearDown();
};

//保证FunctionContext被正确初始化和销毁
struct FunctionContextGuard {
 public:
  ~FunctionContextGuard();

  FunctionContextGuard(FunctionContext &func_context);

 private:
  FunctionContext &func_context_;
};

}  // namespace ArmUtil

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler