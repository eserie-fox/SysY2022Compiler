FILE(GLOB_RECURSE srcs "*.cc")
FILE(GLOB_RECURSE hdrs "include/*.h")

add_executable(hfb ${srcs} ${hdrs})

target_compile_options(hfb PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

target_include_directories(hfb 
    PRIVATE
    "include")

target_link_libraries(hfb 
    PRIVATE
    HaveFunLib)

