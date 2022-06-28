#pragma once
#include <stdint.h>
#include <vector>

namespace HaveFunCompiler {
namespace AssemblyBuilder {

class ArmHelper {
 public:
  static uint32_t BitcastToUInt(float value);

  static bool IsImmediateValue(int value);

  static bool IsImmediateValue(uint32_t value);

  static std::vector<uint32_t> DivideIntoImmediateValues(uint32_t value);

  //作为offset时的立即数
  static bool IsLDRSTRImmediateValue(int value);
};
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler