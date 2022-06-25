#include "ASM/arm/FunctionContext.hh"
#include "ASM/arm/RegAllocator.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {
namespace ArmUtil {

void FunctionContext::SetUp() {
  stack_size_for_regsave_ = 0;
  stack_size_for_args_ = 0;
  stack_size_for_vars_ = 0;
  intregs_ = 0;
  nparam_ = 0;
  floatregs_ = 0;
  reg_alloc_ = nullptr;
  int_freereg1 = nullptr;
  int_freereg2 = nullptr;
  float_freereg1 = nullptr;
  float_freereg2 = nullptr;
}

void FunctionContext::TearDown() {
  delete reg_alloc_;
  int_freereg1 = nullptr;
  int_freereg2 = nullptr;
  float_freereg1 = nullptr;
  float_freereg2 = nullptr;
}

FunctionContextGuard::~FunctionContextGuard() { func_context_.TearDown(); }
FunctionContextGuard::FunctionContextGuard(FunctionContext &func_context) : func_context_(func_context) {
  func_context_.SetUp();
}

}  // namespace ArmUtil
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler