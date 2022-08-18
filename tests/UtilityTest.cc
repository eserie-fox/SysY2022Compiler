#include <gtest/gtest.h>
#include <iostream>
#include "Utility.hh"

using namespace HaveFunCompiler;

TEST(Utility, EraseTest1) {
  std::vector<int> a{1, 2, 3, 4, 5, 6, 2, 2};
  Erase(a, 2);
  std::vector<int> ans{1, 3, 4, 5, 6};
  EXPECT_EQ(a, ans);
}

TEST(Utility, EraseTest2) {
  std::vector<int> a{1, 2, 3, 4, 5, 6, 2, 2};
  Erase(a, 3);
  std::vector<int> ans{1, 2, 4, 5, 6, 2, 2};
  EXPECT_EQ(a, ans);
}

TEST(Utility, EraseTest3) {
  std::vector<int> a{1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  Erase(a, 1);
  std::vector<int> ans{};
  EXPECT_EQ(a, ans);
}