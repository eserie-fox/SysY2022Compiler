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
  auto builder = make_unique<TACBuilder>();
  TACListPtr tac_list = builder->NewTACList();
  auto int_a = builder->NewSymbol(SymbolType::Variable, "a", 0, SymbolValue(0));
  auto int_b = builder->NewSymbol(SymbolType::Variable, "b", 0, SymbolValue(0));
  auto int_c = builder->NewSymbol(SymbolType::Variable, "c", 0, SymbolValue(0));
  (*tac_list) += builder->NewTAC(TACOperationType::Variable, int_a);
  (*tac_list) += builder->NewTAC(TACOperationType::Assign, int_b, int_a);
  cout << tac_list->ToString() << endl;
}

TEST(TACBuilder, ConstAccessArray) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto builder = make_unique<TACBuilder>();
  auto array1 = builder->NewArrayDescriptor();
  array1->dimensions = {2, 2, 3};
  array1->base_offset = builder->CreateConstExp(0)->ret;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = builder->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;
  auto arrayExp = builder->NewExp(builder->NewTACList(), arraySym);

  auto ret = builder->AccessArray(arrayExp, {builder->CreateConstExp(1), builder->CreateConstExp(1)})->ret;
  auto array2 = ret->value_.GetArrayDescriptor();
  cout << array2->base_addr.lock()->name_.value() << " " << array2->dimensions[0] << " " << array2->base_offset->get_name() << endl;
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 3; k++) {
        auto array = builder->AccessArray(arrayExp, {builder->CreateConstExp(i), builder->CreateConstExp(j), builder->CreateConstExp(k)})->ret;
        ASSERT_EQ(i * 6 + j * 3 + k, array->value_.GetArrayDescriptor()->base_offset->value_.GetInt());
        ASSERT_EQ(0, array->value_.GetArrayDescriptor()->dimensions.size());
      }
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      auto array = builder->AccessArray(arrayExp, {builder->CreateConstExp(i), builder->CreateConstExp(j)})->ret;
      ASSERT_EQ(i * 6 + j * 3, array->value_.GetArrayDescriptor()->base_offset->value_.GetInt());
      ASSERT_EQ(1, array->value_.GetArrayDescriptor()->dimensions.size());
      ASSERT_EQ(3, array->value_.GetArrayDescriptor()->dimensions[0]);
    }
  }
}

TEST(TACBuilder, VarAccessArray) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto builder = make_unique<TACBuilder>();
  auto array1 = builder->NewArrayDescriptor();
  array1->dimensions = {2, 2, 3};
  array1->base_offset = builder->CreateConstExp(0)->ret;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = builder->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;
  auto arrayExp = builder->NewExp(builder->NewTACList(), arraySym);

  auto tmpV1 = builder->CreateTempVariable(SymbolValue::ValueType::Int);
  auto tmpV2 = builder->CreateTempVariable(SymbolValue::ValueType::Int);
  auto tmpV3 = builder->CreateTempVariable(SymbolValue::ValueType::Int);
  auto tmpE1 = builder->NewExp(builder->NewTACList(builder->NewTAC(TACOperationType::Variable, tmpV1)), tmpV1);
  auto tmpE2 = builder->NewExp(builder->NewTACList(builder->NewTAC(TACOperationType::Variable, tmpV2)), tmpV2);
  auto tmpE3 = builder->NewExp(builder->NewTACList(builder->NewTAC(TACOperationType::Variable, tmpV3)), tmpV3);
  auto ret = builder->AccessArray(arrayExp, {tmpE1, tmpE2, tmpE3});
  auto nexp = builder->CreateAssign(ret->ret, builder->CreateConstExp(5));
  nexp->tac = builder->NewTACList(*ret->tac + *nexp->tac);
  cout << nexp->tac->ToString() << endl;
  // cout << ret->tac->ToString()<< "\n"<< ret->ret->value_.GetArrayDescriptor()->base_offset->get_name() << endl;
  // for (int i = 0; i < 2; i++) {
  //   for (int j = 0; j < 2; j++) {
  //     for (int k = 0; k < 3; k++) {
  //       auto array = builder
  //                        ->AccessArray(arrayExp, {builder->CreateConstExp(i), builder->CreateConstExp(j),
  //                                                 builder->CreateConstExp(k)})
  //                        ->ret;
  //       ASSERT_EQ(i * 6 + j * 3 + k, array->value_.GetArrayDescriptor()->base_offset->value_.GetInt());
  //       ASSERT_EQ(0, array->value_.GetArrayDescriptor()->dimensions.size());
  //     }
  //   }
  // }
  // for (int i = 0; i < 2; i++) {
  //   for (int j = 0; j < 2; j++) {
  //     auto array = builder->AccessArray(arrayExp, {builder->CreateConstExp(i), builder->CreateConstExp(j)})->ret;
  //     ASSERT_EQ(i * 6 + j * 3, array->value_.GetArrayDescriptor()->base_offset->value_.GetInt());
  //     ASSERT_EQ(1, array->value_.GetArrayDescriptor()->dimensions.size());
  //     ASSERT_EQ(3, array->value_.GetArrayDescriptor()->dimensions[0]);
  //   }
  // }
}

TEST(TACBuilder, ConstArrayInit1) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto builder = make_unique<TACBuilder>();
  auto array1 = builder->NewArrayDescriptor();
  array1->dimensions = {2, 2};
  array1->base_offset = builder->CreateConstExp(0)->ret;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = builder->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;
  auto arrayExp = builder->NewExp(builder->NewTACList(), arraySym);

  auto array2 = builder->NewArrayDescriptor();
  { array2->subarray->emplace(0, builder->CreateConstExp(0)); }
  { array2->subarray->emplace(1, builder->CreateConstExp(1)); }
  { array2->subarray->emplace(2, builder->CreateConstExp(2)); }
  { array2->subarray->emplace(3, builder->CreateConstExp(3)); }
  builder->MakeArrayInit(arrayExp, builder->CreateConstExp(array2));
  ASSERT_EQ(2, array1->subarray->size());
  ASSERT_EQ(2, array1->subarray->at(0)->ret->value_.GetArrayDescriptor()->subarray->size());
  ASSERT_EQ(2, array1->subarray->at(1)->ret->value_.GetArrayDescriptor()->subarray->size());

  EXPECT_EQ(0, array1->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(1, array1->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(2, array1->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(3, array1->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  std::cout << arrayExp->tac->ToString() << std::endl;
}

TEST(TACBuilder, ConstArrayInit2) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto builder = make_unique<TACBuilder>();
  auto array1 = builder->NewArrayDescriptor();
  array1->dimensions = {2, 2};
  array1->base_offset = builder->CreateConstExp(0)->ret;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = builder->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;
  auto arrayExp = builder->NewExp(builder->NewTACList(), arraySym);

  auto array2 = builder->NewArrayDescriptor();
  { array2->subarray->emplace(0, builder->CreateConstExp(0)); }
  { array2->subarray->emplace(1, builder->CreateConstExp(1)); }
  { array2->subarray->emplace(2, builder->CreateConstExp(2)); }
  builder->MakeArrayInit(arrayExp, builder->CreateConstExp(array2));
  ASSERT_EQ(2, array1->subarray->size());
  ASSERT_EQ(2, array1->subarray->at(0)->ret->value_.GetArrayDescriptor()->subarray->size());
  ASSERT_EQ(1, array1->subarray->at(1)->ret->value_.GetArrayDescriptor()->subarray->size());

  EXPECT_EQ(0, array1->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(1, array1->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(2, array1->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(0, array1->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->count(1));
  std::cout << arrayExp->tac->ToString() << std::endl;
}

TEST(TACBuilder, ConstArrayInit3) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto builder = make_unique<TACBuilder>();
  auto array1 = builder->NewArrayDescriptor();
  array1->dimensions = {2, 2};
  array1->base_offset = builder->CreateConstExp(0)->ret;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = builder->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;
  auto arrayExp = builder->NewExp(builder->NewTACList(), arraySym);

  auto array2 = builder->NewArrayDescriptor();
  {
    auto array3 = builder->NewArrayDescriptor();
    array3->subarray->emplace(0, builder->CreateConstExp(1));
    array3->subarray->emplace(1, builder->CreateConstExp(0));
    array2->subarray->emplace(0, builder->CreateConstExp(array3));
  }
  {
    auto array3 = builder->NewArrayDescriptor();
    array3->subarray->emplace(0, builder->CreateConstExp(2));
    array3->subarray->emplace(1, builder->CreateConstExp(3));
    array2->subarray->emplace(1, builder->CreateConstExp(array3));
  }
  builder->MakeArrayInit(arrayExp, builder->CreateConstExp(array2));
  ASSERT_EQ(2, array1->subarray->size());
  ASSERT_EQ(2, array1->subarray->at(0)->ret->value_.GetArrayDescriptor()->subarray->size());
  ASSERT_EQ(2, array1->subarray->at(1)->ret->value_.GetArrayDescriptor()->subarray->size());

  EXPECT_EQ(1, array1->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(0, array1->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(2, array1->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(3, array1->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  std::cout << arrayExp->tac->ToString() << std::endl;
}

TEST(TACBuilder, ConstArrayInit4) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto builder = make_unique<TACBuilder>();
  auto array1 = builder->NewArrayDescriptor();
  array1->dimensions = {2, 2};
  array1->base_offset = builder->CreateConstExp(0)->ret;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = builder->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;
  auto arrayExp = builder->NewExp(builder->NewTACList(), arraySym);

  auto array2 = builder->NewArrayDescriptor();
  {
    auto array3 = builder->NewArrayDescriptor();
    array3->subarray->emplace(0, builder->CreateConstExp(1));
    array2->subarray->emplace(0, builder->CreateConstExp(array3));
  }
  {
    array2->subarray->emplace(1, builder->CreateConstExp(2));
    array2->subarray->emplace(2, builder->CreateConstExp(3));
  }
  builder->MakeArrayInit(arrayExp, builder->CreateConstExp(array2));
  ASSERT_EQ(2, array1->subarray->size());
  ASSERT_EQ(1, array1->subarray->at(0)->ret->value_.GetArrayDescriptor()->subarray->size());
  ASSERT_EQ(2, array1->subarray->at(1)->ret->value_.GetArrayDescriptor()->subarray->size());

  EXPECT_EQ(1, array1->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(0, array1->subarray->at(0)->ret->value_.GetArrayDescriptor()->subarray->count(1));
  EXPECT_EQ(2, array1->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(3, array1->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  std::cout << arrayExp->tac->ToString() << std::endl;
}

TEST(TACBuilder, ConstArrayInit5) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto builder = make_unique<TACBuilder>();
  auto array1 = builder->NewArrayDescriptor();
  array1->dimensions = {2, 2};
  array1->base_offset = builder->CreateConstExp(0)->ret;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = builder->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;
  auto arrayExp = builder->NewExp(builder->NewTACList(), arraySym);

  auto array2 = builder->NewArrayDescriptor();
  {
    auto array3 = builder->NewArrayDescriptor();
    array3->subarray->emplace(0, builder->CreateConstExp(1));
    array2->subarray->emplace(0, builder->CreateConstExp(array3));
  }
  {
    auto array3 = builder->NewArrayDescriptor();
    array3->subarray->emplace(0, builder->CreateConstExp(2));
    array2->subarray->emplace(1, builder->CreateConstExp(array3));
  }
  builder->MakeArrayInit(arrayExp, builder->CreateConstExp(array2));
  ASSERT_EQ(2, array1->subarray->size());
  ASSERT_EQ(1, array1->subarray->at(0)->ret->value_.GetArrayDescriptor()->subarray->size());
  ASSERT_EQ(1, array1->subarray->at(1)->ret->value_.GetArrayDescriptor()->subarray->size());

  EXPECT_EQ(1, array1->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(0, array1->subarray->at(0)->ret->value_.GetArrayDescriptor()->subarray->count(1));
  EXPECT_EQ(2, array1->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(0, array1->subarray->at(1)->ret->value_.GetArrayDescriptor()->subarray->count(1));
  std::cout << arrayExp->tac->ToString() << std::endl;
}

TEST(TACBuilder, ConstArrayInit6) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto builder = make_unique<TACBuilder>();
  auto array1 = builder->NewArrayDescriptor();
  array1->dimensions = {2, 2};
  array1->base_offset = builder->CreateConstExp(0)->ret;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = builder->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;
  auto arrayExp = builder->NewExp(builder->NewTACList(), arraySym);

  auto array2 = builder->NewArrayDescriptor();
  {
    auto array3 = builder->NewArrayDescriptor();
    array2->subarray->emplace(0, builder->CreateConstExp(array3));
  }
  {
    auto array3 = builder->NewArrayDescriptor();
    array3->subarray->emplace(0, builder->CreateConstExp(2));
    array2->subarray->emplace(1, builder->CreateConstExp(array3));
  }
  builder->MakeArrayInit(arrayExp, builder->CreateConstExp(array2));
  ASSERT_EQ(2, array1->subarray->size());
  ASSERT_EQ(0, array1->subarray->at(0)->ret->value_.GetArrayDescriptor()->subarray->size());
  ASSERT_EQ(1, array1->subarray->at(1)->ret->value_.GetArrayDescriptor()->subarray->size());

  EXPECT_EQ(0, array1->subarray->at(0)->ret->value_.GetArrayDescriptor()->subarray->count(0));
  EXPECT_EQ(0, array1->subarray->at(0)->ret->value_.GetArrayDescriptor()->subarray->count(1));
  EXPECT_EQ(2, array1->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(0, array1->subarray->at(1)->ret->value_.GetArrayDescriptor()->subarray->count(1));
  std::cout << arrayExp->tac->ToString() << std::endl;
}

TEST(TACBuilder, VarArrayInit1) {
  using namespace std;
  using namespace HaveFunCompiler::ThreeAddressCode;
  auto builder = make_unique<TACBuilder>();
  auto array1 = builder->NewArrayDescriptor();
  array1->dimensions = {2, 2};
  array1->base_offset = builder->CreateConstExp(0)->ret;
  array1->value_type = SymbolValue::ValueType::Int;
  auto arraySym = builder->NewSymbol(SymbolType::Variable, "hahaha", 0, SymbolValue(array1));
  array1->base_addr = arraySym;
  auto arrayExp = builder->NewExp(builder->NewTACList(), arraySym);

  auto array2 = builder->NewArrayDescriptor();
  auto tmpV1 = builder->CreateTempVariable(SymbolValue::ValueType::Int);
  auto tmpE1 = builder->NewExp(builder->NewTACList(builder->NewTAC(TACOperationType::Variable, tmpV1)), tmpV1);
  auto tmpV2 = builder->CreateTempVariable(SymbolValue::ValueType::Int);
  auto tmpE2 = builder->NewExp(builder->NewTACList(builder->NewTAC(TACOperationType::Variable, tmpV2)), tmpV2);
  { array2->subarray->emplace(0, builder->CreateConstExp(0)); }
  { array2->subarray->emplace(1, builder->CreateConstExp(1)); }
  { array2->subarray->emplace(2, builder->CreateArithmeticOperation(TACOperationType::Add, tmpE1, tmpE2)); }
  { array2->subarray->emplace(3, builder->CreateConstExp(3)); }
  builder->MakeArrayInit(arrayExp, builder->CreateConstExp(array2));
  ASSERT_EQ(2, array1->subarray->size());
  ASSERT_EQ(2, array1->subarray->at(0)->ret->value_.GetArrayDescriptor()->subarray->size());
  ASSERT_EQ(2, array1->subarray->at(1)->ret->value_.GetArrayDescriptor()->subarray->size());

  EXPECT_EQ(0, array1->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(1, array1->subarray->at(0)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  EXPECT_EQ(SymbolValue::ValueType::Int, array1->subarray->at(1)
                                             ->ret->value_.GetArrayDescriptor()
                                             ->subarray->at(0)
                                             ->ret->value_.GetArrayDescriptor()
                                             ->subarray->at(0)
                                             ->ret->value_.Type());
  EXPECT_EQ(3, array1->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(1)
                   ->ret->value_.GetArrayDescriptor()
                   ->subarray->at(0)
                   ->ret->value_.GetInt());
  std::cout << arrayExp->tac->ToString() << std::endl;
}