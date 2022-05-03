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
using ExpListPtr = std::shared_ptr<ExpressionList>;

class TACFactory {
  NONCOPYABLE(TACFactory)
 public:
  static TACFactory *Instance() {
    static TACFactory factory;
    return &factory;
  }
  template <typename... _Args>
  inline std::shared_ptr<ThreeAddressCodeList> NewTACList(_Args &&...__args) {
    return std::make_shared<ThreeAddressCodeList>(std::forward<_Args>(__args)...);
  }
  //新Symbol
  SymbolPtr NewSymbol(SymbolType type, std::optional<std::string> name = std::nullopt, int offset = 0,
                      SymbolValue value = {});
  //新TAC
  ThreeAddressCodePtr NewTAC(TACOperationType operation, SymbolPtr a, SymbolPtr b = nullptr, SymbolPtr c = nullptr);
  ExpressionPtr NewExp(TACListPtr tac, SymbolPtr ret);

  TACListPtr MakeFunction(SymbolPtr label, TACListPtr args, TACListPtr body);
  ExpressionPtr MakeAssign(SymbolPtr var, ExpressionPtr exp);
  TACListPtr MakeCall(const std::string &name, ExpListPtr args);
  ExpressionPtr MakeCallWithRet(const std::string &name, ExpListPtr args, SymbolPtr ret_sym);
  TACListPtr MakeIf(ExpressionPtr cond, SymbolPtr label, TACListPtr stmt);
  TACListPtr MakeTest(ExpressionPtr cond, SymbolPtr label_true, TACListPtr stmt_true, SymbolPtr label_false,
                      TACListPtr stmt_false);
  TACListPtr MakeWhile(ExpressionPtr cond, SymbolPtr label_cont, SymbolPtr label_brk, TACListPtr stmt);
  TACListPtr MakeFor(TACListPtr init, ExpressionPtr cond, TACListPtr modify, SymbolPtr label_cont, SymbolPtr label_brk,
                     TACListPtr stmt);

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
  void Pop(T *p) {
    compiler_stack_.Pop(p);
  }

  SymbolPtr CreateConst(int n);
  SymbolPtr CreateConst(float fn);
  SymbolPtr CreateText(const std::string &text);

  SymbolPtr CreateFuncLabel(const char *name);
  SymbolPtr CreateCustomerLabel(const char *name);
  SymbolPtr CreateTempLabel();
  SymbolPtr CreateFunction(const char *name);
  SymbolPtr CreateVariable(const char *name);
  SymbolPtr CreateParameter(const char *name);

  //查找Variant
  SymbolPtr FindVariant(const char *name);

 private:
  UnionStack compiler_stack_;
  //目前临时变量标号
  int cur_temp_var_;
  //目前临时标签标号
  int cur_temp_label_;
  using SymbolTable = std::unordered_map<std::string, std::shared_ptr<Symbol>>;
  std::vector<SymbolTable> symbol_stack_;
  //储存text的表
  std::unordered_map<std::string, std::shared_ptr<Symbol>> text_tab_;
  std::vector<std::string> stored_text_;
};

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler