#include "ASM/arm/GlobalContext.hh"
#include "ASM/arm/RegAllocator.hh"
#include "MacroUtil.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {
namespace ArmUtil {

void GlobalContext::SetUp() {
  for (int i = 0; i < USE_INT_REG_NUM; i++) {
    int_regs_[i] = nullptr;
    int_regs_ranks_[i] = i;
  }
  for (int i = 0; i < USE_FLOAT_REG_NUM; i++) {
    float_regs_[i] = nullptr;
    float_regs_ranks_[i] = i;
  }
  var_stack_pos.clear();
  stack_size_for_args_ = 0;
  stack_size_for_regsave_ = 0;
  stack_size_for_vars_ = 0;
  arg_nfloatregs_ = 0;
  arg_nintregs_ = 0;
  arg_stacksize_ = 0;
  arg_records_.clear();
}

void GlobalContext::TearDown() {
  //防止SymbolPtr一直占用s
  var_stack_pos.clear();
}

GlobalContextGuard::~GlobalContextGuard() { glob_context_.TearDown(); }
GlobalContextGuard::GlobalContextGuard(GlobalContext &glob_context) : glob_context_(glob_context) {
  glob_context_.SetUp();
}

}  // namespace ArmUtil
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler