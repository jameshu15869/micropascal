add_llvm_executable(main main.cpp)
target_link_libraries(main ${llvm_libs})

foreach (dir IN ITEMS lexer ast)
    add_subdirectory(${dir})
    target_link_libraries(main ${dir})
endforeach()
