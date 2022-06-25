#include "ASM/arm/ArmHelper.hh"
#include <algorithm>
#include <stdexcept>

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

std::vector<uint32_t> ArmHelper::DivideIntoImmediateValues(uint32_t value) {
  if (value == 0) {
    return {0};
  }
  std::vector<uint32_t> ret;
  while (value) {
    int start;
    for (start = 31; start >= 0; start--) {
      uint32_t bit = 1U << start;
      if (value & bit) {
        break;
      }
    }
    if (start == -1) {
      throw std::logic_error("DivideIntoImmediateValues Unknown situation");
    }
    uint32_t nval = 0;
    for (int i = start; i >= std::max(i - 7, 0); i--) {
      uint32_t bit = 1U << start;
      if (value & bit) {
        value &= ~bit;
        nval |= bit;
      }
    }
    ret.push_back(nval);
  }
  return ret;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler