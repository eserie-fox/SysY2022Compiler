#include <gtest/gtest.h>
#include <iostream>
#include "TAC/Symbol.hh"
#include "TAC/TAC.hh"

TEST(TACBuilder, CompilerStack) {
  using namespace HaveFunCompiler::ThreeAddressCode;
  TACBuilder builder;
  builder.Push(5);
  int v;
  builder.Pop(&v);
  void *p = (void *)0x12345678;
  EXPECT_EQ(5, v);
  builder.Push(p);
  p = nullptr;
  builder.Pop(&p);
  EXPECT_EQ(p, (void *)0x12345678);
}

TEST(Symbol, SymbolValue) {
  using namespace HaveFunCompiler::ThreeAddressCode;
  SymbolValue a((int)5);
  EXPECT_ANY_THROW(a.GetFloat());
  EXPECT_NO_THROW(a.GetInt());
  SymbolValue b((int)6);
  auto c = a + b;
  EXPECT_EQ(c.GetInt(), 11);
  c = c - a;
  EXPECT_EQ(c.GetInt(), 6);
  c = c / a;
  EXPECT_EQ(c.GetInt(), 1);
  c = SymbolValue(5.0f);
  c = a * c;
  EXPECT_FLOAT_EQ(c.GetFloat(), 25.0f);
}

TEST(TACList, PrintValue) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  TACListPtr tac_list = TACFactory::Instance()->NewTACList();
  auto int_a = TACFactory::Instance()->NewSymbol(SymbolType::Variable, "a", 0, SymbolValue(0));
  auto int_b = TACFactory::Instance()->NewSymbol(SymbolType::Variable, "b", 0, SymbolValue(0));
  auto int_c = TACFactory::Instance()->NewSymbol(SymbolType::Variable, "c", 0, SymbolValue(0));
  (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Variable, int_a);
  (*tac_list) += TACFactory::Instance()->NewTAC(TACOperationType::Assign, int_b, int_a);
  cout << tac_list->ToString() << endl;
}