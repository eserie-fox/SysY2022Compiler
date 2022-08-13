#include <gtest/gtest.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "ASM/Optimizer.hh"
#include "TAC/TAC.hh"

using namespace HaveFunCompiler;
using namespace ThreeAddressCode;
using namespace AssemblyBuilder;

TEST(SimpleOptimizer, SimpleTest) {
  auto taclist = std::make_shared<ThreeAddressCodeList>();
  auto NewTAC = [](TACOperationType type, SymbolPtr a, SymbolPtr b, SymbolPtr c) {
    auto tac = std::make_shared<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>();
    tac->operation_ = type;
    tac->a_ = a;
    tac->b_ = b;
    tac->c_ = c;
    return tac;
  };
  auto NewSymbol = [](SymbolType type = SymbolType::Variable, std::string name = "1",
                      SymbolValue value = SymbolValue(1), int offset = 0) {
    SymbolPtr sym = std::make_shared<Symbol>();
    sym->type_ = type;
    sym->name_ = name;
    sym->offset_ = offset;
    sym->value_ = value;
    return sym;
  };
  (*taclist) +=
      NewTAC(TACOperationType::Add, NewSymbol(), NewSymbol(), NewSymbol(SymbolType::Constant, "1", SymbolValue(0)));
  (*taclist) +=
      NewTAC(TACOperationType::Add, NewSymbol(), NewSymbol(), NewSymbol(SymbolType::Constant, "1", SymbolValue(1)));
  (*taclist) +=
      NewTAC(TACOperationType::Mul, NewSymbol(), NewSymbol(), NewSymbol(SymbolType::Constant, "1", SymbolValue(0)));
  (*taclist) +=
      NewTAC(TACOperationType::Mul, NewSymbol(), NewSymbol(), NewSymbol(SymbolType::Constant, "1", SymbolValue(1)));
  (*taclist) +=
      NewTAC(TACOperationType::Mul, NewSymbol(), NewSymbol(), NewSymbol(SymbolType::Constant, "1", SymbolValue(2)));
  auto sym = NewSymbol();
  (*taclist) += NewTAC(TACOperationType::Mul, sym, sym, NewSymbol(SymbolType::Constant, "1", SymbolValue(1)));

  SimpleOptimizer opt(taclist, taclist->begin(), taclist->end());
  opt.optimize();
  std::vector<TACPtr> res;
  for (auto it = taclist->begin(); it != taclist->end(); ++it) {
    auto tacptr = *it;
    res.push_back(tacptr);
  }
  ASSERT_EQ(5, res.size());
  ASSERT_EQ(res[0]->operation_, TACOperationType::Assign);
  ASSERT_EQ(res[1]->operation_, TACOperationType::Add);
  ASSERT_EQ(res[2]->operation_, TACOperationType::Assign);
  ASSERT_EQ(res[2]->b_->value_.GetInt(), 0);
  ASSERT_EQ(res[3]->operation_, TACOperationType::Assign);
}