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
namespace Parser {
class location;
}
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
  friend class TACRebuilder;
  using location = HaveFunCompiler::Parser::location;

 private:
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
  SymbolPtr NewSymbol(SymbolType type, std::optional<std::string> name = std::nullopt, SymbolValue value = {},
                      int offset = 0);
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

  TACListPtr MakeFunction(const location *plocation_, SymbolPtr func_head, TACListPtr body);
  ExpressionPtr MakeAssign(const location *plocation_, SymbolPtr var, ExpressionPtr exp);

  // TACListPtr MakeCall(SymbolPtr func_label, ArgListPtr args);
  TACListPtr MakeCallWithRet(const location *plocation_, SymbolPtr func_label, ArgListPtr args, SymbolPtr ret_sym);
  // if(cond_zero==zero)
  //  stmt
  TACListPtr MakeIf(ExpressionPtr cond, SymbolPtr label, TACListPtr stmt);
  // if(cond_zero==zero)
  //  stmt_true
  // else
  //  stmt_false
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
  void Push(T value) {
    compiler_stack_.Push(value);
  }
  template <typename T>
  bool Top(T *out_value) {
    if (compiler_stack_.IsEmpty()) {
      return false;
    }
    return compiler_stack_.Top(out_value);
  }

  void Pop() { compiler_stack_.Pop(); }

  template <typename T>
  void PushFunc(T value) {
    function_stack_.Push(value);
  }
  template <typename T>
  bool TopFunc(T *out_value) {
    if (function_stack_.IsEmpty()) {
      return false;
    }
    return function_stack_.Top(out_value);
  }

  void PopFunc() { function_stack_.Pop(); }

  void PushLoop(SymbolPtr label_cont, SymbolPtr label_brk);

  //如果Loop栈不为空则取出cont和brk的label，并返回true，否则取不出且返回false。
  //如果只对一者感兴趣，可以将不感兴趣的设为nullptr
  //例如想取出最近循环中的brk label，而不需要cont，可以这样：
  // SymbolPtr label_brk;
  // TopLoop(nullptr,&label_brk)
  // ...
  // 如果对两者都感兴趣可以都传，例如TopLoop(&label_cont,&label_brk)
  void TopLoop(SymbolPtr *out_label_cont, SymbolPtr *out_label_brk);

  //将一个Loop Pop掉
  void PopLoop();

  //新TAC列表
  template <typename... _Args>
  inline std::shared_ptr<ThreeAddressCodeList> NewTACList(_Args &&...__args) {
    return TACFactory::Instance()->NewTACList(std::forward<_Args>(__args)...);
  }
  //新Symbol
  SymbolPtr NewSymbol(SymbolType type, std::optional<std::string> name = std::nullopt, SymbolValue value = {},
                      int offset = 0);
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

  ExpressionPtr CreateAssign(SymbolPtr var, ExpressionPtr exp);

  ExpressionPtr AccessArray(ExpressionPtr array, std::vector<ExpressionPtr> pos);
  ExpressionPtr CreateArrayInit(ExpressionPtr array, ExpressionPtr init_array);

  ExpressionPtr CreateConstExp(int n);
  ExpressionPtr CreateConstExp(float fn);
  ExpressionPtr CreateConstExp(SymbolValue v);
  void BindConstName(const std::string &name, SymbolPtr constant);
  SymbolPtr CreateText(const std::string &text);

  //如果是常量，返回来的直接可以用
  ExpressionPtr CastFloatToInt(ExpressionPtr expF);
  ExpressionPtr CastIntToFloat(ExpressionPtr expI);

  SymbolPtr CreateFunctionLabel(const std::string &name);
  SymbolPtr CreateCustomerLabel(const std::string &name);
  SymbolPtr CreateTempLabel();
  SymbolPtr CreateVariable(const std::string &name, SymbolValue::ValueType type);
  SymbolPtr CreateTempVariable(SymbolValue::ValueType type);

  SymbolPtr CreateFunctionHead(SymbolValue::ValueType ret_type, SymbolPtr func_label, ParamListPtr params);

  TACListPtr CreateFunction(SymbolPtr func_head, TACListPtr body);

  // TACListPtr CreateCall(const std::string &func_name, ArgListPtr args);
  TACListPtr CreateCallWithRet(const std::string &func_name, ArgListPtr args, SymbolPtr ret_sym);
  // if(cond!=zero)
  //  stmt
  TACListPtr CreateIf(ExpressionPtr cond, TACListPtr stmt, SymbolPtr *out_label = nullptr);
  // if(cond!=zero)
  //  stmt_true
  // else
  //  stmt_false
  TACListPtr CreateIfElse(ExpressionPtr cond, TACListPtr stmt_true, TACListPtr stmt_false,
                          SymbolPtr *out_label_true = nullptr, SymbolPtr *out_label_false = nullptr);
  TACListPtr CreateWhile(ExpressionPtr cond, TACListPtr stmt, SymbolPtr label_cont, SymbolPtr label_brk);
  TACListPtr CreateFor(TACListPtr init, ExpressionPtr cond, TACListPtr modify, TACListPtr stmt, SymbolPtr label_cont,
                       SymbolPtr label_brk);

  ExpressionPtr CreateArithmeticOperation(TACOperationType arith_op, ExpressionPtr exp1, ExpressionPtr exp2 = nullptr);

  //查找Variant
  SymbolPtr FindVariableOrConstant(const std::string &name);
  SymbolPtr FindFunctionLabel(const std::string &name);
  SymbolPtr FindCustomerLabel(const std::string &name);

  void SetTACList(TACListPtr tac_list);

  TACListPtr GetTACList();

  void SetLocation(HaveFunCompiler::Parser::location *plocation);

 private:
  using FlattenedArray = std::vector<std::pair<int, std::shared_ptr<Expression>>>;
  HaveFunCompiler::Parser::location *plocation_;
  void FlattenInitArrayImpl(FlattenedArray *out_result, ArrayDescriptorPtr array);
  FlattenedArray FlattenInitArray(ArrayDescriptorPtr array);
  int ArrayInitImpl(ExpressionPtr array, FlattenedArray::iterator &it, const FlattenedArray::iterator &end,
                    TACListPtr tac_list);
  std::string AppendScopePrefix(const std::string &name, uint64_t scope_id = (uint64_t)-1);
  SymbolPtr FindSymbolWithName(const std::string &name);
  VariantStack compiler_stack_;
  VariantStack function_stack_;
  //目前临时变量标号
  uint64_t cur_temp_var_;
  //目前临时标签标号
  uint64_t cur_temp_label_;
  uint64_t cur_symtab_id_;
  std::vector<SymbolTable> symbol_stack_;
  std::vector<std::pair<SymbolPtr, SymbolPtr>> loop_cont_brk_stack_;

  //储存text的表
  std::unordered_map<std::string, SymbolPtr> text_tab_;
  std::vector<std::string> stored_text_;

  TACListPtr tac_list_;
};

class TACRebuilder {
 public:
  TACRebuilder() = default;
  //新Symbol
  SymbolPtr NewSymbol(SymbolType type, std::optional<std::string> name = std::nullopt, SymbolValue value = {},
                      int offset = 0);
  //新TAC
  ThreeAddressCodePtr NewTAC(TACOperationType operation, SymbolPtr a = nullptr, SymbolPtr b = nullptr,
                             SymbolPtr c = nullptr);
  //新TAC列表
  template <typename... _Args>
  inline std::shared_ptr<ThreeAddressCodeList> NewTACList(_Args &&...__args) {
    return TACFactory::Instance()->NewTACList(std::forward<_Args>(__args)...);
  }
  //新建常量Symbol
  template <typename ValueType>
  SymbolPtr CreateConstSym(ValueType value) {
    return NewSymbol(SymbolType::Constant, std::nullopt, SymbolValue(value));
  }

  //插入符号表，如果已经存在该名字会覆盖
  void InsertSymbol(std::string name, SymbolPtr sym);
  //从符号表中查询，没找到返回nullptr
  SymbolPtr FindSymbol(std::string name);

  //建一个新数组
  SymbolPtr CreateArray(SymbolValue::ValueType type, int size, bool is_const = false,
                        std::optional<std::string> name = std::nullopt);
  //返回array[index]
  SymbolPtr AccessArray(SymbolPtr array, SymbolPtr index);

  void SetTACList(TACListPtr tac_list);

  TACListPtr GetTACList();

 private:
  std::unordered_map<std::string, SymbolPtr> symbol_table_;

  // //储存text的表
  // std::unordered_map<std::string, SymbolPtr> text_tab_;
  // std::vector<std::string> stored_text_;

  TACListPtr tac_list_;
};

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler