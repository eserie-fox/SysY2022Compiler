#pragma once
#include <stdint.h>

namespace HaveFunCompiler {
namespace AssemblyBuilder {

class ArmHelper {
 public:
  static uint32_t BitcastToUInt(float value);

  static bool IsImmediateValue(uint32_t value);
};
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler