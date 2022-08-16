#include <gtest/gtest.h>
#include <iostream>
#include "DominatorTree.hh"

using namespace HaveFunCompiler;

TEST(DominatorTree, SimpleTest1) {
  DominatorTree dt;
  dt.AddEdge(1, 2);
  dt.AddEdge(2, 3);
  dt.AddEdge(3, 4);
  dt.AddEdge(3, 5);
  dt.AddEdge(4, 6);
  dt.AddEdge(5, 6);
  dt.AddEdge(6, 7);
  dt.AddEdge(7, 8);
  dt.AddEdge(7, 2);
  dt.Build(1);
  int dominator_ans[] = {0, 0, 1, 2, 3, 3, 3, 6, 7};
  for (int i = 2; i <= 8; i++) {
    ASSERT_EQ(dominator_ans[i], dt.get_dominator(i));
    // std::cout << dt.get_dominator(i) << " " << i << std::endl;
  }
}

TEST(DominatorTree, SimpleTest2) {
  DominatorTree dt;
  dt.AddEdge(1, 2);
  dt.AddEdge(2, 3);
  dt.AddEdge(3, 4);
  dt.AddEdge(3, 5);
  dt.AddEdge(3, 6);
  dt.AddEdge(4, 7);
  dt.AddEdge(7, 8);
  dt.AddEdge(7, 9);
  dt.AddEdge(7, 10);
  dt.AddEdge(5, 6);
  dt.AddEdge(6, 8);
  dt.AddEdge(7, 8);
  dt.AddEdge(4, 1);
  dt.AddEdge(3, 6);
  dt.AddEdge(5, 3);
  dt.Build(1);
  int dominator_ans[] = {0, 10, 9, 8, 4, 1, 1, 3, 1, 1, 1};
  int sum[11];
  for (int i = 1; i <= 10; i++) {
    sum[i] = 1;
  }
  for (int i = 10; i > 1; i--) {
    sum[dt.get_dominator(i)] += sum[i];
  }
  for (int i = 1; i <= 10; i++) {
    ASSERT_EQ(dominator_ans[i], sum[i]);
  }
}