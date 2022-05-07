#pragma once

#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include "MacroUtil.hh"
#include "TAC/Expression.hh"
#include "TAC/Symbol.hh"
#include "TAC/ThreeAddressCode.hh"
#include "UnionStack.hh"

namespace HaveFunCompiler {
namespace ThreeAddressCode {

using SymbolPtr = std::shared_ptr<Symbol>;
using ThreeAddressCodePtr = std::shared_ptr<ThreeAddressCode>;
using TACListPtr = std::shared_ptr<ThreeAddressCodeList>;
using ExpressionPtr = std::shared_ptr<Expression>;
using ArgListPtr = std::shared_ptr<ArgumentList>;
using ParamListPtr = std::shared_ptr<ParameterList>;
using ArrayDescriptorPtr = std::shared_ptr<ArrayDescriptor>;

class TACFactory {
  NONCOPYABLE(TACFactory)
  friend class TACBuilder;
 public:
  static TACFactory *Instance() {
    static TACFactory factory;
    return &factory;
  }
  //新TAC列表
  template <typename... _Args>
  inline std::shared_ptr<ThreeAddressCodeList> NewTACList(_Args &&...__args) {
    return std::make_shared<ThreeAddressCodeList>(std::forward<_Args>(__args)...);
  }
  //新Symbol
  SymbolPtr NewSymbol(SymbolType type, std::optional<std::string> name = std::nullopt, int offset = 0,
                      SymbolValue value = {});
  //新TAC
  ThreeAddressCodePtr NewTAC(TACOperationType operation, SymbolPtr a = nullptr, SymbolPtr b = nullptr,
                             SymbolPtr c = nullptr);
  //新表达式
  ExpressionPtr NewExp(TACListPtr tac, SymbolPtr ret);
  //新实参列表，用于Call函数
  ArgListPtr NewArgList();
  //新形参列表，用于定义函数
  ParamListPtr NewParamList();
  ArrayDescriptorPtr NewArrayDescriptor();

  private:
  TACListPtr MakeFunction(SymbolPtr func_label, ParamListPtr params, TACListPtr body);
  ExpressionPtr MakeAssign(SymbolPtr var, ExpressionPtr exp);

  SymbolPtr AccessArray(SymbolPtr array, std::vector<int> pos);
  ExpressionPtr MakeArrayInit(SymbolPtr array, ExpressionPtr exp_array);

  TACListPtr MakeCall(SymbolPtr func_label, ArgListPtr args);
  TACListPtr MakeCallWithRet(SymbolPtr func_label, ArgListPtr args, SymbolPtr ret_sym);
  TACListPtr MakeIf(ExpressionPtr cond, SymbolPtr label, TACListPtr stmt);
  TACListPtr MakeIfElse(ExpressionPtr cond, SymbolPtr label_true, TACListPtr stmt_true, SymbolPtr label_false,
                        TACListPtr stmt_false);
  TACListPtr MakeWhile(ExpressionPtr cond, SymbolPtr label_cont, SymbolPtr label_brk, TACListPtr stmt);
  TACListPtr MakeFor(TACListPtr init, ExpressionPtr cond, TACListPtr modify, SymbolPtr label_cont, SymbolPtr label_brk,
                     TACListPtr stmt);

  std::string ToFuncLabelName(const std::string name);
  std::string ToCustomerLabelName(const std::string name);
  std::string ToTempLabelName(uint64_t id);
  std::string ToVariableOrConstantName(const std::string name);
  std::string ToTempVariableName(uint64_t id);

 private:
  TACFactory() = default;
};

class TACBuilder {
  NONCOPYABLE(TACBuilder)
 public:
  TACBuilder();

  //进入一个作用域
  void EnterSubscope();
  //离开一个作用域
  void ExitSubscope();

  template <typename T>
  void Push(T v) {
    compiler_stack_.Push(v);
  }
  template <typename T>
  void Top(T *p) {
    compiler_stack_.Top(p);
  }
  
