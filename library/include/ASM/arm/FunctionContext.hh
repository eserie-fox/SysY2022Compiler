#pragma once

#include "ASM/Common.hh"
#include "ASM/arm/RegAllocator.hh"
#include "stdint.h"

namespace HaveFunCompiler {
namespace AssemblyBuilder {

namespace ArmUtil {

struct FunctionContext {
  //寄存器是否是可用状态的位图
  uint32_t intregs_;
  uint32_t floatregs_;

  SymbolPtr int_freereg1_;
  SymbolPtr int_freereg2_;
  int last_int_freereg_;

  SymbolPtr float_freereg1_;
  SymbolPtr float_freereg2_;
  int last_float_freereg_;

  SymAttribute func_attr_;

  //记录当前压栈要用到多少寄存器
  uint32_t arg_nintregs_;
  uint32_t arg_nfloatregs_;
  //记录当前压栈需要用到多少栈大小（还没有真压）
  uint32_t arg_stacksize_;
  //压arg要到最后才知道需不需要添加空格来对齐。所以暂存一下。
  struct ArgRecord{
    SymbolPtr sym;
    bool isaddr;
    //要么放寄存器内要么放栈上
    bool storage_in_reg;
    //放的位置
    //storage_in_reg=true时为寄存器编号(0,1,2,3...)，否则为栈上位置(4为单位, 0,4,8,12...)。
    int storage_pos;
  };
  std::vector<ArgRecord> arg_records_;
  
  //以下三个是用来给函数返回保存的信息，用来还原函数进入时的状态。
  std::vector<uint32_t> var_stack_immvals_;
  std::vector<std::pair<uint32_t,uint32_t>> savefloatregs_;
  std::vector<uint32_t> saveintregs_;

  //为寄存器保存用的栈空间，字节单位
  int32_t stack_size_for_regsave_;

  //为额外参数分配的栈大小
  int32_t stack_size_for_args_;

  //为变量分配的栈大小
  int32_t stack_size_for_vars_;

  //为形参分配的栈大小
  int32_t stack_size_for_params_;

  // int形参数量
  uint32_t nint_param_;

  // float形参数量
  uint32_t nfloat_param_;

  //当前函数形参数量
  uint32_t nparam_;

  //当前函数的reg allocator
  RegAllocator *reg_alloc_;

  //是否处于函数头部位置，用来断言parameter只能出现在函数开头位置。
  bool parameter_head_;

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