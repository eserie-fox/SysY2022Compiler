

find_package(BISON)
find_package(FLEX)

FILE(GLOB_RECURSE ysrcs "*.y")
FILE(GLOB_RECURSE lsrcs "*.l")
LIST(GET ysrcs 0 ysrc)
LIST(GET lsrcs 0 lsrc)

BISON_TARGET(parser ${ysrc} ${CMAKE_CURRENT_BINARY_DIR}/HFParser.cpp
            DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/HFParser.h)

FLEX_TARGET(lexer ${lsrc} ${CMAKE_CURRENT_BINARY_DIR}/HFLexer.cpp
            COMPILE_FLAGS )

ADD_FLEX_BISON_DEPENDENCY(lexer parser)

set(srcs    ${CMAKE_CURRENT_BINARY_DIR}/HFParser.cpp 
            ${CMAKE_CURRENT_BINARY_DIR}/HFLexer.cpp)

add_library(HaveFunParser STATIC ${srcs})

target_include_directories(HaveFunParser
    PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR})

find_library(LEX_LIB lexlib)
target_link_libraries(HaveFunParser
    PUBLIC
    ${lexlib})
