#pragma once

#include <unordered_map>
#include "ASM/Common.hh"
#include "ASM/arm/RegAllocator.hh"
#include "stdint.h"

namespace HaveFunCompiler {
namespace AssemblyBuilder {

namespace ArmUtil {

struct GlobalContext {
  //简单起见，我们只使用12个通用寄存器
  static const int USE_INT_REG_NUM = 12;
  static const int USE_FLOAT_REG_NUM = 32;

  //变量在栈上的位置，从0开始，0,4,8,12... ，value(sym) = [sp+STACK_TOTALSIZE - var_stack_pos[sym] -4]
  std::unordered_map<SymbolPtr,int> var_stack_pos;

  //为寄存器保存用的栈空间，字节单位
  uint32_t stack_size_for_regsave_;

  //为额外参数分配的栈大小
  uint32_t stack_size_for_args_;

  //为变量分配的栈大小
  uint32_t stack_size_for_vars_;

  //用于存储SymbolPtr
  SymbolPtr int_regs_[USE_INT_REG_NUM];
  //用于LRU
  int int_regs_ranks_[USE_INT_REG_NUM];
  //用于存储SymbolPtr
  SymbolPtr float_regs_[USE_FLOAT_REG_NUM];
  //用于LRU
  int float_regs_ranks_[USE_FLOAT_REG_NUM];


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