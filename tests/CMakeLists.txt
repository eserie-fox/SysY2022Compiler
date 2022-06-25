# GoogleTest requires at least C++11
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

FILE(GLOB Sources "*.cc")

add_executable(Tests ${Sources})
target_link_libraries(Tests gtest_main HaveFunLib)

target_include_directories(Tests PRIVATE "../source/include" "../library/include")

include(GoogleTest)
gtest_discover_tests(Tests)

