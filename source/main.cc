#include <cstdlib>
#include <cstring>
#include <iostream>

#include "Driver.hh"

int main([[maybe_unused]] const int arg, [[maybe_unused]] const char **argv) {
  // HaveFunCompiler::Parser::Driver driver;
  // driver.parse("test.txt");
  // driver.print(std::cout) << "\n";
  [[maybe_unused]] int a[3][4] = {{1}, {2}, {4, 5}};
  for (int i = 0; i < 12; i++) {
    std::cout << ((int *)a)[i] << std::endl;
  }
  return 0;
}