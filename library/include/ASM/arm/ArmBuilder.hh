#pragma once
#include <utility>
#include <vector>
#include "ASM/AssemblyBuilder.hh"
#include "ASM/Common.hh"
#include "TAC/ThreeAddressCode.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {

class RegAllocator;

class ArmBuilder : public AssemblyBuilder {
  NONCOPYABLE(ArmBuilder)
  //特殊寄存器编号
  static const int SP_REGID = 13;
  static const int LR_REGID = 14;
  static const int PC_REGID = 15;

 public:
  ArmBuilder(TACListPtr tac_list);
  bool Translate(std::string *output) override;

 private:
  //汇编头部
  bool AppendPrefix();

  //汇编尾部
  bool AppendSuffix();

  //处理全局（函数之外）
  bool TranslateGlobal();

  //处理所有函数（全局之外）
  bool TranslateFunctions();

  //处理单独一个函数
  bool TranslateFunction();

  //通用寄存器id转名字
  static std::string IntRegIDToName(int regid);

  std::string DeclareDataToASMString(TACPtr tac);

  std::string AddDataRefToASMString(TACPtr tac);

  std::string GlobalTACToASMString(TACPtr tac);

  std::string FuncTACToASMString(TACPtr tac);

  std::string *target_output_;

  //数据段代码
  std::string data_section_;

  //初始化段代码，用于对全局数据进行预先初始化。
  std::string init_section_;

  // text段末尾，补充上对数据段的引用
  std::string text_section_back_;

  struct FuncASM {
    std::string name_;
    std::string body_;
    FuncASM(const std::string &name, const std::string &body) : name_(name), body_(body) {}
    FuncASM(const std::string &name) : name_(name), body_() {}
  };
  //各个函数被分类放置，pair.first为函数名，pair.second为函数的具体内容
  std::vector<FuncASM> func_sections_;

  //当前函数的reg allocator
  std::unique_ptr<RegAllocator> reg_alloc_;

  TACListPtr tac_list_;
  TACList::iterator current_;
  TACList::iterator end_;
};
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler