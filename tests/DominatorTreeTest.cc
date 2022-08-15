#include <gtest/gtest.h>
#include <iostream>
#include "DominatorTree.hh"

using namespace HaveFunCompiler;

TEST(DominatorTree, SimpleTest) {
  {
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
  {
    DominatorTree dt;
    dt.AddEdge(1, 2);
    dt.AddEdge(2, 3);
    dt.AddEdge(3, 4);
    dt.AddEdge(3, 5);
    dt.AddEdge(4, 6);
    dt.AddEdge(5, 6);
    dt.AddEdge(6, 7);
    dt.AddEdge(7, 8);
    dt.AddEdge(6, 2);
    dt.Build(1);
    // int dominator_ans[] = {0, 0, 1, 2, 3, 3, 3, 6, 7};
    for (int i = 2; i <= 8; i++) {
    //   ASSERT_EQ(dominator_ans[i], dt.get_dominator(i));
      std::cout << dt.get_dominator(i) << " " << i << std::endl;
    }
  }
}