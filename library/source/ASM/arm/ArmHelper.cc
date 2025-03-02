#include "ASM/arm/ArmHelper.hh"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include "Utility.hh"

static int log2(uint32_t value) {
  int ret = 0;
  while (value) {
    ++ret;
    value >>= 1;
  }
  return ret;
}

namespace HaveFunCompiler {
namespace AssemblyBuilder {

uint32_t ArmHelper::BitcastToUInt(float value) { return *reinterpret_cast<uint32_t *>(&value); }

bool ArmHelper::IsImmediateValue(int value) { return IsImmediateValue(static_cast<uint32_t>(value)); }

bool ArmHelper::IsImmediateValue(uint32_t value) {
  int lowbit;

  if ((value & ~0xffU) == 0) return true;

  /* Get the number of trailing zeros.  */
  lowbit = log2(value & (-value)) - 1;

  lowbit &= ~1;

  if ((value & ~(0xffU << lowbit)) == 0) return true;

  if (lowbit <= 4) {
    if ((value & ~0xc000003fU) == 0 || (value & ~0xf000000fU) == 0 || (value & ~0xfc000003U) == 0) {
      return true;
    }
  }
  return false;
}

bool ArmHelper::IsPowerOf2(int value) { return !(value & (value - 1)); }

int ArmHelper::Log2(int value) {
  int ret = -1;
  while (value) {
    value >>= 1;
    ++ret;
  }
  return ret;
}

std::vector<uint32_t> ArmHelper::DivideIntoImmediateValues(uint32_t value) {
  if (value == 0) {
    return {0};
  }
  std::vector<uint32_t> ret;
  while (value) {
    int start;
    for (start = 30; start >= 0; start -= 2) {
      uint32_t bit = 3U << start;
      if (value & bit) {
        break;
      }
    }
    if (start == -1) {
      throw std::logic_error("DivideIntoImmediateValues Unknown situation");
    }
    uint32_t nval = 0;
    for (int i = start + 1; i >= std::max(start - 6, 0); i--) {
      uint32_t bit = 1U << i;
      if (value & bit) {
        value &= ~bit;
        nval |= bit;
      }
    }
    ret.push_back(nval);
  }
  return ret;
}

bool ArmHelper::IsLDRSTRImmediateValue(int value) {
  if (value > -4096 && value < 4096) {
    return true;
  }
  return false;
}

int ArmHelper::CountLines(const std::string &str) {
  int ret = 0;
  for (auto c : str) {
    ret += (c == '\n');
  }
  return ret;
}

bool ArmHelper::EmitImmediateInstWithCheck(std::function<void(const std::string &)> emitln,
                                           const std::string &operation, const std::string &operand1,
                                           const std::string &operand2, int imm, const std::string suffix) {
  enum OperationType { None, IgnoreZero };
  OperationType type = None;
  if (Contains(operation, "add") || Contains(operation, "sub")) {
    type = IgnoreZero;
  }
  if (type == None) {
    std::cerr << "Warning:EmitImmediateInstWithCheck" << std::endl;
    return false;
  }
  if ((type == IgnoreZero && imm == 0)) {
    if (operand1 != operand2) {
      emitln("mov " + operand1 + ", " + operand2);
    }
    return true;
  }
  if (IsImmediateValue(imm)) {
    std::string inst = operation + " " + operand1 + ", " + operand2 + ", #" + std::to_string(imm);
    if (!suffix.empty()) {
      inst += ", " + suffix;
    }
    emitln(inst);
    return true;
  }
  return false;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler