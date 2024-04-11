set(SOURCE_FILES "")

foreach (dir IN ITEMS lexer ast logger parser codegen)
    list(APPEND SOURCE_FILES "${dir}/${dir}.cpp" "${dir}/${dir}.h")
endforeach()

list(APPEND SOURCE_FILES "kaleidoscopejit/KaleidoscopeJIT.h")

message("SOURCES ${SOURCE_FILES}")
add_llvm_executable(main main.cpp ${SOURCE_FILES})
target_link_libraries(main PRIVATE ${llvm_libs})

# Required to read extern functions
target_link_options(main PRIVATE "-rdynamic")
