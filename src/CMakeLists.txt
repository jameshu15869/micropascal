set(SOURCE_FILES "")

foreach (dir IN ITEMS lexer ast logger parser codegen)
    list(APPEND SOURCE_FILES "${dir}/${dir}.cpp" "${dir}/${dir}.h")
endforeach()

message("SOURCES ${SOURCE_FILES}")
add_llvm_executable(main main.cpp ${SOURCE_FILES})
target_link_libraries(main ${llvm_libs})
