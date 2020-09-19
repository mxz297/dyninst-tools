#include <stdio.h>

void *origMalloc(unsigned long size);

void *malloc_wrapper(unsigned long size) {
    printf("Enter malloc_wrapper, allocate %lu bytes\n", size);
    void* ret = origMalloc(size);
    printf("Exit malloc_wrapper\n");
    return ret;
}
