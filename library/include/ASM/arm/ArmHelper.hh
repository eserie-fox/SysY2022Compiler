#pragma once
#include <stdint.h>
#include <functional>
#include <string>
#include <vector>

namespace HaveFunCompiler {
namespace AssemblyBuilder {

class ArmHelper {
 public:
  static uint32_t BitcastToUInt(float value);

  static bool IsImmediateValue(int value);

  static bool IsImmediateValue(uint32_t value);

  static std::vector<uint32_t> DivideIntoImmediateValues(uint32_t value);

  static bool IsPowerOf2(int value);

  static int Log2(int value);

  static int CountLines(const std::string &str);

  //作为offset时的立即数
  static bool IsLDRSTRImmediateValue(int value);

  //现在只能支持add和sub指令
  static bool EmitImmediateInstWithCheck(std::function<void(const std::string &)> emitln, const std::string &operation,
                                         const std::string &operand1, const std::string &operand2, int imm,
                                         const std::string suffix = "");
};
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler