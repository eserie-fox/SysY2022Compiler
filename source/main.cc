#include <cstdlib>
#include <cstring>
#include <iostream>

#include "Driver.hh"

int main([[maybe_unused]] const int arg, [[maybe_unused]] const char **argv) {
  HaveFunCompiler::Parser::Driver driver;
  if (driver.parse("test.txt")) {
    driver.print(std::cout) << "\n";
  }
  return 0;
}