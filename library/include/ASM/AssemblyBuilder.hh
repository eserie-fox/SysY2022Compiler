#pragma once

#include <string>
#include "MacroUtil.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {
class AssemblyBuilder {
  NONCOPYABLE(AssemblyBuilder)
 public:
  AssemblyBuilder() = default;
  virtual ~AssemblyBuilder() = default;
  //成功时返回true，否则返回false。output!=nullptr时会记录结果。
  virtual bool Translate(std::string *output);
};
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler