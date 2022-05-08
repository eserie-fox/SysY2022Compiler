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

TEST(TACFactory, AccessArray) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto array1 = TACFactory::Instance()->NewArrayDescriptor();
  array1->dimensions = {2, 2, 3};
  array1->base_offset = 0;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = TACFactory::Instance()->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;

  auto ret = TACFactory::Instance()->AccessArray(arraySym, {1, 1});
  auto array2 = ret->value_.GetArrayDescriptor();
  cout << array2->base_addr.lock()->name_.value() << " " << array2->dimensions[0] << " " << array2->base_offset << endl;
  for (size_t i = 0; i < 2; i++) {
    for (size_t j = 0; j < 2; j++) {
      for (size_t k = 0; k < 3; k++) {
        auto array = TACFactory::Instance()->AccessArray(arraySym, {i, j, k});
        ASSERT_EQ(i * 6 + j * 3 + k, array->value_.GetArrayDescriptor()->base_offset);
        ASSERT_EQ(0, array->value_.GetArrayDescriptor()->dimensions.size());
      }
    }
  }
  for (size_t i = 0; i < 2; i++) {
    for (size_t j = 0; j < 2; j++) {
      auto array = TACFactory::Instance()->AccessArray(arraySym, {i, j});
      ASSERT_EQ(i * 6 + j * 3, array->value_.GetArrayDescriptor()->base_offset);
      ASSERT_EQ(1, array->value_.GetArrayDescriptor()->dimensions.size());
      ASSERT_EQ(3, array->value_.GetArrayDescriptor()->dimensions[0]);
    }
  }
}

TEST(TACFactory, ArrayInit1) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto array1 = TACFactory::Instance()->NewArrayDescriptor();
  array1->dimensions = {2, 2};
  array1->base_offset = 0;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = TACFactory::Instance()->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;

  auto array2 = TACFactory::Instance()->NewArrayDescriptor();
  {
    array2->subarray->emplace(0,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(0)));
  }
  {
    array2->subarray->emplace(1,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(1)));
  }
  {
    array2->subarray->emplace(2,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(2)));
  }
  {
    array2->subarray->emplace(3,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(3)));
  }
  TACFactory::Instance()->MakeArrayInit(
      arraySym,
      TACFactory::Instance()->NewExp(
          nullptr, TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(array2))));
  ASSERT_EQ(2, array1->subarray->size());
  ASSERT_EQ(2, array1->subarray->at(0)->value_.GetArrayDescriptor()->subarray->size());
  ASSERT_EQ(2, array1->subarray->at(1)->value_.GetArrayDescriptor()->subarray->size());

  EXPECT_EQ(0, array1->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
  EXPECT_EQ(1, array1->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
  EXPECT_EQ(2, array1->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
  EXPECT_EQ(3, array1->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
}

TEST(TACFactory, ArrayInit2) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto array1 = TACFactory::Instance()->NewArrayDescriptor();
  array1->dimensions = {2, 2};
  array1->base_offset = 0;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = TACFactory::Instance()->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;

  auto array2 = TACFactory::Instance()->NewArrayDescriptor();
  {
    array2->subarray->emplace(0,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(0)));
  }
  {
    array2->subarray->emplace(1,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(1)));
  }
  {
    array2->subarray->emplace(2,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(2)));
  }
  TACFactory::Instance()->MakeArrayInit(
      arraySym,
      TACFactory::Instance()->NewExp(
          nullptr, TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(array2))));
  ASSERT_EQ(2, array1->subarray->size());
  ASSERT_EQ(2, array1->subarray->at(0)->value_.GetArrayDescriptor()->subarray->size());
  ASSERT_EQ(1, array1->subarray->at(1)->value_.GetArrayDescriptor()->subarray->size());

  EXPECT_EQ(0, array1->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
  EXPECT_EQ(1, array1->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
  EXPECT_EQ(2, array1->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
  EXPECT_EQ(0, array1->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->count(1));
}

TEST(TACFactory, ArrayInit3) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto array1 = TACFactory::Instance()->NewArrayDescriptor();
  array1->dimensions = {2, 2};
  array1->base_offset = 0;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = TACFactory::Instance()->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;

  auto array2 = TACFactory::Instance()->NewArrayDescriptor();
  {
    auto array3 = TACFactory::Instance()->NewArrayDescriptor();
    array3->subarray->emplace(0,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(0)));
    array3->subarray->emplace(1,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(1)));
    array2->subarray->emplace(
        0, TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(array3)));
  }
  {
    array2->subarray->emplace(2,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(2)));
  }
  {
    array2->subarray->emplace(3,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(3)));
  }
  TACFactory::Instance()->MakeArrayInit(
      arraySym,
      TACFactory::Instance()->NewExp(
          nullptr, TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(array2))));
  ASSERT_EQ(2, array1->subarray->size());
  ASSERT_EQ(2, array1->subarray->at(0)->value_.GetArrayDescriptor()->subarray->size());
  ASSERT_EQ(2, array1->subarray->at(1)->value_.GetArrayDescriptor()->subarray->size());

  EXPECT_EQ(0, array1->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
  EXPECT_EQ(1, array1->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
  EXPECT_EQ(2, array1->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
  EXPECT_EQ(3, array1->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
}

TEST(TACFactory, ArrayInit4) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto array1 = TACFactory::Instance()->NewArrayDescriptor();
  array1->dimensions = {2, 2};
  array1->base_offset = 0;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = TACFactory::Instance()->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;

  auto array2 = TACFactory::Instance()->NewArrayDescriptor();
  {
    auto array3 = TACFactory::Instance()->NewArrayDescriptor();
    array3->subarray->emplace(0,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(0)));
    array2->subarray->emplace(
        0, TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(array3)));
  }
  {
    array2->subarray->emplace(1,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(2)));
  }
  {
    array2->subarray->emplace(2,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(3)));
  }
  TACFactory::Instance()->MakeArrayInit(
      arraySym,
      TACFactory::Instance()->NewExp(
          nullptr, TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(array2))));
  ASSERT_EQ(2, array1->subarray->size());
  ASSERT_EQ(1, array1->subarray->at(0)->value_.GetArrayDescriptor()->subarray->size());
  ASSERT_EQ(2, array1->subarray->at(1)->value_.GetArrayDescriptor()->subarray->size());

  EXPECT_EQ(0, array1->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
  EXPECT_EQ(0, array1->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->count(1));
  EXPECT_EQ(2, array1->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
  EXPECT_EQ(3, array1->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
}

TEST(TACFactory, ArrayInit5) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto array1 = TACFactory::Instance()->NewArrayDescriptor();
  array1->dimensions = {2, 2};
  array1->base_offset = 0;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = TACFactory::Instance()->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;

  auto array2 = TACFactory::Instance()->NewArrayDescriptor();
  {
    auto array3 = TACFactory::Instance()->NewArrayDescriptor();
    array3->subarray->emplace(0,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(0)));
    array2->subarray->emplace(
        0, TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(array3)));
  }
  {
    auto array3 = TACFactory::Instance()->NewArrayDescriptor();
    array3->subarray->emplace(0,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(2)));
    array2->subarray->emplace(1,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(2)));
  }
  TACFactory::Instance()->MakeArrayInit(
      arraySym,
      TACFactory::Instance()->NewExp(
          nullptr, TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(array2))));
  ASSERT_EQ(2, array1->subarray->size());
  ASSERT_EQ(1, array1->subarray->at(0)->value_.GetArrayDescriptor()->subarray->size());
  ASSERT_EQ(1, array1->subarray->at(1)->value_.GetArrayDescriptor()->subarray->size());

  EXPECT_EQ(0, array1->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
  EXPECT_EQ(0, array1->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->count(1));
  EXPECT_EQ(2, array1->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
  EXPECT_EQ(0, array1->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->count(1));
}

TEST(TACFactory, ArrayInit6) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto array1 = TACFactory::Instance()->NewArrayDescriptor();
  array1->dimensions = {2, 2};
  array1->base_offset = 0;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = TACFactory::Instance()->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;

  auto array2 = TACFactory::Instance()->NewArrayDescriptor();
  {
    auto array3 = TACFactory::Instance()->NewArrayDescriptor();
    array2->subarray->emplace(
        0, TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(array3)));
  }
  {
    auto array3 = TACFactory::Instance()->NewArrayDescriptor();
    array3->subarray->emplace(0,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(2)));
    array2->subarray->emplace(1,
                              TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(2)));
  }
  TACFactory::Instance()->MakeArrayInit(
      arraySym,
      TACFactory::Instance()->NewExp(
          nullptr, TACFactory::Instance()->NewSymbol(SymbolType::Constant, std::nullopt, 0, SymbolValue(array2))));
  ASSERT_EQ(2, array1->subarray->size());
  ASSERT_EQ(0, array1->subarray->at(0)->value_.GetArrayDescriptor()->subarray->size());
  ASSERT_EQ(1, array1->subarray->at(1)->value_.GetArrayDescriptor()->subarray->size());

  EXPECT_EQ(0, array1->subarray->at(0)->value_.GetArrayDescriptor()->subarray->count(0));
  EXPECT_EQ(0, array1->subarray->at(0)->value_.GetArrayDescriptor()->subarray->count(1));
  EXPECT_EQ(2, array1->subarray->at(1)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->value_.GetInt());
  EXPECT_EQ(0, array1->subarray->at(1)->value_.GetArrayDescriptor()->subarray->count(1));
}