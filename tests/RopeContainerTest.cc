#include <gtest/gtest.h>
#include <iostream>
#include "ASM/Common.hh"
#include "RopeContainer.hh"

using namespace HaveFunCompiler;

TEST(RopeContainer, ConstructTest) {
  const int MAXN = 100000;
  RopeContainer<int> rc(MAXN);
  auto encode = [](int i) -> int { return ((((i + 6) & (i * 26 + 7)) | i) >> (i & 3)) & 3; };
  for (int i = 0; i < MAXN; i++) {
    if (encode(i) == 3) {
      rc.set(i);
    }
  }
  std::vector<RopeContainer<int>> rcs{rc};
  std::vector<int> posvec;
  for (int i = 0; i < 3000; i++) {
    rcs.emplace_back(rcs.back());
    int pos = rand() % MAXN;
    posvec.push_back(pos);
    if (rcs.back().test(pos)) {
      rcs.back().reset(pos);
    } else {
      rcs.back().set(pos);
    }
  }
  for (auto x : posvec) {
    if (rc.test(x)) {
      rc.reset(x);
    } else {
      rc.set(x);
    }
  }
  ASSERT_EQ(rc, rcs.back());
}
