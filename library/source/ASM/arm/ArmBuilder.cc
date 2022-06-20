#include "ASM/arm/ArmBuilder.hh"
#include "TAC/TAC.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {

ArmBuilder::ArmBuilder(TACListPtr tac_list) : tac_list_(tac_list) {}

bool ArmBuilder::Translate([[maybe_unused]] std::string *output) {
  current_ = tac_list_->begin();
  end_ = tac_list_->end();

  return false;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler