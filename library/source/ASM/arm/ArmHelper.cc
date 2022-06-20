#include "ASM/arm/ArmHelper.hh"
#include <algorithm>

namespace HaveFunCompiler {
namespace AssemblyBuilder {

uint32_t ArmHelper::BitcastToUInt(float value) { return *reinterpret_cast<uint32_t *>(&value); }

bool ArmHelper::IsImmediateValue(uint32_t value) {
  int lZeros = 0;
  int rZeros = 0;
  int mZeros = 0;
  for (int i = 0; i < 32; i++) {
    if (value & (1 << i)) break;
    lZeros++;
  }
  for (int i = 31; i >= 0; i--) {
    if (value & (1 << i)) break;
    rZeros++;
  }
  int tmp = 0;
  for (int i = 0; i < 32; i++) {
    if (value & (1 << i)) {
      mZeros = std::max(mZeros, tmp);
      tmp = 0;
      continue;
    }
    tmp++;
  }
  return std::max(mZeros, lZeros + rZeros) >= 24;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler