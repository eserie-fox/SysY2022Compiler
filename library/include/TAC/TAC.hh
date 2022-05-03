#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include "MacroUtil.hh"
#include "TAC/Expression.hh"
#include "TAC/Symbol.hh"
#include "TAC/ThreeAddressCode.hh"

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
  //新Symbol
  SymbolPtr NewSymbol(SymbolType type, std::optional<std::string> name = std::nullopt, int offset = 0,
                      SymbolValue value = {});
  //新TAC
  ThreeAddressCodePtr NewTAC(TACOperationType operation, SymbolPtr a, SymbolPtr b = nullptr, SymbolPtr c = nullptr);
  ExpressionPtr NewExp(TACListPtr tac, SymbolPtr ret);

  TACListPtr MakeFunction(SymbolPtr label, TACListPtr args, TACListPtr body);
  ExpressionPtr MakeAssign(SymbolPtr var, ExpressionPtr exp);
  TACListPtr MakeCall(const std::string &name, ExpListPtr args);
  TACListPtr MakeIf(ExpressionPtr cond, TACListPtr stmt);
  TACListPtr MakeTest(ExpressionPtr cond, TACListPtr stmt_true, TACListPtr stmt_false);
  TACListPtr MakeWhile(ExpressionPtr cond, TACListPtr stmt);
  TACListPtr MakeFor(TACListPtr init, ExpressionPtr cond, TACListPtr modify, TACListPtr stmt);
  ExpressionPtr MakeCallWithRet(const std::string &name, ExpListPtr args);

 private:
  TACFactory() = default;
};

class TACBuilder {
  NONCOPYABLE(TACBuilder)
 public:
  TACBuilder();

  SymbolPtr CreateConst(int n);
  SymbolPtr CreateConst(float fn);
  SymbolPtr CreateText(const char *text);
  SymbolPtr CreateLabel(const char *name);
  SymbolPtr CreateFunction(const char *name);
  SymbolPtr CreateVariable(const char *name);
  SymbolPtr CreateParameter(const char *name);

  //查找Variant
  SymbolPtr FindVariant(const char *name);

 private:
  //目前作用域
  enum class ScopeType { Global, Local } scope_;
  //目前临时变量标号
  int cur_temp_var_;
  //目前临时标签标号
  int cur_temp_label_;
  using SymbolTable = std::unordered_map<std::string, std::shared_ptr<Symbol>>;
  SymbolTable global_symbols_;
  SymbolTable local_symbols_;
  ThreeAddressCodeList tac_list_;
};

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler