#pragma once
#include "ASM/AssemblyBuilder.hh"
#include "ASM/Common.hh"
#include "TAC/ThreeAddressCode.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {
class ArmBuilder : public AssemblyBuilder {
  NONCOPYABLE(ArmBuilder)
 public:
  ArmBuilder(TACListPtr tac_list);
  bool Translate(std::string *output) override;

 private:
  TACListPtr tac_list_;
  TACList::iterator current_;
  TACList::iterator end_;
};
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler