#include <gtest/gtest.h>
#include <iostream>
#include "TAC/Symbol.hh"
#include "TAC/TAC.hh"

TEST(TACBuilder, CompilerStack) {
    using namespace HaveFunCompiler::ThreeAddressCode;
    TACBuilder builder;
    builder.Push(5);
    int v;
    builder.Top(&v);
    builder.Pop();
    void *p = (void *)0x12345678;
    EXPECT_EQ(5, v);
    builder.Push(p);
    p = nullptr;
    builder.Top(&p);
    builder.Pop();
    EXPECT_EQ(p, (void *)0x12345678);
}

TEST(Symbol, SymbolValue) {
  using namespace HaveFunCompiler::ThreeAddressCode;
  SymbolValue v((int)5);
  EXPECT_ANY_THROW(v.GetFloat());
  EXPECT_NO_THROW(v.GetInt());
}