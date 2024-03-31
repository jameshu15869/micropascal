add_library(ast ast.h ast.cpp)
target_link_libraries(lexer ${llvm_libs})
