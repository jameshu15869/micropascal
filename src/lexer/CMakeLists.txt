add_library(lexer lexer.h lexer.cpp)
target_link_libraries(lexer ${llvm_libs})
