

find_package(BISON)
find_package(FLEX)

FILE(GLOB_RECURSE ysrcs "*.yy")
FILE(GLOB_RECURSE lsrcs "*.ll")
LIST(GET ysrcs 0 ysrc)
LIST(GET lsrcs 0 lsrc)

BISON_TARGET(parser ${ysrc} ${CMAKE_CURRENT_BINARY_DIR}/HFParser.cc
            DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/HFParser.hh
            COMPILE_FLAGS "--debug" )

FLEX_TARGET(lexer ${lsrc} ${CMAKE_CURRENT_BINARY_DIR}/HFScanner.cc)

ADD_FLEX_BISON_DEPENDENCY(lexer parser)

FILE(GLOB_RECURSE srcs "*.cc")

list(APPEND srcs    ${CMAKE_CURRENT_BINARY_DIR}/HFParser.cc 
                    ${CMAKE_CURRENT_BINARY_DIR}/HFScanner.cc)

add_library(HaveFunParser STATIC ${srcs})

target_include_directories(HaveFunParser
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    "../../include")

find_library(LEX_LIB lexlib)
target_link_libraries(HaveFunParser
    PUBLIC
    ${lexlib}
    HaveFunLib)