  void Pop() {
    compiler_stack_.Pop();
  }
  //新TAC列表
  template <typename... _Args>
  inline std::shared_ptr<ThreeAddressCodeList> NewTACList(_Args &&...__args) {
    return TACFactory::Instance()->NewTACList(std::forward<_Args>(__args)...);
  }
  //新Symbol
  inline SymbolPtr NewSymbol(SymbolType type, std::optional<std::string> name = std::nullopt, int offset = 0,
                      SymbolValue value = {}) {
    return TACFactory::Instance()->NewSymbol(type, name, offset, value);
  }
  //新TAC
  inline ThreeAddressCodePtr NewTAC(TACOperationType operation, SymbolPtr a = nullptr, SymbolPtr b = nullptr,
                                    SymbolPtr c = nullptr) {
    return TACFactory::Instance()->NewTAC(operation, a, b, c);
  }
  //新表达式
  inline ExpressionPtr NewExp(TACListPtr tac, SymbolPtr ret) { return TACFactory::Instance()->NewExp(tac, ret); }
  //新实参列表，用于Call函数
  inline ArgListPtr NewArgList(){
    return TACFactory::Instance()->NewArgList();
  }
  //新形参列表，用于定义函数
  inline ParamListPtr NewParamList(){
    return TACFactory::Instance()->NewParamList();
  }
  inline ArrayDescriptorPtr NewArrayDescriptor(){
    return TACFactory::Instance()->NewArrayDescriptor();
  }

  inline ExpressionPtr CreateAssign(SymbolPtr var, ExpressionPtr exp){
    return TACFactory::Instance()->MakeAssign(var, exp);
  }

  ExpressionPtr CreateConstExp(int n);
  ExpressionPtr CreateConstExp(float fn);
  ExpressionPtr CreateConstExp(SymbolValue v);
  bool BindConstName(const std::string &name, SymbolPtr constant);
  SymbolPtr CreateText(const std::string &text);

  //如果是常量，返回来的直接可以用
  ExpressionPtr CastFloatToInt(ExpressionPtr expF);
  ExpressionPtr CastIntToFloat(ExpressionPtr expI);

  SymbolPtr CreateFunctionLabel(const std::string &name);
  SymbolPtr CreateCustomerLabel(const std::string &name);
  SymbolPtr CreateTempLabel();
  SymbolPtr CreateVariable(const std::string &name, SymbolValue::ValueType type);
  SymbolPtr CreateTempVariable(SymbolValue::ValueType type);

  TACListPtr CreateFunction(const std::string &func_name, ParamListPtr params, TACListPtr body);
  TACListPtr CreateCall(const std::string &func_name, ArgListPtr args);
  TACListPtr CreateCallWithRet(const std::string &func_name, ArgListPtr args, SymbolPtr ret_sym);
  TACListPtr CreateIf(ExpressionPtr cond, TACListPtr stmt, SymbolPtr *out_label);
  TACListPtr CreateIfElse(ExpressionPtr cond, TACListPtr stmt_true, TACListPtr stmt_false, SymbolPtr *out_label_true,
                          SymbolPtr *out_label_false);
  TACListPtr CreateWhile(ExpressionPtr cond, TACListPtr stmt, SymbolPtr *out_label_cont, SymbolPtr *out_label_brk);
  TACListPtr CreateFor(TACListPtr init, ExpressionPtr cond, TACListPtr modify, TACListPtr stmt,
                       SymbolPtr *out_label_cont, SymbolPtr *out_label_brk);

  ExpressionPtr CreateArithmeticOperation(TACOperationType arith_op, ExpressionPtr exp1, ExpressionPtr exp2 = nullptr);

  //查找Variant
  SymbolPtr FindVariableOrConstant(const std::string &name);
  SymbolPtr FindFunctionLabel(const std::string &name);
  SymbolPtr FindCustomerLabel(const std::string &name);

  void Error(const std::string &message);

  void SetTACList(TACListPtr tac_list);

  TACListPtr GetTACList();

 private:
  std::string AppendScopePrefix(const std::string &name);
  SymbolPtr FindSymbolWithName(const std::string &name);
  UnionStack compiler_stack_;
  //目前临时变量标号
  uint64_t cur_temp_var_;
  //目前临时标签标号
  uint64_t cur_temp_label_;
  uint64_t cur_symtab_id_;
  std::vector<SymbolTable> symbol_stack_;

  //储存text的表
  std::unordered_map<std::string, std::shared_ptr<Symbol>> text_tab_;
  std::vector<std::string> stored_text_;

  TACListPtr tac_list_;
};

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler