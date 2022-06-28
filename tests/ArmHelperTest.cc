#include <gtest/gtest.h>
#include "ASM/arm/ArmHelper.hh"

using namespace HaveFunCompiler;
using namespace HaveFunCompiler::AssemblyBuilder;

TEST(ArmHelper, IsImmediateValue) {
  std::vector<uint32_t> vals = {4000016, 3997696, 2320, 510, 1540096, static_cast<uint32_t>(-1540096)};
  std::vector<char> ans = {0, 1, 1, 0, 1, 0};
  for (unsigned i = 0; i < vals.size(); i++) {
    char res = ArmHelper::IsImmediateValue(vals[i]);
    EXPECT_EQ(ans[i], res);
  }
}

TEST(ArmHelper, DivideIntoImmediateValues) {
  srand(time(NULL));
  std::vector<uint32_t> vals = {4000016, 1545451, 6597522, 6458445, 51545269, 32156489};
  for (int i = 0; i < 100000; i++) {
    vals.push_back(rand() << 15 | rand());
  }
  for (auto val : vals) {
    auto res = ArmHelper::DivideIntoImmediateValues(val);
    uint32_t sum = 0;
    for (auto v : res) {
      sum += v;
    }
    ASSERT_EQ(val, sum);
    for (auto v : res) {
      ASSERT_TRUE(ArmHelper::IsImmediateValue(v));
    }
  }
}