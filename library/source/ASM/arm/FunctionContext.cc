#include "ASM/arm/FunctionContext.hh"
#include "ASM/arm/RegAllocator.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {
namespace ArmUtil {

void FunctionContext::SetUp() {
  stack_size_for_regsave_ = 0;
  stack_size_for_args_ = 0;
  stack_size_for_vars_ = 0;
  stack_size_for_params_ = 0;
  intregs_ = 0;
  nparam_ = 0;
  floatregs_ = 0;
  nint_param_ = 0;
  nfloat_param_ = 0;
  func_attr_ = {};
  parameter_head_ = true;
  reg_alloc_ = nullptr;
  int_freereg1_ = nullptr;
  int_freereg2_ = nullptr;
  float_freereg1_ = nullptr;
  float_freereg2_ = nullptr;
}

void FunctionContext::TearDown() {
  delete reg_alloc_;
  int_freereg1_ = nullptr;
  int_freereg2_ = nullptr;
  float_freereg1_ = nullptr;
  float_freereg2_ = nullptr;
}

FunctionContextGuard::~FunctionContextGuard() { func_context_.TearDown(); }
FunctionContextGuard::FunctionContextGuard(FunctionContext &func_context) : func_context_(func_context) {
  func_context_.SetUp();
}

}  // namespace ArmUtil
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler