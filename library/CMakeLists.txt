
FILE(GLOB_RECURSE srcs "source/*.cc")
FILE(GLOB_RECURSE hdrs "include/*.h")

add_library(HaveFunLib STATIC ${srcs} ${hdrs})

target_include_directories(HaveFunLib 
    PUBLIC 
    "include")

target_link_libraries(HaveFunLib
    PUBLIC
    "${CMAKE_THREAD_LIBS_INIT}")

target_compile_options(HaveFunLib PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)