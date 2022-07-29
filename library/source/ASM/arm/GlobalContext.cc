#include "ASM/arm/GlobalContext.hh"
#include "ASM/arm/RegAllocator.hh"
#include "MacroUtil.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {
namespace ArmUtil {

void GlobalContext::SetUp() {}

void GlobalContext::TearDown() {}

GlobalContextGuard::~GlobalContextGuard() { glob_context_.TearDown(); }
GlobalContextGuard::GlobalContextGuard(GlobalContext &glob_context) : glob_context_(glob_context) {
  glob_context_.SetUp();
}

}  // namespace ArmUtil
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler