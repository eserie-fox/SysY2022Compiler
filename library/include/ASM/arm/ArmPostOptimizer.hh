#pragma once
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "MacroUtil.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {
class ArmPostOptimizer {
  NONCOPYABLE(ArmPostOptimizer)
 public:
  ArmPostOptimizer(const std::string &arm_assembly);

  const std::string& optimize();

 private:
  void OptimizeImpl();
  void SplitToLines();
  void SummarizeNoncommentLines();

  bool CheckCanOptimize(const std::vector<size_t> &buf);

  void SubOptimize1();
  
  void SubOptimize2();

  //line_idx为对应行在lines_中的下标
  void DeleteLine(size_t line_idx);

  bool is_optimized;
  const std::string &arm_assembly_;
  std::vector<std::string> lines_;
  //非注释行下标
  std::set<size_t> noncomm_lines_idx_;
  //是否会被删除
  std::vector<bool> is_deleted_;
  std::string optimized_;
};
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler