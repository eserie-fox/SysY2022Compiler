#pragma once
#include <utility>
#include <vector>
#include "ASM/AssemblyBuilder.hh"
#include "ASM/Common.hh"
#include "ASM/arm/FunctionContext.hh"
#include "ASM/arm/GlobalContext.hh"
#include "TAC/ThreeAddressCode.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {

class ArmBuilder : public AssemblyBuilder {
  NONCOPYABLE(ArmBuilder)
  //特殊寄存器编号
  static const int FP_REGID = 11;
  static const int IP_REGID = 12;
  static const int SP_REGID = 13;
  static const int LR_REGID = 14;
  static const int PC_REGID = 15;
  static const int DATA_POOL_DISTANCE_THRESHOLD = 400;

 public:
  ArmBuilder(TACListPtr tac_list);
  ArmBuilder() = delete;
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

  static std::string FloatRegIDToName(int regid);

  //获得该sym对应的变量名
  std::string GetVariableName(SymbolPtr sym);

  //获得某个数据的引用名称
  std::string ToDataRefName(std::string name);

  std::string DeclareDataToASMString(TACPtr tac);

  void AddDataRef(TACPtr tac);

  std::string GlobalTACToASMString(TACPtr tac);

  std::string FuncTACToASMString(TACPtr tac);

  std::string EndCurrentDataPool(bool ignorebranch = false);

  std::string *target_output_;

  //数据段代码
  std::string data_section_;

  // 对数据段的引用
  std::vector<std::string> ref_data_;

  struct FuncASM {
    std::string name_;
    std::string body_;
    FuncASM(const std::string &name, const std::string &body) : name_(name), body_(body) {}
    FuncASM(const std::string &name) : name_(name), body_() {}
  };
  //各个函数被分类放置，pair.first为函数名，pair.second为函数的具体内容
  std::vector<FuncASM> func_sections_;

  ArmUtil::FunctionContext func_context_;

  ArmUtil::GlobalContext glob_context_;

  int data_pool_distance_;
  int data_pool_id_;

  TACListPtr tac_list_;
  TACList::iterator current_;
  TACList::iterator end_;
};
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler