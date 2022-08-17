#include <gtest/gtest.h>
#include <iostream>
#include "BitContainer.hh"

using namespace HaveFunCompiler;

TEST(BitContainer, ConstructTest) {
  BitContainer bc;
  bc.set(1);
  BitContainer bc2(bc);
  EXPECT_EQ(bc2.test(1), true);
  BitContainer bc3(std::move(bc));
  EXPECT_EQ(bc3.test(1), true);
  EXPECT_EQ(bc.test(1), false);
  EXPECT_EQ((bc | bc3).test(1), true);
  EXPECT_EQ((bc & bc3).test(1), false);
  EXPECT_EQ(bc3.test(1), true);
  EXPECT_EQ(bc.test(1), false);
}