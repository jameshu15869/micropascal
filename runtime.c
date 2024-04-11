#include <stdio.h>
#include <inttypes.h>

void writeln(uint64_t v) {
    printf("%" PRIi64, v);
    printf("\n");
}